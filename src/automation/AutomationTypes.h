#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <optional>

namespace churchpresenter {

//! Gatilhos aceitos pelas automações. São sempre fatos do domínio, nunca
//! detalhes de interface.
namespace AutomationTrigger {
inline constexpr auto PresentationStarted = "presentation.started";
inline constexpr auto PresentationStopped = "presentation.stopped";
inline constexpr auto SongStarted = "song.started";
inline constexpr auto MediaStarted = "media.started";
inline constexpr auto MediaPaused = "media.paused";
inline constexpr auto MediaFinished = "media.finished";
inline constexpr auto EventSelected = "event.selected";
inline constexpr auto EventItemExecuted = "event.item.executed";
inline constexpr auto SlideChanged = "slide.changed";
inline constexpr auto LocalTime = "time.local";
inline constexpr auto RemoteCommandAccepted = "remote.command.accepted";
inline constexpr auto TimerStarted = "timer.started";
inline constexpr auto TimerFinished = "timer.finished";
} // namespace AutomationTrigger

//! Ações aceitas. `command.*` chega pela CommandBus, `integration.*` pelo
//! IntegrationEngine e `process.run` pelo executor autorizado.
namespace AutomationAction {
inline constexpr auto Command = "command";
inline constexpr auto Integration = "integration";
inline constexpr auto Process = "process";
inline constexpr auto Wait = "wait";
} // namespace AutomationAction

struct Trigger {
    QString type;
    QVariantMap parameters;
};

struct Condition {
    QString field;
    QString operation;
    QVariant expected;
};

struct Action {
    QString type;
    QVariantMap parameters;
};

enum class ConditionGroup {
    All,
    Any,
};

struct Automation {
    QString id;
    QString name;
    bool enabled = true;
    Trigger trigger;
    ConditionGroup conditionGroup = ConditionGroup::All;
    QList<Condition> conditions;
    QList<Action> actions;
    //! Intervalo mínimo entre duas execuções desta automação.
    int debounceMs = 0;
    //! Orçamento total da execução, incluindo todas as ações.
    int budgetMs = 15000;
    //! Falhas consecutivas que desativam a automação automaticamente.
    int failureLimit = 5;
    int consecutiveFailures = 0;
};

struct ActionOutcome {
    QString actionType;
    bool accepted = false;
    QString errorCode;
    QString message;
    int durationMs = 0;
};

struct AutomationRun {
    QString id;
    QString automationId;
    QString correlationId;
    //! `completed`, `failed`, `skipped`, `dry-run`, `blocked` ou `cancelled`.
    QString status;
    QString reason;
    QDateTime startedAt;
    QDateTime finishedAt;
    QList<ActionOutcome> outcomes;
    bool dryRun = false;
};

[[nodiscard]] QStringList automationTriggerTypes();
[[nodiscard]] QStringList automationActionTypes();
[[nodiscard]] QStringList automationConditionOperations();

[[nodiscard]] QString conditionGroupName(ConditionGroup group);
[[nodiscard]] std::optional<ConditionGroup> conditionGroupFromName(const QString &name);

[[nodiscard]] QVariantMap automationToMap(const Automation &automation);
[[nodiscard]] Automation automationFromMap(const QVariantMap &map);
[[nodiscard]] QVariantMap automationRunToMap(const AutomationRun &run);

} // namespace churchpresenter
