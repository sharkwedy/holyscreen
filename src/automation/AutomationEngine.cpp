#include "automation/AutomationEngine.h"

#include <QElapsedTimer>
#include <QUuid>

#include <algorithm>

namespace churchpresenter {
namespace {

QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ActionOutcome blocked(const QString &type, const QString &code, const QString &message)
{
    return {.actionType = type, .accepted = false, .errorCode = code, .message = message};
}

} // namespace

AutomationEngine::AutomationEngine(QObject *parent)
    : QObject(parent)
    , m_clock([] { return QDateTime::currentDateTimeUtc(); })
{
}

void AutomationEngine::setPorts(Ports ports) { m_ports = std::move(ports); }
void AutomationEngine::setLimits(Limits limits) { m_limits = limits; }
AutomationEngine::Limits AutomationEngine::limits() const { return m_limits; }

void AutomationEngine::setClock(std::function<QDateTime()> clock)
{
    if (clock) m_clock = std::move(clock);
}

void AutomationEngine::setEnabled(bool enabled) { m_enabled = enabled; }
bool AutomationEngine::isEnabled() const { return m_enabled; }

void AutomationEngine::setAutomations(const QList<Automation> &automations)
{
    m_automations = automations;
    emit automationsChanged();
}

QList<Automation> AutomationEngine::automations() const { return m_automations; }

std::optional<Automation> AutomationEngine::automation(const QString &id) const
{
    const auto found = std::find_if(m_automations.cbegin(), m_automations.cend(),
                                    [&id](const Automation &candidate) {
        return candidate.id == id;
    });
    if (found == m_automations.cend()) return std::nullopt;
    return *found;
}

QString AutomationEngine::correlationFor(const Automation &automation,
                                         const QString &incoming) const
{
    const auto root = incoming.isEmpty() ? newId() : incoming;
    return QStringLiteral("%1/%2").arg(root, automation.id);
}

int AutomationEngine::depthOf(const QString &correlationId) const
{
    return static_cast<int>(correlationId.count(QLatin1Char('/')));
}

QList<AutomationRun> AutomationEngine::handleTrigger(const QString &triggerType,
                                                     const QVariantMap &payload,
                                                     const QString &correlationId)
{
    QList<AutomationRun> runs;
    if (!m_enabled) return runs;

    for (auto &automation : m_automations) {
        if (!automation.enabled || automation.trigger.type != triggerType) continue;

        AutomationRun run;
        run.id = newId();
        run.automationId = automation.id;
        run.correlationId = correlationFor(automation, correlationId);
        run.startedAt = m_clock();

        // Uma automação nunca pode reentrar na própria correlação.
        if (correlationId.contains(QStringLiteral("/%1").arg(automation.id))
            || correlationId.endsWith(automation.id)) {
            run.status = QStringLiteral("blocked");
            run.reason = QStringLiteral("A automação já está nesta cadeia de correlação.");
            run.finishedAt = m_clock();
            runs.append(run);
            emit runFinished(run);
            continue;
        }
        if (depthOf(run.correlationId) > m_limits.maximumChainDepth) {
            run.status = QStringLiteral("blocked");
            run.reason = QStringLiteral("Profundidade máxima de cadeia atingida.");
            run.finishedAt = m_clock();
            runs.append(run);
            emit runFinished(run);
            continue;
        }
        if (m_activeRuns >= m_limits.maximumConcurrentRuns) {
            run.status = QStringLiteral("blocked");
            run.reason = QStringLiteral("Limite de execuções simultâneas atingido.");
            run.finishedAt = m_clock();
            runs.append(run);
            emit runFinished(run);
            continue;
        }

        const auto last = m_lastRun.value(automation.id);
        if (automation.debounceMs > 0 && last.isValid()
            && last.msecsTo(run.startedAt) < automation.debounceMs) {
            run.status = QStringLiteral("skipped");
            run.reason = QStringLiteral("Debounce de %1 ms.").arg(automation.debounceMs);
            run.finishedAt = m_clock();
            runs.append(run);
            emit runFinished(run);
            continue;
        }

        const ConditionEvaluator::Context context{
            .event = payload,
            .state = m_ports.state ? m_ports.state() : QVariantMap{},
            .localTime = m_clock().toLocalTime().time(),
        };
        if (!ConditionEvaluator::matches(automation.conditions, automation.conditionGroup,
                                         context)) {
            run.status = QStringLiteral("skipped");
            run.reason = QStringLiteral("As condições não foram satisfeitas.");
            run.finishedAt = m_clock();
            runs.append(run);
            emit runFinished(run);
            continue;
        }

        m_lastRun.insert(automation.id, run.startedAt);
        ++m_activeRuns;
        auto executed = execute(automation, payload, run.correlationId, false);
        --m_activeRuns;
        executed.id = run.id;
        executed.startedAt = run.startedAt;
        runs.append(executed);
        emit runFinished(executed);
    }
    return runs;
}

AutomationRun AutomationEngine::execute(Automation &automation, const QVariantMap &payload,
                                        const QString &correlationId, bool dryRun)
{
    AutomationRun run;
    run.id = newId();
    run.automationId = automation.id;
    run.correlationId = correlationId;
    run.startedAt = m_clock();
    run.dryRun = dryRun;

    QElapsedTimer budget;
    budget.start();
    bool failed = false;

    for (qsizetype index = 0; index < automation.actions.size(); ++index) {
        if (index >= m_limits.maximumActionsPerRun) {
            run.outcomes.append(blocked(QStringLiteral("limit"),
                                        QStringLiteral("action_limit"),
                                        QStringLiteral("Limite de %1 ações por execução.")
                                            .arg(m_limits.maximumActionsPerRun)));
            failed = true;
            break;
        }
        if (!dryRun && automation.budgetMs > 0 && budget.elapsed() > automation.budgetMs) {
            run.outcomes.append(blocked(QStringLiteral("limit"),
                                        QStringLiteral("budget_exhausted"),
                                        QStringLiteral("Orçamento de %1 ms esgotado.")
                                            .arg(automation.budgetMs)));
            failed = true;
            break;
        }

        const auto &action = automation.actions[index];
        QElapsedTimer actionTimer;
        actionTimer.start();
        ActionOutcome outcome{.actionType = action.type};

        if (dryRun) {
            // O ensaio nunca chama rede, MIDI, OSC, OBS ou processo.
            outcome.accepted = true;
            outcome.message = QStringLiteral("Simulada: %1").arg(
                action.parameters.value(QStringLiteral("description"),
                                        action.parameters.value(QStringLiteral("type"),
                                            action.type)).toString());
            run.outcomes.append(outcome);
            continue;
        }

        if (action.type == QLatin1StringView(AutomationAction::Command)) {
            const auto type = action.parameters.value(QStringLiteral("type")).toString();
            outcome = m_ports.dispatchCommand
                ? m_ports.dispatchCommand(type,
                                          action.parameters.value(QStringLiteral("payload")).toMap(),
                                          correlationId)
                : blocked(action.type, QStringLiteral("unavailable"),
                          QStringLiteral("CommandBus indisponível."));
        } else if (action.type == QLatin1StringView(AutomationAction::Integration)) {
            outcome = m_ports.runIntegration
                ? m_ports.runIntegration(
                      action.parameters.value(QStringLiteral("integrationId")).toString(),
                      action.parameters.value(QStringLiteral("operation")).toString(),
                      action.parameters.value(QStringLiteral("payload")).toMap(), correlationId)
                : blocked(action.type, QStringLiteral("unavailable"),
                          QStringLiteral("Motor de integrações indisponível."));
        } else if (action.type == QLatin1StringView(AutomationAction::Process)) {
            outcome = m_ports.runProcess
                ? m_ports.runProcess(action.parameters, correlationId)
                : blocked(action.type, QStringLiteral("process_disabled"),
                          QStringLiteral("Processos externos estão desativados."));
        } else if (action.type == QLatin1StringView(AutomationAction::Wait)) {
            outcome.accepted = true;
            outcome.message = QStringLiteral("Espera de %1 ms")
                                  .arg(action.parameters.value(QStringLiteral("milliseconds")).toInt());
        } else {
            outcome = blocked(action.type, QStringLiteral("unsupported_action"),
                              QStringLiteral("Ação não suportada: %1.").arg(action.type));
        }

        outcome.actionType = action.type;
        if (outcome.durationMs <= 0) outcome.durationMs = static_cast<int>(actionTimer.elapsed());
        run.outcomes.append(outcome);
        if (!outcome.accepted) {
            failed = true;
            break;
        }
    }

    run.finishedAt = m_clock();
    run.status = dryRun ? QStringLiteral("dry-run")
                        : (failed ? QStringLiteral("failed") : QStringLiteral("completed"));
    if (!dryRun) registerFailure(automation, failed);
    return run;
}

void AutomationEngine::registerFailure(Automation &automation, bool failed)
{
    if (!failed) {
        if (automation.consecutiveFailures != 0) {
            automation.consecutiveFailures = 0;
            emit automationsChanged();
        }
        return;
    }
    ++automation.consecutiveFailures;
    if (automation.failureLimit > 0
        && automation.consecutiveFailures >= automation.failureLimit) {
        automation.enabled = false;
        emit automationDisabled(automation.id,
                                QStringLiteral("Desativada após %1 falhas seguidas.")
                                    .arg(automation.consecutiveFailures));
    }
    emit automationsChanged();
}

AutomationRun AutomationEngine::dryRun(const QString &automationId, const QVariantMap &payload)
{
    const auto found = std::find_if(m_automations.begin(), m_automations.end(),
                                    [&automationId](const Automation &candidate) {
        return candidate.id == automationId;
    });
    if (found == m_automations.end()) {
        AutomationRun run;
        run.automationId = automationId;
        run.status = QStringLiteral("blocked");
        run.reason = QStringLiteral("Automação não encontrada.");
        run.startedAt = m_clock();
        run.finishedAt = run.startedAt;
        run.dryRun = true;
        return run;
    }
    auto copy = *found;
    auto run = execute(copy, payload, correlationFor(copy, {}), true);
    const ConditionEvaluator::Context context{
        .event = payload,
        .state = m_ports.state ? m_ports.state() : QVariantMap{},
        .localTime = m_clock().toLocalTime().time(),
    };
    if (!ConditionEvaluator::matches(copy.conditions, copy.conditionGroup, context)) {
        run.reason = QStringLiteral("Com este contexto as condições não passariam.");
    }
    return run;
}

bool AutomationEngine::resume(const QString &automationId)
{
    for (auto &automation : m_automations) {
        if (automation.id != automationId) continue;
        automation.enabled = true;
        automation.consecutiveFailures = 0;
        emit automationsChanged();
        return true;
    }
    return false;
}

} // namespace churchpresenter
