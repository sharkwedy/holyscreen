#include "bible/BibleRepository.h"

#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <algorithm>

namespace churchpresenter {

namespace {

QString nonNullTrimmed(const QString &value)
{
    const auto trimmed = value.trimmed();
    return trimmed.isNull() ? QStringLiteral("") : trimmed;
}

} // namespace

BibleRepository::BibleRepository(QString databasePath)
    : m_databasePath(std::move(databasePath))
{
}

BibleRepository::~BibleRepository()
{
    if (m_connectionName.isEmpty()) return;
    {
        auto database = QSqlDatabase::database(m_connectionName, false);
        if (database.isValid()) database.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool BibleRepository::open()
{
    if (!m_connectionName.isEmpty()
        && QSqlDatabase::database(m_connectionName, false).isOpen()) return true;
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) return false;

    m_connectionName = QStringLiteral("holyscreen-bible-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_databasePath);
    if (!database.open()) {
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }

    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA foreign_keys=ON"))) return false;
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bible_translations ("
            "id TEXT PRIMARY KEY NOT NULL,"
            "name TEXT NOT NULL,"
            "abbreviation TEXT NOT NULL,"
            "language TEXT NOT NULL"
            ")"))) return false;
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bible_verses ("
            "translation_id TEXT NOT NULL,"
            "book INTEGER NOT NULL,"
            "chapter INTEGER NOT NULL,"
            "verse INTEGER NOT NULL,"
            "text TEXT NOT NULL,"
            "PRIMARY KEY(translation_id,book,chapter,verse),"
            "FOREIGN KEY(translation_id) REFERENCES bible_translations(id) ON DELETE CASCADE"
            ")"))) return false;
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bible_translation_sources ("
            "translation_id TEXT PRIMARY KEY NOT NULL,"
            "source_kind TEXT NOT NULL,source_location TEXT NOT NULL,"
            "source_revision TEXT NOT NULL DEFAULT '',license TEXT NOT NULL,"
            "publisher TEXT NOT NULL DEFAULT '',source_name TEXT NOT NULL DEFAULT '',"
            "source_code TEXT NOT NULL DEFAULT '',scope TEXT NOT NULL DEFAULT '',"
            "imported_at TEXT NOT NULL,content_hash TEXT NOT NULL DEFAULT '',"
            "FOREIGN KEY(translation_id) REFERENCES bible_translations(id) ON DELETE CASCADE"
            ")"))) return false;
    return query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS bible_verses_reference_idx "
        "ON bible_verses(translation_id,book,chapter,verse)"));
}

QString BibleRepository::saveTranslation(BibleTranslation translation)
{
    if (m_connectionName.isEmpty() && !open()) return {};
    translation.name = translation.name.trimmed();
    translation.abbreviation = translation.abbreviation.trimmed().toUpper();
    translation.language = translation.language.trimmed();
    if (translation.name.isEmpty() || translation.abbreviation.isEmpty()
        || translation.language.isEmpty()) return {};
    if (translation.id.isEmpty()) {
        translation.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO bible_translations(id,name,abbreviation,language) "
        "VALUES(:id,:name,:abbreviation,:language) "
        "ON CONFLICT(id) DO UPDATE SET name=excluded.name,"
        "abbreviation=excluded.abbreviation,language=excluded.language"));
    query.bindValue(QStringLiteral(":id"), translation.id);
    query.bindValue(QStringLiteral(":name"), translation.name);
    query.bindValue(QStringLiteral(":abbreviation"), translation.abbreviation);
    query.bindValue(QStringLiteral(":language"), translation.language);
    return query.exec() ? translation.id : QString{};
}

QVector<BibleTranslation> BibleRepository::translations() const
{
    QVector<BibleTranslation> result;
    if (m_connectionName.isEmpty()) return result;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    if (!query.exec(QStringLiteral(
            "SELECT id,name,abbreviation,language FROM bible_translations "
            "ORDER BY name COLLATE NOCASE,id"))) return result;
    while (query.next()) {
        result.append({query.value(0).toString(), query.value(1).toString(),
                       query.value(2).toString(), query.value(3).toString()});
    }
    return result;
}

bool BibleRepository::importVerses(
    const QString &translationId, const QVector<BibleVerse> &versesToImport)
{
    if (m_connectionName.isEmpty() || translationId.isEmpty() || versesToImport.isEmpty()) return false;
    auto database = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery translationQuery(database);
    translationQuery.prepare(QStringLiteral("SELECT 1 FROM bible_translations WHERE id=:id"));
    translationQuery.bindValue(QStringLiteral(":id"), translationId);
    if (!translationQuery.exec() || !translationQuery.next() || !database.transaction()) return false;

    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO bible_verses(translation_id,book,chapter,verse,text) "
        "VALUES(:translation,:book,:chapter,:verse,:text) "
        "ON CONFLICT(translation_id,book,chapter,verse) DO UPDATE SET text=excluded.text"));
    for (const auto &verse : versesToImport) {
        const auto text = verse.text.trimmed();
        if (verse.book == BibleBook::Unknown || verse.chapter <= 0 || verse.verse <= 0 || text.isEmpty()) {
            database.rollback();
            return false;
        }
        query.bindValue(QStringLiteral(":translation"), translationId);
        query.bindValue(QStringLiteral(":book"), static_cast<int>(verse.book));
        query.bindValue(QStringLiteral(":chapter"), verse.chapter);
        query.bindValue(QStringLiteral(":verse"), verse.verse);
        query.bindValue(QStringLiteral(":text"), text);
        if (!query.exec()) {
            database.rollback();
            return false;
        }
    }
    return database.commit();
}

bool BibleRepository::replaceImportedTranslation(
    const PlannedBibleTranslation &planned, const BibleImportCancellation &cancel)
{
    if (m_connectionName.isEmpty() && !open()) return false;
    auto translation = planned.translation;
    translation.id = translation.id.trimmed();
    translation.name = translation.name.trimmed();
    translation.abbreviation = translation.abbreviation.trimmed().toUpper();
    translation.language = translation.language.trimmed();
    if ((cancel && cancel()) || translation.id.isEmpty() || translation.name.isEmpty()
        || translation.abbreviation.isEmpty() || translation.language.isEmpty()
        || planned.verses.isEmpty() || planned.source.license.trimmed().isEmpty()) return false;
    for (const auto &verse : planned.verses) {
        if (cancel && cancel()) return false;
        if (verse.book == BibleBook::Unknown || verse.chapter <= 0 || verse.verse <= 0
            || verse.text.trimmed().isEmpty()) return false;
    }

    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.transaction()) return false;
    const auto rollback = [&database] { database.rollback(); return false; };

    QSqlQuery translationQuery(database);
    translationQuery.prepare(QStringLiteral(
        "INSERT INTO bible_translations(id,name,abbreviation,language) "
        "VALUES(:id,:name,:abbreviation,:language) "
        "ON CONFLICT(id) DO UPDATE SET name=excluded.name,"
        "abbreviation=excluded.abbreviation,language=excluded.language"));
    translationQuery.bindValue(QStringLiteral(":id"), translation.id);
    translationQuery.bindValue(QStringLiteral(":name"), translation.name);
    translationQuery.bindValue(QStringLiteral(":abbreviation"), translation.abbreviation);
    translationQuery.bindValue(QStringLiteral(":language"), translation.language);
    if (!translationQuery.exec()) return rollback();

    QSqlQuery deleteQuery(database);
    deleteQuery.prepare(QStringLiteral("DELETE FROM bible_verses WHERE translation_id=:id"));
    deleteQuery.bindValue(QStringLiteral(":id"), translation.id);
    if (!deleteQuery.exec()) return rollback();

    QSqlQuery verseQuery(database);
    verseQuery.prepare(QStringLiteral(
        "INSERT INTO bible_verses(translation_id,book,chapter,verse,text) "
        "VALUES(:translation,:book,:chapter,:verse,:text)"));
    for (const auto &verse : planned.verses) {
        if (cancel && cancel()) return rollback();
        verseQuery.bindValue(QStringLiteral(":translation"), translation.id);
        verseQuery.bindValue(QStringLiteral(":book"), static_cast<int>(verse.book));
        verseQuery.bindValue(QStringLiteral(":chapter"), verse.chapter);
        verseQuery.bindValue(QStringLiteral(":verse"), verse.verse);
        verseQuery.bindValue(QStringLiteral(":text"), verse.text.trimmed());
        if (!verseQuery.exec()) return rollback();
    }

    const auto importedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QSqlQuery sourceQuery(database);
    sourceQuery.prepare(QStringLiteral(
        "INSERT INTO bible_translation_sources("
        "translation_id,source_kind,source_location,source_revision,license,publisher,"
        "source_name,source_code,scope,imported_at,content_hash) "
        "VALUES(:translation,:kind,:location,:revision,:license,:publisher,"
        ":sourceName,:sourceCode,:scope,:importedAt,:contentHash) "
        "ON CONFLICT(translation_id) DO UPDATE SET "
        "source_kind=excluded.source_kind,source_location=excluded.source_location,"
        "source_revision=excluded.source_revision,license=excluded.license,"
        "publisher=excluded.publisher,source_name=excluded.source_name,"
        "source_code=excluded.source_code,scope=excluded.scope,"
        "imported_at=excluded.imported_at,content_hash=excluded.content_hash"));
    sourceQuery.bindValue(QStringLiteral(":translation"), translation.id);
    sourceQuery.bindValue(QStringLiteral(":kind"), bibleSourceKindName(planned.source.kind));
    sourceQuery.bindValue(QStringLiteral(":location"), nonNullTrimmed(planned.source.location));
    sourceQuery.bindValue(QStringLiteral(":revision"), nonNullTrimmed(planned.source.revision));
    sourceQuery.bindValue(QStringLiteral(":license"), nonNullTrimmed(planned.source.license));
    sourceQuery.bindValue(QStringLiteral(":publisher"), nonNullTrimmed(planned.source.publisher));
    sourceQuery.bindValue(QStringLiteral(":sourceName"), nonNullTrimmed(planned.source.sourceName));
    sourceQuery.bindValue(QStringLiteral(":sourceCode"), nonNullTrimmed(planned.source.sourceCode));
    sourceQuery.bindValue(QStringLiteral(":scope"), nonNullTrimmed(planned.source.scope));
    sourceQuery.bindValue(QStringLiteral(":importedAt"), importedAt);
    sourceQuery.bindValue(QStringLiteral(":contentHash"), nonNullTrimmed(planned.source.contentHash));
    if (!sourceQuery.exec()) return rollback();
    return database.commit();
}

std::optional<BibleTranslationSource> BibleRepository::translationSource(
    const QString &translationId) const
{
    if (m_connectionName.isEmpty() || translationId.trimmed().isEmpty()) return std::nullopt;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "SELECT translation_id,source_kind,source_location,source_revision,license,publisher,"
        "source_name,source_code,scope,imported_at,content_hash "
        "FROM bible_translation_sources WHERE translation_id=:id"));
    query.bindValue(QStringLiteral(":id"), translationId);
    if (!query.exec() || !query.next()) return std::nullopt;
    const auto kindName = query.value(1).toString();
    BibleSourceKind kind = BibleSourceKind::LocalFolder;
    if (kindName == QStringLiteral("git-https")) kind = BibleSourceKind::GitHttps;
    else if (kindName == QStringLiteral("zip-url")) kind = BibleSourceKind::ZipUrl;
    else if (kindName == QStringLiteral("holyscreen-json")) kind = BibleSourceKind::HolyScreenJson;
    return BibleTranslationSource{
        .translationId = query.value(0).toString(),
        .kind = kind,
        .location = query.value(2).toString(),
        .revision = query.value(3).toString(),
        .license = query.value(4).toString(),
        .publisher = query.value(5).toString(),
        .sourceName = query.value(6).toString(),
        .sourceCode = query.value(7).toString(),
        .scope = query.value(8).toString(),
        .importedAt = query.value(9).toString(),
        .contentHash = query.value(10).toString(),
    };
}

QVector<BibleVerse> BibleRepository::verses(
    const QString &translationId, const BibleReference &reference) const
{
    QVector<BibleVerse> result;
    if (m_connectionName.isEmpty() || translationId.isEmpty() || !reference.isValid()) return result;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "SELECT translation_id,book,chapter,verse,text FROM bible_verses "
        "WHERE translation_id=:translation AND book=:book AND chapter=:chapter "
        "AND verse BETWEEN :firstVerse AND :lastVerse ORDER BY verse"));
    query.bindValue(QStringLiteral(":translation"), translationId);
    query.bindValue(QStringLiteral(":book"), static_cast<int>(reference.book));
    query.bindValue(QStringLiteral(":chapter"), reference.chapter);
    query.bindValue(QStringLiteral(":firstVerse"), reference.firstVerse);
    query.bindValue(QStringLiteral(":lastVerse"), reference.lastVerse);
    if (!query.exec()) return result;
    while (query.next()) {
        result.append({query.value(0).toString(), static_cast<BibleBook>(query.value(1).toInt()),
                       query.value(2).toInt(), query.value(3).toInt(), query.value(4).toString()});
    }
    return result;
}

QVector<int> BibleRepository::chapters(
    const QString &translationId, BibleBook book) const
{
    QVector<int> result;
    if (m_connectionName.isEmpty() || translationId.isEmpty()
        || book == BibleBook::Unknown) return result;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "SELECT DISTINCT chapter FROM bible_verses "
        "WHERE translation_id=:translation AND book=:book ORDER BY chapter"));
    query.bindValue(QStringLiteral(":translation"), translationId);
    query.bindValue(QStringLiteral(":book"), static_cast<int>(book));
    if (!query.exec()) return result;
    while (query.next()) result.append(query.value(0).toInt());
    return result;
}

QVector<int> BibleRepository::verseNumbers(
    const QString &translationId, BibleBook book, int chapter) const
{
    QVector<int> result;
    if (m_connectionName.isEmpty() || translationId.isEmpty()
        || book == BibleBook::Unknown || chapter <= 0) return result;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "SELECT verse FROM bible_verses "
        "WHERE translation_id=:translation AND book=:book AND chapter=:chapter "
        "ORDER BY verse"));
    query.bindValue(QStringLiteral(":translation"), translationId);
    query.bindValue(QStringLiteral(":book"), static_cast<int>(book));
    query.bindValue(QStringLiteral(":chapter"), chapter);
    if (!query.exec()) return result;
    while (query.next()) result.append(query.value(0).toInt());
    return result;
}

QVector<BibleVerse> BibleRepository::search(
    const QString &translationId, const QString &text, int limit) const
{
    QVector<BibleVerse> result;
    auto escaped = text.trimmed();
    if (m_connectionName.isEmpty() || translationId.isEmpty() || escaped.isEmpty()) return result;
    escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escaped.replace(QStringLiteral("%"), QStringLiteral("\\%"));
    escaped.replace(QStringLiteral("_"), QStringLiteral("\\_"));

    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "SELECT translation_id,book,chapter,verse,text FROM bible_verses "
        "WHERE translation_id=:translation AND text LIKE :text ESCAPE '\\' COLLATE NOCASE "
        "ORDER BY book,chapter,verse LIMIT :limit"));
    query.bindValue(QStringLiteral(":translation"), translationId);
    query.bindValue(QStringLiteral(":text"), QStringLiteral("%%1%").arg(escaped));
    query.bindValue(QStringLiteral(":limit"), std::clamp(limit, 1, 1000));
    if (!query.exec()) return result;
    while (query.next()) {
        result.append({query.value(0).toString(), static_cast<BibleBook>(query.value(1).toInt()),
                       query.value(2).toInt(), query.value(3).toInt(), query.value(4).toString()});
    }
    return result;
}

} // namespace churchpresenter
