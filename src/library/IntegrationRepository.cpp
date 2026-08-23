#include "library/IntegrationRepository.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace churchpresenter {
namespace {

QString encodeConfiguration(const QVariantMap &configuration)
{
    return QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(configuration)).toJson(QJsonDocument::Compact));
}

QVariantMap decodeConfiguration(const QString &json)
{
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    return document.isObject() ? document.object().toVariantMap() : QVariantMap{};
}

QString encodeReferences(const QStringList &references)
{
    return QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(references)).toJson(QJsonDocument::Compact));
}

QStringList decodeReferences(const QString &json)
{
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray()) return {};
    QStringList references;
    for (const auto &value : document.array()) references.append(value.toString());
    return references;
}

//! SQLite grava QString nula como NULL; as colunas de texto são NOT NULL.
QString text(const QString &value)
{
    return value.isNull() ? QString::fromLatin1("") : value;
}

} // namespace

IntegrationRepository::IntegrationRepository(QString databasePath)
    : m_databasePath(std::move(databasePath))
{
}

IntegrationRepository::~IntegrationRepository()
{
    if (m_connectionName.isEmpty()) return;
    {
        auto database = QSqlDatabase::database(m_connectionName, false);
        if (database.isValid()) database.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool IntegrationRepository::open()
{
    if (!m_connectionName.isEmpty()
        && QSqlDatabase::database(m_connectionName, false).isOpen()) {
        return true;
    }
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) return false;
    m_connectionName = QStringLiteral("holyscreen-integrations-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_databasePath);
    if (!database.open()) {
        qWarning() << "Could not open integration database:" << database.lastError().text();
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }
    QSqlQuery query(database);
    const bool ready = query.exec(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name='integration_definitions'"));
    if (!ready || !query.next()) {
        qWarning() << "The integration tables are missing; run the database migrations first.";
        database.close();
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }
    return true;
}

QVector<IntegrationDefinition> IntegrationRepository::definitions() const
{
    QVector<IntegrationDefinition> definitions;
    if (m_connectionName.isEmpty()) return definitions;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    if (!query.exec(QStringLiteral(
            "SELECT id,name,type,enabled,configuration,secret_references,timeout_ms,"
            "retry_attempts,retry_backoff_ms FROM integration_definitions ORDER BY name,id"))) {
        qWarning() << "Could not read integrations:" << query.lastError().text();
        return definitions;
    }
    while (query.next()) {
        const auto type = integrationTypeFromName(query.value(2).toString());
        if (!type.has_value()) {
            qWarning() << "Ignoring integration with an unknown type:" << query.value(2).toString();
            continue;
        }
        definitions.append(IntegrationDefinition{
            .id = query.value(0).toString(),
            .name = query.value(1).toString(),
            .type = *type,
            .enabled = query.value(3).toBool(),
            .configuration = decodeConfiguration(query.value(4).toString()),
            .secretReferences = decodeReferences(query.value(5).toString()),
            .timeoutMs = query.value(6).toInt(),
            .retryPolicy = {.maximumAttempts = query.value(7).toInt(),
                            .backoffMs = query.value(8).toInt()},
        });
    }
    return definitions;
}

bool IntegrationRepository::save(const IntegrationDefinition &definition)
{
    if (m_connectionName.isEmpty() && !open()) return false;
    if (definition.id.trimmed().isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO integration_definitions(id,name,type,enabled,configuration,"
        "secret_references,timeout_ms,retry_attempts,retry_backoff_ms,updated_at) "
        "VALUES(:id,:name,:type,:enabled,:configuration,:references,:timeout,:attempts,"
        ":backoff,:updated) "
        "ON CONFLICT(id) DO UPDATE SET name=excluded.name,type=excluded.type,"
        "enabled=excluded.enabled,configuration=excluded.configuration,"
        "secret_references=excluded.secret_references,timeout_ms=excluded.timeout_ms,"
        "retry_attempts=excluded.retry_attempts,retry_backoff_ms=excluded.retry_backoff_ms,"
        "updated_at=excluded.updated_at"));
    query.bindValue(QStringLiteral(":id"), definition.id);
    query.bindValue(QStringLiteral(":name"), definition.name);
    query.bindValue(QStringLiteral(":type"), integrationTypeName(definition.type));
    query.bindValue(QStringLiteral(":enabled"), definition.enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":configuration"), encodeConfiguration(definition.configuration));
    query.bindValue(QStringLiteral(":references"), encodeReferences(definition.secretReferences));
    query.bindValue(QStringLiteral(":timeout"), definition.timeoutMs);
    query.bindValue(QStringLiteral(":attempts"), definition.retryPolicy.maximumAttempts);
    query.bindValue(QStringLiteral(":backoff"), definition.retryPolicy.backoffMs);
    query.bindValue(QStringLiteral(":updated"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        qWarning() << "Could not save integration:" << query.lastError().text();
        return false;
    }
    return true;
}

bool IntegrationRepository::remove(const QString &integrationId)
{
    if (m_connectionName.isEmpty()) return false;
    QSqlQuery history(QSqlDatabase::database(m_connectionName, false));
    history.prepare(QStringLiteral(
        "DELETE FROM integration_call_history WHERE integration_id=:id"));
    history.bindValue(QStringLiteral(":id"), integrationId);
    history.exec();

    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("DELETE FROM integration_definitions WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), integrationId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool IntegrationRepository::recordCall(const IntegrationCall &call)
{
    if (m_connectionName.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO integration_call_history(id,integration_id,operation,correlation_id,"
        "accepted,error_code,message,duration_ms,attempts,occurred_at) "
        "VALUES(:id,:integration,:operation,:correlation,:accepted,:error,:message,:duration,"
        ":attempts,:occurred)"));
    query.bindValue(QStringLiteral(":id"),
                    call.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                                      : call.id);
    query.bindValue(QStringLiteral(":integration"), call.integrationId);
    query.bindValue(QStringLiteral(":operation"), text(call.operation));
    query.bindValue(QStringLiteral(":correlation"), text(call.correlationId));
    query.bindValue(QStringLiteral(":accepted"), call.accepted ? 1 : 0);
    query.bindValue(QStringLiteral(":error"), text(call.errorCode));
    query.bindValue(QStringLiteral(":message"), text(call.message));
    query.bindValue(QStringLiteral(":duration"), call.durationMs);
    query.bindValue(QStringLiteral(":attempts"), call.attempts);
    query.bindValue(QStringLiteral(":occurred"),
                    (call.occurredAt.isValid() ? call.occurredAt
                                               : QDateTime::currentDateTimeUtc())
                        .toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        qWarning() << "Could not record the integration call:" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<IntegrationCall> IntegrationRepository::history(const QString &integrationId,
                                                        int limit) const
{
    QVector<IntegrationCall> calls;
    if (m_connectionName.isEmpty()) return calls;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    if (integrationId.isEmpty()) {
        query.prepare(QStringLiteral(
            "SELECT id,integration_id,operation,correlation_id,accepted,error_code,message,"
            "duration_ms,attempts,occurred_at FROM integration_call_history "
            "ORDER BY occurred_at DESC,id DESC LIMIT :limit"));
    } else {
        query.prepare(QStringLiteral(
            "SELECT id,integration_id,operation,correlation_id,accepted,error_code,message,"
            "duration_ms,attempts,occurred_at FROM integration_call_history "
            "WHERE integration_id=:integration ORDER BY occurred_at DESC,id DESC LIMIT :limit"));
        query.bindValue(QStringLiteral(":integration"), integrationId);
    }
    query.bindValue(QStringLiteral(":limit"), std::max(0, limit));
    if (!query.exec()) return calls;
    while (query.next()) {
        calls.append(IntegrationCall{
            .id = query.value(0).toString(),
            .integrationId = query.value(1).toString(),
            .operation = query.value(2).toString(),
            .correlationId = query.value(3).toString(),
            .accepted = query.value(4).toBool(),
            .errorCode = query.value(5).toString(),
            .message = query.value(6).toString(),
            .durationMs = query.value(7).toInt(),
            .attempts = query.value(8).toInt(),
            .occurredAt = QDateTime::fromString(query.value(9).toString(), Qt::ISODateWithMs),
        });
    }
    return calls;
}

int IntegrationRepository::pruneHistory(int maximumEntriesPerIntegration)
{
    if (m_connectionName.isEmpty() || maximumEntriesPerIntegration <= 0) return 0;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "DELETE FROM integration_call_history WHERE id IN ("
        "SELECT id FROM (SELECT id,ROW_NUMBER() OVER ("
        "PARTITION BY integration_id ORDER BY occurred_at DESC,id DESC) AS position "
        "FROM integration_call_history) WHERE position > :maximum)"));
    query.bindValue(QStringLiteral(":maximum"), maximumEntriesPerIntegration);
    if (!query.exec()) return 0;
    return query.numRowsAffected();
}

} // namespace churchpresenter
