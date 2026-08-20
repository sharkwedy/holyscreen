#include "persistence/SettingsRepository.h"

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace churchpresenter {

SettingsRepository::SettingsRepository(const QString &databasePath, QObject *parent)
    : QObject(parent)
    , m_databasePath(databasePath)
{
}

SettingsRepository::~SettingsRepository()
{
    if (!m_connectionName.isEmpty()) {
        {
            auto database = QSqlDatabase::database(m_connectionName, false);
            if (database.isValid()) {
                database.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool SettingsRepository::open()
{
    if (!m_connectionName.isEmpty() && QSqlDatabase::database(m_connectionName, false).isOpen()) {
        return true;
    }

    const QFileInfo databaseInfo(m_databasePath);
    if (!QDir().mkpath(databaseInfo.absolutePath())) {
        return false;
    }

    m_connectionName = QStringLiteral("holyscreen-settings-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_databasePath);
    if (!database.open()) {
        qWarning() << "Could not open settings database:" << database.lastError().text();
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }

    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA busy_timeout=3000"))) {
        qWarning() << "Could not configure settings database:" << query.lastError().text();
    }
    constexpr auto schema =
        "CREATE TABLE IF NOT EXISTS settings ("
        "key TEXT PRIMARY KEY NOT NULL,"
        "value BLOB NOT NULL"
        ")";
    if (!query.exec(QString::fromLatin1(schema))) {
        qWarning() << "Could not create settings table:" << query.lastError().text();
        database.close();
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }
    return true;
}

bool SettingsRepository::setValue(const QString &key, const QVariant &value)
{
    if ((m_connectionName.isEmpty() || !QSqlDatabase::database(m_connectionName, false).isOpen()) && !open()) {
        return false;
    }

    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_8);
    stream << value;
    if (stream.status() != QDataStream::Ok) {
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO settings(key, value) VALUES(:key, :value) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    query.bindValue(QStringLiteral(":key"), key);
    query.bindValue(QStringLiteral(":value"), payload);
    if (!query.exec()) {
        qWarning() << "Could not save setting:" << query.lastError().text();
        return false;
    }

    emit valueChanged(key, value);
    return true;
}

QVariant SettingsRepository::value(const QString &key, const QVariant &fallback) const
{
    if (m_connectionName.isEmpty()) {
        return fallback;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("SELECT value FROM settings WHERE key=:key"));
    query.bindValue(QStringLiteral(":key"), key);
    if (!query.exec() || !query.next()) {
        return fallback;
    }

    auto payload = query.value(0).toByteArray();
    QDataStream stream(&payload, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_6_8);
    QVariant stored;
    stream >> stored;
    if (stream.status() != QDataStream::Ok) {
        return fallback;
    }
    return stored;
}

} // namespace churchpresenter
