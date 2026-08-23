#include "library/AutomationRepository.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace churchpresenter {
namespace {

QString encodeMap(const QVariantMap &value)
{
    return QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact));
}

QVariantMap decodeMap(const QString &json)
{
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    return document.isObject() ? document.object().toVariantMap() : QVariantMap{};
}

QString encodeVariant(const QVariant &value)
{
    return QString::fromUtf8(
        QJsonDocument(QJsonObject{{QStringLiteral("v"), QJsonValue::fromVariant(value)}})
            .toJson(QJsonDocument::Compact));
}

QVariant decodeVariant(const QString &json)
{
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isObject()) return {};
    return document.object().value(QStringLiteral("v")).toVariant();
}

QString encodeOutcomes(const QList<ActionOutcome> &outcomes)
{
    QJsonArray array;
    for (const auto &outcome : outcomes) {
        array.append(QJsonObject{{QStringLiteral("actionType"), outcome.actionType},
                                 {QStringLiteral("accepted"), outcome.accepted},
                                 {QStringLiteral("errorCode"), outcome.errorCode},
                                 {QStringLiteral("message"), outcome.message},
                                 {QStringLiteral("durationMs"), outcome.durationMs}});
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<ActionOutcome> decodeOutcomes(const QString &json)
{
    QList<ActionOutcome> outcomes;
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray()) return outcomes;
    for (const auto &entry : document.array()) {
        const auto object = entry.toObject();
        outcomes.append(ActionOutcome{
            .actionType = object.value(QStringLiteral("actionType")).toString(),
            .accepted = object.value(QStringLiteral("accepted")).toBool(),
            .errorCode = object.value(QStringLiteral("errorCode")).toString(),
            .message = object.value(QStringLiteral("message")).toString(),
            .durationMs = object.value(QStringLiteral("durationMs")).toInt(),
        });
    }
    return outcomes;
}

QString text(const QString &value)
{
    return value.isNull() ? QString::fromLatin1("") : value;
}

} // namespace

AutomationRepository::AutomationRepository(QString databasePath)
    : m_databasePath(std::move(databasePath))
{
}

AutomationRepository::~AutomationRepository()
{
    if (m_connectionName.isEmpty()) return;
    {
        auto database = QSqlDatabase::database(m_connectionName, false);
        if (database.isValid()) database.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool AutomationRepository::open()
{
    if (!m_connectionName.isEmpty()
        && QSqlDatabase::database(m_connectionName, false).isOpen()) {
        return true;
    }
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) return false;
    m_connectionName = QStringLiteral("holyscreen-automations-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_databasePath);
    if (!database.open()) {
        qWarning() << "Could not open the automation database:" << database.lastError().text();
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }
    QSqlQuery query(database);
    const bool ready = query.exec(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name='automations'"));
    if (!ready || !query.next()) {
        qWarning() << "The automation tables are missing; run the database migrations first.";
        database.close();
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }
    return true;
}

QList<Automation> AutomationRepository::automations() const
{
    QList<Automation> automations;
    if (m_connectionName.isEmpty()) return automations;
    auto database = QSqlDatabase::database(m_connectionName, false);

    QSqlQuery query(database);
    if (!query.exec(QStringLiteral(
            "SELECT id,name,enabled,trigger_type,trigger_parameters,condition_group,"
            "debounce_ms,budget_ms,failure_limit,consecutive_failures FROM automations "
            "ORDER BY name,id"))) {
        qWarning() << "Could not read automations:" << query.lastError().text();
        return automations;
    }
    while (query.next()) {
        Automation automation;
        automation.id = query.value(0).toString();
        automation.name = query.value(1).toString();
        automation.enabled = query.value(2).toBool();
        automation.trigger.type = query.value(3).toString();
        automation.trigger.parameters = decodeMap(query.value(4).toString());
        automation.conditionGroup = conditionGroupFromName(query.value(5).toString())
                                        .value_or(ConditionGroup::All);
        automation.debounceMs = query.value(6).toInt();
        automation.budgetMs = query.value(7).toInt();
        automation.failureLimit = query.value(8).toInt();
        automation.consecutiveFailures = query.value(9).toInt();
        automations.append(automation);
    }

    for (auto &automation : automations) {
        QSqlQuery conditions(database);
        conditions.prepare(QStringLiteral(
            "SELECT field,operation,expected FROM automation_conditions "
            "WHERE automation_id=:id ORDER BY position"));
        conditions.bindValue(QStringLiteral(":id"), automation.id);
        if (conditions.exec()) {
            while (conditions.next()) {
                automation.conditions.append(Condition{
                    .field = conditions.value(0).toString(),
                    .operation = conditions.value(1).toString(),
                    .expected = decodeVariant(conditions.value(2).toString()),
                });
            }
        }
        QSqlQuery actions(database);
        actions.prepare(QStringLiteral(
            "SELECT type,parameters FROM automation_actions "
            "WHERE automation_id=:id ORDER BY position"));
        actions.bindValue(QStringLiteral(":id"), automation.id);
        if (actions.exec()) {
            while (actions.next()) {
                automation.actions.append(Action{
                    .type = actions.value(0).toString(),
                    .parameters = decodeMap(actions.value(1).toString()),
                });
            }
        }
    }
    return automations;
}

bool AutomationRepository::save(const Automation &automation)
{
    if (m_connectionName.isEmpty() && !open()) return false;
    if (automation.id.trimmed().isEmpty()) return false;
    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.transaction()) return false;

    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO automations(id,name,enabled,trigger_type,trigger_parameters,"
        "condition_group,debounce_ms,budget_ms,failure_limit,consecutive_failures,updated_at) "
        "VALUES(:id,:name,:enabled,:trigger,:parameters,:group,:debounce,:budget,:limit,"
        ":failures,:updated) "
        "ON CONFLICT(id) DO UPDATE SET name=excluded.name,enabled=excluded.enabled,"
        "trigger_type=excluded.trigger_type,trigger_parameters=excluded.trigger_parameters,"
        "condition_group=excluded.condition_group,debounce_ms=excluded.debounce_ms,"
        "budget_ms=excluded.budget_ms,failure_limit=excluded.failure_limit,"
        "consecutive_failures=excluded.consecutive_failures,updated_at=excluded.updated_at"));
    query.bindValue(QStringLiteral(":id"), automation.id);
    query.bindValue(QStringLiteral(":name"), text(automation.name));
    query.bindValue(QStringLiteral(":enabled"), automation.enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":trigger"), text(automation.trigger.type));
    query.bindValue(QStringLiteral(":parameters"), encodeMap(automation.trigger.parameters));
    query.bindValue(QStringLiteral(":group"), conditionGroupName(automation.conditionGroup));
    query.bindValue(QStringLiteral(":debounce"), automation.debounceMs);
    query.bindValue(QStringLiteral(":budget"), automation.budgetMs);
    query.bindValue(QStringLiteral(":limit"), automation.failureLimit);
    query.bindValue(QStringLiteral(":failures"), automation.consecutiveFailures);
    query.bindValue(QStringLiteral(":updated"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        qWarning() << "Could not save the automation:" << query.lastError().text();
        database.rollback();
        return false;
    }

    // Condições e ações são substituídas por completo, mantendo a ordem.
    for (const auto &table : {QStringLiteral("automation_conditions"),
                              QStringLiteral("automation_actions")}) {
        QSqlQuery cleanup(database);
        cleanup.prepare(QStringLiteral("DELETE FROM %1 WHERE automation_id=:id").arg(table));
        cleanup.bindValue(QStringLiteral(":id"), automation.id);
        if (!cleanup.exec()) {
            database.rollback();
            return false;
        }
    }
    for (qsizetype index = 0; index < automation.conditions.size(); ++index) {
        const auto &condition = automation.conditions[index];
        QSqlQuery insert(database);
        insert.prepare(QStringLiteral(
            "INSERT INTO automation_conditions(automation_id,position,field,operation,expected) "
            "VALUES(:id,:position,:field,:operation,:expected)"));
        insert.bindValue(QStringLiteral(":id"), automation.id);
        insert.bindValue(QStringLiteral(":position"), static_cast<int>(index));
        insert.bindValue(QStringLiteral(":field"), text(condition.field));
        insert.bindValue(QStringLiteral(":operation"), text(condition.operation));
        insert.bindValue(QStringLiteral(":expected"), encodeVariant(condition.expected));
        if (!insert.exec()) {
            database.rollback();
            return false;
        }
    }
    for (qsizetype index = 0; index < automation.actions.size(); ++index) {
        const auto &action = automation.actions[index];
        QSqlQuery insert(database);
        insert.prepare(QStringLiteral(
            "INSERT INTO automation_actions(automation_id,position,type,parameters) "
            "VALUES(:id,:position,:type,:parameters)"));
        insert.bindValue(QStringLiteral(":id"), automation.id);
        insert.bindValue(QStringLiteral(":position"), static_cast<int>(index));
        insert.bindValue(QStringLiteral(":type"), text(action.type));
        insert.bindValue(QStringLiteral(":parameters"), encodeMap(action.parameters));
        if (!insert.exec()) {
            database.rollback();
            return false;
        }
    }
    return database.commit();
}

bool AutomationRepository::remove(const QString &automationId)
{
    if (m_connectionName.isEmpty()) return false;
    auto database = QSqlDatabase::database(m_connectionName, false);
    for (const auto &table : {QStringLiteral("automation_conditions"),
                              QStringLiteral("automation_actions"),
                              QStringLiteral("automation_runs")}) {
        QSqlQuery cleanup(database);
        cleanup.prepare(QStringLiteral("DELETE FROM %1 WHERE automation_id=:id").arg(table));
        cleanup.bindValue(QStringLiteral(":id"), automationId);
        cleanup.exec();
    }
    QSqlQuery query(database);
    query.prepare(QStringLiteral("DELETE FROM automations WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), automationId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool AutomationRepository::updateRuntimeState(const QString &automationId, bool enabled,
                                              int consecutiveFailures)
{
    if (m_connectionName.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "UPDATE automations SET enabled=:enabled,consecutive_failures=:failures WHERE id=:id"));
    query.bindValue(QStringLiteral(":enabled"), enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":failures"), consecutiveFailures);
    query.bindValue(QStringLiteral(":id"), automationId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool AutomationRepository::recordRun(const AutomationRun &run)
{
    if (m_connectionName.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO automation_runs(id,automation_id,correlation_id,status,reason,dry_run,"
        "started_at,finished_at,outcomes) VALUES(:id,:automation,:correlation,:status,:reason,"
        ":dry,:started,:finished,:outcomes)"));
    query.bindValue(QStringLiteral(":id"),
                    run.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : run.id);
    query.bindValue(QStringLiteral(":automation"), text(run.automationId));
    query.bindValue(QStringLiteral(":correlation"), text(run.correlationId));
    query.bindValue(QStringLiteral(":status"), text(run.status));
    query.bindValue(QStringLiteral(":reason"), text(run.reason));
    query.bindValue(QStringLiteral(":dry"), run.dryRun ? 1 : 0);
    query.bindValue(QStringLiteral(":started"),
                    (run.startedAt.isValid() ? run.startedAt : QDateTime::currentDateTimeUtc())
                        .toString(Qt::ISODateWithMs));
    query.bindValue(QStringLiteral(":finished"),
                    run.finishedAt.isValid() ? run.finishedAt.toString(Qt::ISODateWithMs)
                                             : QString::fromLatin1(""));
    query.bindValue(QStringLiteral(":outcomes"), encodeOutcomes(run.outcomes));
    if (!query.exec()) {
        qWarning() << "Could not record the automation run:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<AutomationRun> AutomationRepository::runs(const QString &automationId, int limit) const
{
    QList<AutomationRun> runs;
    if (m_connectionName.isEmpty()) return runs;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    if (automationId.isEmpty()) {
        query.prepare(QStringLiteral(
            "SELECT id,automation_id,correlation_id,status,reason,dry_run,started_at,"
            "finished_at,outcomes FROM automation_runs ORDER BY started_at DESC,id DESC "
            "LIMIT :limit"));
    } else {
        query.prepare(QStringLiteral(
            "SELECT id,automation_id,correlation_id,status,reason,dry_run,started_at,"
            "finished_at,outcomes FROM automation_runs WHERE automation_id=:automation "
            "ORDER BY started_at DESC,id DESC LIMIT :limit"));
        query.bindValue(QStringLiteral(":automation"), automationId);
    }
    query.bindValue(QStringLiteral(":limit"), std::max(0, limit));
    if (!query.exec()) return runs;
    while (query.next()) {
        runs.append(AutomationRun{
            .id = query.value(0).toString(),
            .automationId = query.value(1).toString(),
            .correlationId = query.value(2).toString(),
            .status = query.value(3).toString(),
            .reason = query.value(4).toString(),
            .startedAt = QDateTime::fromString(query.value(6).toString(), Qt::ISODateWithMs),
            .finishedAt = QDateTime::fromString(query.value(7).toString(), Qt::ISODateWithMs),
            .outcomes = decodeOutcomes(query.value(8).toString()),
            .dryRun = query.value(5).toBool(),
        });
    }
    return runs;
}

int AutomationRepository::pruneRuns(int maximumEntriesPerAutomation)
{
    if (m_connectionName.isEmpty() || maximumEntriesPerAutomation <= 0) return 0;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "DELETE FROM automation_runs WHERE id IN ("
        "SELECT id FROM (SELECT id,ROW_NUMBER() OVER ("
        "PARTITION BY automation_id ORDER BY started_at DESC,id DESC) AS position "
        "FROM automation_runs) WHERE position > :maximum)"));
    query.bindValue(QStringLiteral(":maximum"), maximumEntriesPerAutomation);
    if (!query.exec()) return 0;
    return query.numRowsAffected();
}

QList<AuthorizedExecutables::Entry> AutomationRepository::authorizedExecutables() const
{
    QList<AuthorizedExecutables::Entry> entries;
    if (m_connectionName.isEmpty()) return entries;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    if (!query.exec(QStringLiteral(
            "SELECT canonical_path,label,authorized_at FROM authorized_executables "
            "ORDER BY label,canonical_path"))) {
        return entries;
    }
    while (query.next()) {
        entries.append(AuthorizedExecutables::Entry{
            .canonicalPath = query.value(0).toString(),
            .label = query.value(1).toString(),
            .authorizedAt = QDateTime::fromString(query.value(2).toString(), Qt::ISODateWithMs),
        });
    }
    return entries;
}

bool AutomationRepository::authorizeExecutable(const AuthorizedExecutables::Entry &entry)
{
    if (m_connectionName.isEmpty() || entry.canonicalPath.trimmed().isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO authorized_executables(canonical_path,label,authorized_at) "
        "VALUES(:path,:label,:at) ON CONFLICT(canonical_path) DO UPDATE SET label=excluded.label"));
    query.bindValue(QStringLiteral(":path"), entry.canonicalPath);
    query.bindValue(QStringLiteral(":label"), text(entry.label));
    query.bindValue(QStringLiteral(":at"),
                    (entry.authorizedAt.isValid() ? entry.authorizedAt
                                                  : QDateTime::currentDateTimeUtc())
                        .toString(Qt::ISODateWithMs));
    return query.exec();
}

bool AutomationRepository::revokeExecutable(const QString &canonicalPath)
{
    if (m_connectionName.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "DELETE FROM authorized_executables WHERE canonical_path=:path"));
    query.bindValue(QStringLiteral(":path"), canonicalPath);
    return query.exec() && query.numRowsAffected() > 0;
}

} // namespace churchpresenter
