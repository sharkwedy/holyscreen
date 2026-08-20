#include "persistence/DatabaseMigrator.h"

#include <algorithm>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace churchpresenter {

namespace {

QString uniqueConnectionName(const QString &prefix)
{
    return QStringLiteral("holyscreen-%1-%2")
        .arg(prefix, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

} // namespace

DatabaseMigrator::DatabaseMigrator(QString databasePath)
    : m_databasePath(std::move(databasePath))
{
}

bool DatabaseMigrator::addMigration(int version, QString description, Migration migration)
{
    if (version <= 0 || description.trimmed().isEmpty() || !migration) return false;
    for (const auto &entry : m_migrations) {
        if (entry.version == version) return false;
    }
    m_migrations.append({version, std::move(description), std::move(migration)});
    return true;
}

MigrationResult DatabaseMigrator::migrate() const
{
    MigrationResult result;
    if (m_databasePath.trimmed().isEmpty()) {
        result.error = QStringLiteral("Caminho do banco não informado.");
        return result;
    }
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) {
        result.error = QStringLiteral("Não foi possível criar a pasta do banco.");
        return result;
    }

    auto migrations = m_migrations;
    std::sort(migrations.begin(), migrations.end(), [](const Entry &left, const Entry &right) {
        return left.version < right.version;
    });

    const auto inspectionConnection = uniqueConnectionName(QStringLiteral("migration-inspection"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), inspectionConnection);
        database.setDatabaseName(m_databasePath);
        if (!database.open()) {
            result.error = database.lastError().text();
            database.close();
        } else {
            QSqlQuery tableQuery(database);
            if (!tableQuery.exec(QStringLiteral(
                    "SELECT 1 FROM sqlite_master WHERE type='table' AND name='schema_version'"))) {
                result.error = tableQuery.lastError().text();
            } else if (tableQuery.next()) {
                QSqlQuery versionQuery(database);
                if (!versionQuery.exec(QStringLiteral("SELECT COALESCE(MAX(version), 0) FROM schema_version"))
                    || !versionQuery.next()) {
                    result.error = versionQuery.lastError().text();
                } else {
                    result.previousVersion = versionQuery.value(0).toInt();
                    result.currentVersion = result.previousVersion;
                }
            }
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(inspectionConnection);
    if (!result.error.isEmpty()) return result;

    QVector<Entry> pending;
    for (const auto &migration : migrations) {
        if (migration.version > result.previousVersion) pending.append(migration);
    }
    if (pending.isEmpty()) {
        result.success = true;
        return result;
    }

    const QFileInfo existingDatabase(m_databasePath);
    if (existingDatabase.exists() && existingDatabase.size() > 0) {
        result.backupPath = QStringLiteral("%1.backup-%2-%3")
            .arg(m_databasePath,
                 QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmsszzz")),
                 QUuid::createUuid().toString(QUuid::WithoutBraces));
        if (!QFile::copy(m_databasePath, result.backupPath)) {
            result.error = QStringLiteral("Não foi possível criar o backup pré-migração.");
            result.backupPath.clear();
            return result;
        }
    }

    const auto migrationConnection = uniqueConnectionName(QStringLiteral("migration"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), migrationConnection);
        database.setDatabaseName(m_databasePath);
        if (!database.open()) {
            result.error = database.lastError().text();
        } else if (!database.transaction()) {
            result.error = database.lastError().text();
        } else {
            QSqlQuery schemaQuery(database);
            bool applied = schemaQuery.exec(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS schema_version("
                "version INTEGER PRIMARY KEY,"
                "description TEXT NOT NULL,"
                "applied_at TEXT NOT NULL)"));
            if (!applied) result.error = schemaQuery.lastError().text();

            for (const auto &entry : pending) {
                if (!applied) break;
                QString migrationError;
                applied = entry.migration(database, &migrationError);
                if (!applied) {
                    result.error = migrationError.isEmpty()
                        ? QStringLiteral("A migração %1 falhou.").arg(entry.version)
                        : migrationError;
                    break;
                }

                QSqlQuery versionQuery(database);
                versionQuery.prepare(QStringLiteral(
                    "INSERT INTO schema_version(version, description, applied_at) VALUES(?, ?, ?)"));
                versionQuery.addBindValue(entry.version);
                versionQuery.addBindValue(entry.description);
                versionQuery.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
                applied = versionQuery.exec();
                if (!applied) result.error = versionQuery.lastError().text();
                if (applied) result.currentVersion = entry.version;
            }

            if (applied && database.commit()) {
                result.success = true;
            } else {
                if (result.error.isEmpty()) result.error = database.lastError().text();
                database.rollback();
                result.currentVersion = result.previousVersion;
            }
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(migrationConnection);
    return result;
}

} // namespace churchpresenter
