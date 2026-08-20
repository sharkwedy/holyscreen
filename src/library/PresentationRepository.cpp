#include "library/PresentationRepository.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QDebug>

namespace churchpresenter {

PresentationRepository::PresentationRepository(QString path) : m_databasePath(std::move(path)) {}
PresentationRepository::~PresentationRepository()
{
    if (m_connectionName.isEmpty()) return;
    { auto database = QSqlDatabase::database(m_connectionName, false); if (database.isValid()) database.close(); }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool PresentationRepository::open()
{
    if (!m_connectionName.isEmpty() && QSqlDatabase::database(m_connectionName, false).isOpen()) return true;
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) return false;
    m_connectionName = QStringLiteral("holyscreen-presentations-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_databasePath);
    if (!database.open()) return false;
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys=ON"))) return false;
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS presentations (id TEXT PRIMARY KEY NOT NULL,type INTEGER NOT NULL,title TEXT NOT NULL,author TEXT NOT NULL DEFAULT '',default_theme TEXT NOT NULL DEFAULT '')"))) return false;
    if(!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS slides (id TEXT PRIMARY KEY NOT NULL,presentation_id TEXT NOT NULL,label TEXT NOT NULL DEFAULT '',text TEXT NOT NULL DEFAULT '',position INTEGER NOT NULL,theme_override TEXT NOT NULL DEFAULT '',FOREIGN KEY(presentation_id) REFERENCES presentations(id) ON DELETE CASCADE)"))) return false;
    return query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS presentation_sequence(presentation_id TEXT NOT NULL,slide_id TEXT NOT NULL,position INTEGER NOT NULL,PRIMARY KEY(presentation_id,position),FOREIGN KEY(presentation_id) REFERENCES presentations(id) ON DELETE CASCADE)"));
}

QVector<Presentation> PresentationRepository::presentations(PresentationType type) const
{
    QVector<Presentation> result;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("SELECT id FROM presentations WHERE type=:type ORDER BY title COLLATE NOCASE,id"));
    query.bindValue(QStringLiteral(":type"), static_cast<int>(type));
    if (!query.exec()) return result;
    while (query.next()) result.append(presentation(query.value(0).toString()));
    return result;
}

Presentation PresentationRepository::presentation(const QString &id) const
{
    Presentation item;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("SELECT id,type,title,author,default_theme FROM presentations WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) return item;
    item.id = query.value(0).toString(); item.type = static_cast<PresentationType>(query.value(1).toInt());
    item.title = query.value(2).toString(); item.author = query.value(3).toString(); item.defaultTheme = query.value(4).toString();
    QSqlQuery slides(QSqlDatabase::database(m_connectionName, false));
    slides.prepare(QStringLiteral("SELECT id,label,text,position,theme_override FROM slides WHERE presentation_id=:id ORDER BY position,id"));
    slides.bindValue(QStringLiteral(":id"), id);
    if (slides.exec()) while (slides.next()) item.slides.append(Slide{.id=slides.value(0).toString(), .label=slides.value(1).toString(), .text=slides.value(2).toString(), .order=slides.value(3).toInt(), .themeOverride=slides.value(4).toString()});
    QSqlQuery sequence(QSqlDatabase::database(m_connectionName,false));sequence.prepare("SELECT slide_id FROM presentation_sequence WHERE presentation_id=:id ORDER BY position");sequence.bindValue(":id",id);if(sequence.exec())while(sequence.next())item.sequence.append(sequence.value(0).toString());
    return item;
}

QString PresentationRepository::save(Presentation item)
{
    if (m_connectionName.isEmpty() && !open()) return {};
    if (item.id.isEmpty()) item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (item.title.trimmed().isEmpty()) item.title = QStringLiteral("Sem título");
    if (item.author.isNull()) item.author = QStringLiteral("");
    if (item.defaultTheme.isNull()) item.defaultTheme = QStringLiteral("");
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.transaction()) return {};
    QSqlQuery query(database);
    query.prepare(QStringLiteral("INSERT INTO presentations(id,type,title,author,default_theme) VALUES(:id,:type,:title,:author,:theme) ON CONFLICT(id) DO UPDATE SET type=excluded.type,title=excluded.title,author=excluded.author,default_theme=excluded.default_theme"));
    query.bindValue(":id",item.id); query.bindValue(":type",static_cast<int>(item.type)); query.bindValue(":title",item.title.trimmed()); query.bindValue(":author",item.author); query.bindValue(":theme",item.defaultTheme);
    if (!query.exec()) { database.rollback(); return {}; }
    query.prepare(QStringLiteral("DELETE FROM slides WHERE presentation_id=:id")); query.bindValue(":id",item.id);
    if (!query.exec()) { database.rollback(); return {}; }
    query.prepare(QStringLiteral("INSERT INTO slides(id,presentation_id,label,text,position,theme_override) VALUES(:id,:presentation,:label,:text,:position,:theme)"));
    for (int index=0; index<item.slides.size(); ++index) {
        auto slide=item.slides[index]; if (slide.id.isEmpty()) slide.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (slide.label.isNull()) slide.label = QStringLiteral("");
        if (slide.text.isNull()) slide.text = QStringLiteral("");
        if (slide.themeOverride.isNull()) slide.themeOverride = QStringLiteral("");
        query.bindValue(":id",slide.id); query.bindValue(":presentation",item.id); query.bindValue(":label",slide.label); query.bindValue(":text",slide.text); query.bindValue(":position",index); query.bindValue(":theme",slide.themeOverride);
        if (!query.exec()) { qWarning() << query.lastError(); database.rollback(); return {}; }
    }
    query.prepare("DELETE FROM presentation_sequence WHERE presentation_id=:id");query.bindValue(":id",item.id);if(!query.exec()){database.rollback();return{};}
    query.prepare("INSERT INTO presentation_sequence(presentation_id,slide_id,position) VALUES(:presentation,:slide,:position)");
    for(int index=0;index<item.sequence.size();++index){query.bindValue(":presentation",item.id);query.bindValue(":slide",item.sequence[index]);query.bindValue(":position",index);if(!query.exec()){database.rollback();return{};}}
    return database.commit() ? item.id : QString{};
}

bool PresentationRepository::remove(const QString &id)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("DELETE FROM presentations WHERE id=:id")); query.bindValue(":id",id);
    return query.exec() && query.numRowsAffected()==1;
}

} // namespace churchpresenter
