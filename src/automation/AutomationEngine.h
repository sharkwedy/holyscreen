#pragma once

#include "automation/AutomationTypes.h"
#include "automation/ConditionEvaluator.h"

#include <QHash>
#include <QObject>

#include <functional>

namespace churchpresenter {

//! Executa automações locais com limites explícitos: correlação, profundidade
//! de cadeia, número de ações, concorrência, debounce e orçamento de tempo.
//! Nada aqui conhece QML, banco ou protocolo.
class AutomationEngine final : public QObject {
    Q_OBJECT

public:
    struct Limits {
        int maximumChainDepth = 8;
        int maximumActionsPerRun = 20;
        int maximumConcurrentRuns = 10;
    };

    struct Ports {
        //! Despacha um comando do catálogo, propagando a correlação.
        std::function<ActionOutcome(const QString &type, const QVariantMap &payload,
                                    const QString &correlationId)> dispatchCommand;
        std::function<ActionOutcome(const QString &integrationId, const QString &operation,
                                    const QVariantMap &payload, const QString &correlationId)>
            runIntegration;
        std::function<ActionOutcome(const QVariantMap &parameters, const QString &correlationId)>
            runProcess;
        //! Estado atual usado pelas condições.
        std::function<QVariantMap()> state;
    };

    explicit AutomationEngine(QObject *parent = nullptr);

    void setPorts(Ports ports);
    void setLimits(Limits limits);
    [[nodiscard]] Limits limits() const;
    //! Relógio injetável, para tornar debounce e horários determinísticos.
    void setClock(std::function<QDateTime()> clock);
    //! Interruptor global; quando desligado nenhuma automação executa.
    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const;

    void setAutomations(const QList<Automation> &automations);
    [[nodiscard]] QList<Automation> automations() const;
    [[nodiscard]] std::optional<Automation> automation(const QString &id) const;

    //! Reage a um fato do domínio. Devolve as execuções resultantes.
    QList<AutomationRun> handleTrigger(const QString &triggerType, const QVariantMap &payload,
                                       const QString &correlationId = {});
    //! Simula a execução sem tocar em rede, MIDI, OSC, OBS ou processo.
    AutomationRun dryRun(const QString &automationId, const QVariantMap &payload = {});
    //! Reabilita uma automação desativada por falhas consecutivas.
    bool resume(const QString &automationId);

signals:
    void runFinished(const churchpresenter::AutomationRun &run);
    //! Emitido quando o motor desativa uma automação por falhas seguidas.
    void automationDisabled(const QString &automationId, const QString &reason);
    void automationsChanged();

private:
    [[nodiscard]] QString correlationFor(const Automation &automation,
                                         const QString &incoming) const;
    [[nodiscard]] int depthOf(const QString &correlationId) const;
    AutomationRun execute(Automation &automation, const QVariantMap &payload,
                          const QString &correlationId, bool dryRun);
    void registerFailure(Automation &automation, bool failed);

    QList<Automation> m_automations;
    Ports m_ports;
    Limits m_limits;
    std::function<QDateTime()> m_clock;
    QHash<QString, QDateTime> m_lastRun;
    int m_activeRuns = 0;
    bool m_enabled = true;
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::AutomationRun)
