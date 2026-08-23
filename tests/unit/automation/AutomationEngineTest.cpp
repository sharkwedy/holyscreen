#include "automation/AutomationEngine.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

namespace {

struct Recorder {
    QStringList commands;
    QStringList integrations;
    QStringList processes;
    QStringList correlations;
    bool commandAccepted = true;
    bool integrationAccepted = true;

    AutomationEngine::Ports ports()
    {
        return {
            .dispatchCommand = [this](const QString &type, const QVariantMap &,
                                      const QString &correlationId) {
                commands.append(type);
                correlations.append(correlationId);
                return ActionOutcome{.actionType = QStringLiteral("command"),
                                     .accepted = commandAccepted,
                                     .errorCode = commandAccepted ? QString{}
                                                                  : QStringLiteral("rejected")};
            },
            .runIntegration = [this](const QString &integrationId, const QString &operation,
                                     const QVariantMap &, const QString &correlationId) {
                integrations.append(QStringLiteral("%1:%2").arg(integrationId, operation));
                correlations.append(correlationId);
                return ActionOutcome{.actionType = QStringLiteral("integration"),
                                     .accepted = integrationAccepted};
            },
            .runProcess = [this](const QVariantMap &parameters, const QString &) {
                processes.append(parameters.value(QStringLiteral("executable")).toString());
                return ActionOutcome{.actionType = QStringLiteral("process"), .accepted = true};
            },
            .state = [] {
                return QVariantMap{{QStringLiteral("activeOutputs"), 2}};
            },
        };
    }
};

Automation automation(const QString &id, const QString &trigger,
                      const QList<Action> &actions = {})
{
    Automation value;
    value.id = id;
    value.name = QStringLiteral("Automação %1").arg(id);
    value.trigger.type = trigger;
    value.actions = actions.isEmpty()
        ? QList<Action>{Action{.type = QStringLiteral("command"),
                               .parameters = {{QStringLiteral("type"),
                                               QStringLiteral("presentation.blackout.set")}}}}
        : actions;
    return value;
}

} // namespace

class AutomationEngineTest final : public QObject {
    Q_OBJECT

private slots:
    void runsOnlyTheAutomationsOfTheTrigger();
    void skipsWhenConditionsDoNotMatch();
    void propagatesCorrelationAndBlocksReentrancy();
    void enforcesChainDepthActionAndConcurrencyLimits();
    void honoursDebouncePerAutomation();
    void stopsAtTheFirstFailingActionAndDisablesAfterTheLimit();
    void dryRunNeverTouchesTheOutsideWorld();
    void globalSwitchStopsEverything();
};

void AutomationEngineTest::runsOnlyTheAutomationsOfTheTrigger()
{
    AutomationEngine engine;
    Recorder recorder;
    engine.setPorts(recorder.ports());
    engine.setAutomations({
        automation(QStringLiteral("a"), QString::fromLatin1(AutomationTrigger::SlideChanged)),
        automation(QStringLiteral("b"), QString::fromLatin1(AutomationTrigger::MediaStarted),
                   {Action{.type = QStringLiteral("integration"),
                           .parameters = {{QStringLiteral("integrationId"), QStringLiteral("obs")},
                                          {QStringLiteral("operation"),
                                           QStringLiteral("scene.set")}}}}),
        automation(QStringLiteral("desativada"),
                   QString::fromLatin1(AutomationTrigger::SlideChanged)),
    });
    auto list = engine.automations();
    list[2].enabled = false;
    engine.setAutomations(list);

    QSignalSpy spy(&engine, &AutomationEngine::runFinished);
    const auto runs = engine.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged),
                                           {{QStringLiteral("slideIndex"), 2}});

    QCOMPARE(runs.size(), 1);
    QCOMPARE(runs.first().automationId, QStringLiteral("a"));
    QCOMPARE(runs.first().status, QStringLiteral("completed"));
    QCOMPARE(recorder.commands, QStringList{QStringLiteral("presentation.blackout.set")});
    QVERIFY(recorder.integrations.isEmpty());
    QCOMPARE(spy.count(), 1);

    engine.handleTrigger(QString::fromLatin1(AutomationTrigger::MediaStarted), {});
    QCOMPARE(recorder.integrations, QStringList{QStringLiteral("obs:scene.set")});
}

void AutomationEngineTest::skipsWhenConditionsDoNotMatch()
{
    AutomationEngine engine;
    Recorder recorder;
    engine.setPorts(recorder.ports());
    auto rule = automation(QStringLiteral("a"),
                           QString::fromLatin1(AutomationTrigger::SlideChanged));
    rule.conditions = {Condition{.field = QStringLiteral("event.slideIndex"),
                                 .operation = QStringLiteral("greaterThan"),
                                 .expected = 5}};
    engine.setAutomations({rule});

    const auto skipped = engine.handleTrigger(
        QString::fromLatin1(AutomationTrigger::SlideChanged),
        {{QStringLiteral("slideIndex"), 2}});
    QCOMPARE(skipped.first().status, QStringLiteral("skipped"));
    QVERIFY(recorder.commands.isEmpty());

    const auto executed = engine.handleTrigger(
        QString::fromLatin1(AutomationTrigger::SlideChanged),
        {{QStringLiteral("slideIndex"), 9}});
    QCOMPARE(executed.first().status, QStringLiteral("completed"));
    QCOMPARE(recorder.commands.size(), 1);
}

void AutomationEngineTest::propagatesCorrelationAndBlocksReentrancy()
{
    AutomationEngine engine;
    Recorder recorder;
    engine.setPorts(recorder.ports());
    engine.setAutomations({automation(QStringLiteral("luzes"),
                                      QString::fromLatin1(AutomationTrigger::SlideChanged))});

    const auto first = engine.handleTrigger(
        QString::fromLatin1(AutomationTrigger::SlideChanged), {}, QStringLiteral("origem"));
    QCOMPARE(first.first().correlationId, QStringLiteral("origem/luzes"));
    QCOMPARE(recorder.correlations.last(), QStringLiteral("origem/luzes"));

    // O comando disparado pela automação volta como novo gatilho, com a mesma
    // correlação: a automação não pode reentrar.
    const auto reentrant = engine.handleTrigger(
        QString::fromLatin1(AutomationTrigger::SlideChanged), {},
        QStringLiteral("origem/luzes"));
    QCOMPARE(reentrant.first().status, QStringLiteral("blocked"));
    QCOMPARE(recorder.commands.size(), 1);
}

void AutomationEngineTest::enforcesChainDepthActionAndConcurrencyLimits()
{
    AutomationEngine engine;
    Recorder recorder;
    engine.setPorts(recorder.ports());
    engine.setAutomations({automation(QStringLiteral("a"),
                                      QString::fromLatin1(AutomationTrigger::SlideChanged))});

    QStringList deep;
    for (int index = 0; index < engine.limits().maximumChainDepth + 1; ++index) {
        deep.append(QStringLiteral("n%1").arg(index));
    }
    const auto blocked = engine.handleTrigger(
        QString::fromLatin1(AutomationTrigger::SlideChanged), {}, deep.join(QLatin1Char('/')));
    QCOMPARE(blocked.first().status, QStringLiteral("blocked"));
    QVERIFY(blocked.first().reason.contains(QStringLiteral("Profundidade")));

    // Mais ações do que o limite param a execução com erro explícito.
    QList<Action> many;
    for (int index = 0; index < engine.limits().maximumActionsPerRun + 3; ++index) {
        many.append(Action{.type = QStringLiteral("command"),
                           .parameters = {{QStringLiteral("type"),
                                           QStringLiteral("presentation.slide.next")}}});
    }
    engine.setAutomations({automation(QStringLiteral("muitas"),
                                      QString::fromLatin1(AutomationTrigger::MediaStarted), many)});
    const auto run = engine.handleTrigger(QString::fromLatin1(AutomationTrigger::MediaStarted), {});
    QCOMPARE(run.first().status, QStringLiteral("failed"));
    QCOMPARE(recorder.commands.size(), engine.limits().maximumActionsPerRun);
    QCOMPARE(run.first().outcomes.last().errorCode, QStringLiteral("action_limit"));

    // Concorrência: um comando que dispara o mesmo gatilho de outra automação
    // enquanto a primeira executa é barrado pelo limite.
    AutomationEngine concurrent;
    AutomationEngine::Limits limits;
    limits.maximumConcurrentRuns = 1;
    concurrent.setLimits(limits);
    QList<AutomationRun> nested;
    AutomationEngine::Ports reentrantPorts;
    reentrantPorts.dispatchCommand = [&](const QString &, const QVariantMap &, const QString &) {
        nested = concurrent.handleTrigger(QString::fromLatin1(AutomationTrigger::MediaStarted), {},
                                          QStringLiteral("outra"));
        return ActionOutcome{.accepted = true};
    };
    concurrent.setPorts(reentrantPorts);
    concurrent.setAutomations({
        automation(QStringLiteral("a"), QString::fromLatin1(AutomationTrigger::SlideChanged)),
        automation(QStringLiteral("b"), QString::fromLatin1(AutomationTrigger::MediaStarted)),
    });
    concurrent.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged), {});
    QCOMPARE(nested.size(), 1);
    QCOMPARE(nested.first().status, QStringLiteral("blocked"));
    QVERIFY(nested.first().reason.contains(QStringLiteral("simultâneas")));
}

void AutomationEngineTest::honoursDebouncePerAutomation()
{
    AutomationEngine engine;
    Recorder recorder;
    engine.setPorts(recorder.ports());
    auto now = QDateTime::fromString(QStringLiteral("2026-08-23T19:00:00.000Z"),
                                     Qt::ISODateWithMs);
    engine.setClock([&now] { return now; });

    auto rule = automation(QStringLiteral("a"),
                           QString::fromLatin1(AutomationTrigger::SlideChanged));
    rule.debounceMs = 1000;
    engine.setAutomations({rule});

    QCOMPARE(engine.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged), {})
                 .first().status,
             QStringLiteral("completed"));

    now = now.addMSecs(400);
    const auto debounced = engine.handleTrigger(
        QString::fromLatin1(AutomationTrigger::SlideChanged), {});
    QCOMPARE(debounced.first().status, QStringLiteral("skipped"));
    QVERIFY(debounced.first().reason.contains(QStringLiteral("Debounce")));

    now = now.addMSecs(900);
    QCOMPARE(engine.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged), {})
                 .first().status,
             QStringLiteral("completed"));
    QCOMPARE(recorder.commands.size(), 2);
}

void AutomationEngineTest::stopsAtTheFirstFailingActionAndDisablesAfterTheLimit()
{
    AutomationEngine engine;
    Recorder recorder;
    recorder.commandAccepted = false;
    engine.setPorts(recorder.ports());

    auto rule = automation(QStringLiteral("a"),
                           QString::fromLatin1(AutomationTrigger::SlideChanged),
                           {Action{.type = QStringLiteral("command"),
                                   .parameters = {{QStringLiteral("type"),
                                                   QStringLiteral("presentation.slide.next")}}},
                            Action{.type = QStringLiteral("integration"),
                                   .parameters = {{QStringLiteral("integrationId"),
                                                   QStringLiteral("obs")}}}});
    rule.failureLimit = 2;
    engine.setAutomations({rule});
    QSignalSpy disabledSpy(&engine, &AutomationEngine::automationDisabled);

    const auto first = engine.handleTrigger(
        QString::fromLatin1(AutomationTrigger::SlideChanged), {});
    QCOMPARE(first.first().status, QStringLiteral("failed"));
    // A segunda ação não roda depois da falha.
    QVERIFY(recorder.integrations.isEmpty());
    QCOMPARE(disabledSpy.count(), 0);

    engine.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged), {});
    QCOMPARE(disabledSpy.count(), 1);
    QVERIFY(!engine.automation(QStringLiteral("a"))->enabled);

    // Automação desativada não executa mais nada até ser retomada.
    QVERIFY(engine.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged), {})
                .isEmpty());
    QVERIFY(engine.resume(QStringLiteral("a")));
    QVERIFY(engine.automation(QStringLiteral("a"))->enabled);
    QCOMPARE(engine.automation(QStringLiteral("a"))->consecutiveFailures, 0);
}

void AutomationEngineTest::dryRunNeverTouchesTheOutsideWorld()
{
    AutomationEngine engine;
    Recorder recorder;
    engine.setPorts(recorder.ports());
    auto rule = automation(QStringLiteral("a"),
                           QString::fromLatin1(AutomationTrigger::SlideChanged),
                           {Action{.type = QStringLiteral("integration"),
                                   .parameters = {{QStringLiteral("integrationId"),
                                                   QStringLiteral("obs")},
                                                  {QStringLiteral("operation"),
                                                   QStringLiteral("recording.start")}}},
                            Action{.type = QStringLiteral("process"),
                                   .parameters = {{QStringLiteral("executable"),
                                                   QStringLiteral("/bin/echo")}}}});
    rule.conditions = {Condition{.field = QStringLiteral("event.slideIndex"),
                                 .operation = QStringLiteral("greaterThan"),
                                 .expected = 50}};
    engine.setAutomations({rule});

    const auto run = engine.dryRun(QStringLiteral("a"), {{QStringLiteral("slideIndex"), 1}});
    QCOMPARE(run.status, QStringLiteral("dry-run"));
    QVERIFY(run.dryRun);
    QCOMPARE(run.outcomes.size(), 2);
    QVERIFY(recorder.integrations.isEmpty());
    QVERIFY(recorder.processes.isEmpty());
    QVERIFY(recorder.commands.isEmpty());
    // O ensaio avisa quando as condições não passariam com o contexto atual.
    QVERIFY(run.reason.contains(QStringLiteral("condições")));

    const auto missing = engine.dryRun(QStringLiteral("inexistente"));
    QCOMPARE(missing.status, QStringLiteral("blocked"));
}

void AutomationEngineTest::globalSwitchStopsEverything()
{
    AutomationEngine engine;
    Recorder recorder;
    engine.setPorts(recorder.ports());
    engine.setAutomations({automation(QStringLiteral("a"),
                                      QString::fromLatin1(AutomationTrigger::SlideChanged))});

    engine.setEnabled(false);
    QVERIFY(engine.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged), {})
                .isEmpty());
    QVERIFY(recorder.commands.isEmpty());

    engine.setEnabled(true);
    QCOMPARE(engine.handleTrigger(QString::fromLatin1(AutomationTrigger::SlideChanged), {}).size(),
             1);
}

QTEST_MAIN(AutomationEngineTest)
#include "AutomationEngineTest.moc"
