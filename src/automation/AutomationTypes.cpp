#include "automation/AutomationTypes.h"

#include <QVariantList>

namespace churchpresenter {

QStringList automationTriggerTypes()
{
    return {
        QString::fromLatin1(AutomationTrigger::PresentationStarted),
        QString::fromLatin1(AutomationTrigger::PresentationStopped),
        QString::fromLatin1(AutomationTrigger::SongStarted),
        QString::fromLatin1(AutomationTrigger::MediaStarted),
        QString::fromLatin1(AutomationTrigger::MediaPaused),
        QString::fromLatin1(AutomationTrigger::MediaFinished),
        QString::fromLatin1(AutomationTrigger::EventSelected),
        QString::fromLatin1(AutomationTrigger::EventItemExecuted),
        QString::fromLatin1(AutomationTrigger::SlideChanged),
        QString::fromLatin1(AutomationTrigger::LocalTime),
        QString::fromLatin1(AutomationTrigger::RemoteCommandAccepted),
        QString::fromLatin1(AutomationTrigger::TimerStarted),
        QString::fromLatin1(AutomationTrigger::TimerFinished),
    };
}

QStringList automationActionTypes()
{
    return {
        QString::fromLatin1(AutomationAction::Command),
        QString::fromLatin1(AutomationAction::Integration),
        QString::fromLatin1(AutomationAction::Process),
        QString::fromLatin1(AutomationAction::Wait),
    };
}

QStringList automationConditionOperations()
{
    return {QStringLiteral("equals"),      QStringLiteral("notEquals"),
            QStringLiteral("contains"),    QStringLiteral("notContains"),
            QStringLiteral("greaterThan"), QStringLiteral("lessThan"),
            QStringLiteral("between"),     QStringLiteral("timeBetween"),
            QStringLiteral("isEmpty"),     QStringLiteral("isNotEmpty")};
}

QString conditionGroupName(ConditionGroup group)
{
    return group == ConditionGroup::Any ? QStringLiteral("any") : QStringLiteral("all");
}

std::optional<ConditionGroup> conditionGroupFromName(const QString &name)
{
    const auto normalized = name.trimmed().toLower();
    if (normalized == QStringLiteral("all")) return ConditionGroup::All;
    if (normalized == QStringLiteral("any")) return ConditionGroup::Any;
    return std::nullopt;
}

QVariantMap automationToMap(const Automation &automation)
{
    QVariantList conditions;
    for (const auto &condition : automation.conditions) {
        conditions.append(QVariantMap{{QStringLiteral("field"), condition.field},
                                      {QStringLiteral("operation"), condition.operation},
                                      {QStringLiteral("expected"), condition.expected}});
    }
    QVariantList actions;
    for (const auto &action : automation.actions) {
        actions.append(QVariantMap{{QStringLiteral("type"), action.type},
                                   {QStringLiteral("parameters"), action.parameters}});
    }
    return {
        {QStringLiteral("id"), automation.id},
        {QStringLiteral("name"), automation.name},
        {QStringLiteral("enabled"), automation.enabled},
        {QStringLiteral("triggerType"), automation.trigger.type},
        {QStringLiteral("triggerParameters"), automation.trigger.parameters},
        {QStringLiteral("conditionGroup"), conditionGroupName(automation.conditionGroup)},
        {QStringLiteral("conditions"), conditions},
        {QStringLiteral("actions"), actions},
        {QStringLiteral("debounceMs"), automation.debounceMs},
        {QStringLiteral("budgetMs"), automation.budgetMs},
        {QStringLiteral("failureLimit"), automation.failureLimit},
        {QStringLiteral("consecutiveFailures"), automation.consecutiveFailures},
    };
}

Automation automationFromMap(const QVariantMap &map)
{
    Automation automation;
    automation.id = map.value(QStringLiteral("id")).toString().trimmed();
    automation.name = map.value(QStringLiteral("name")).toString().trimmed();
    automation.enabled = map.value(QStringLiteral("enabled"), true).toBool();
    automation.trigger.type = map.value(QStringLiteral("triggerType")).toString().trimmed();
    automation.trigger.parameters = map.value(QStringLiteral("triggerParameters")).toMap();
    automation.conditionGroup =
        conditionGroupFromName(map.value(QStringLiteral("conditionGroup")).toString())
            .value_or(ConditionGroup::All);
    for (const auto &entry : map.value(QStringLiteral("conditions")).toList()) {
        const auto condition = entry.toMap();
        automation.conditions.append(Condition{
            .field = condition.value(QStringLiteral("field")).toString().trimmed(),
            .operation = condition.value(QStringLiteral("operation")).toString().trimmed(),
            .expected = condition.value(QStringLiteral("expected")),
        });
    }
    for (const auto &entry : map.value(QStringLiteral("actions")).toList()) {
        const auto action = entry.toMap();
        automation.actions.append(Action{
            .type = action.value(QStringLiteral("type")).toString().trimmed(),
            .parameters = action.value(QStringLiteral("parameters")).toMap(),
        });
    }
    automation.debounceMs = map.value(QStringLiteral("debounceMs"), 0).toInt();
    automation.budgetMs = map.value(QStringLiteral("budgetMs"), 15000).toInt();
    automation.failureLimit = map.value(QStringLiteral("failureLimit"), 5).toInt();
    automation.consecutiveFailures = map.value(QStringLiteral("consecutiveFailures"), 0).toInt();
    return automation;
}

QVariantMap automationRunToMap(const AutomationRun &run)
{
    QVariantList outcomes;
    for (const auto &outcome : run.outcomes) {
        outcomes.append(QVariantMap{{QStringLiteral("actionType"), outcome.actionType},
                                    {QStringLiteral("accepted"), outcome.accepted},
                                    {QStringLiteral("errorCode"), outcome.errorCode},
                                    {QStringLiteral("message"), outcome.message},
                                    {QStringLiteral("durationMs"), outcome.durationMs}});
    }
    return {
        {QStringLiteral("id"), run.id},
        {QStringLiteral("automationId"), run.automationId},
        {QStringLiteral("correlationId"), run.correlationId},
        {QStringLiteral("status"), run.status},
        {QStringLiteral("reason"), run.reason},
        {QStringLiteral("dryRun"), run.dryRun},
        {QStringLiteral("startedAt"), run.startedAt.toLocalTime()
                                          .toString(QStringLiteral("dd/MM HH:mm:ss"))},
        {QStringLiteral("durationMs"),
         run.startedAt.isValid() && run.finishedAt.isValid()
             ? static_cast<int>(run.startedAt.msecsTo(run.finishedAt)) : 0},
        {QStringLiteral("outcomes"), outcomes},
    };
}

} // namespace churchpresenter
