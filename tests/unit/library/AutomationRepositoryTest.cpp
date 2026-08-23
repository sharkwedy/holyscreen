#include "library/AutomationRepository.h"
#include "persistence/ApplicationDatabase.h"

#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

namespace {

Automation sample(const QString &id)
{
    Automation automation;
    automation.id = id;
    automation.name = QStringLiteral("Cena de louvor");
    automation.trigger.type = QString::fromLatin1(AutomationTrigger::SongStarted);
    automation.trigger.parameters = {{QStringLiteral("songId"), QStringLiteral("s-1")}};
    automation.conditionGroup = ConditionGroup::Any;
    automation.conditions = {
        Condition{.field = QStringLiteral("event.title"),
                  .operation = QStringLiteral("contains"),
                  .expected = QStringLiteral("Senhor")},
        Condition{.field = QStringLiteral("event.slideIndex"),
                  .operation = QStringLiteral("between"),
                  .expected = QVariantList{1, 4}},
    };
    automation.actions = {
        Action{.type = QStringLiteral("integration"),
               .parameters = {{QStringLiteral("integrationId"), QStringLiteral("obs")},
                              {QStringLiteral("operation"), QStringLiteral("scene.set")},
                              {QStringLiteral("payload"),
                               QVariantMap{{QStringLiteral("sceneName"),
                                            QStringLiteral("Louvor")}}}}},
        Action{.type = QStringLiteral("command"),
               .parameters = {{QStringLiteral("type"), QStringLiteral("presentation.slide.next")}}},
    };
    automation.debounceMs = 750;
    automation.budgetMs = 9000;
    automation.failureLimit = 3;
    return automation;
}

} // namespace

class AutomationRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void savesAndReloadsAnAutomationWithOrderedConditionsAndActions();
    void replacesConditionsAndActionsOnUpdate();
    void updatesOnlyTheRuntimeState();
    void recordsRunsAndPrunesPerAutomation();
    void storesTheAuthorizedExecutables();
    void refusesToOpenADatabaseWithoutTheMigrations();
};

void AutomationRepositoryTest::savesAndReloadsAnAutomationWithOrderedConditionsAndActions()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    AutomationRepository repository(path);
    QVERIFY(repository.open());
    QVERIFY(repository.save(sample(QStringLiteral("a"))));

    AutomationRepository reopened(path);
    QVERIFY(reopened.open());
    const auto stored = reopened.automations();
    QCOMPARE(stored.size(), 1);
    const auto &automation = stored.first();
    QCOMPARE(automation.name, QStringLiteral("Cena de louvor"));
    QCOMPARE(automation.trigger.type, QString::fromLatin1(AutomationTrigger::SongStarted));
    QCOMPARE(automation.trigger.parameters.value(QStringLiteral("songId")).toString(),
             QStringLiteral("s-1"));
    QCOMPARE(automation.conditionGroup, ConditionGroup::Any);
    QCOMPARE(automation.conditions.size(), 2);
    QCOMPARE(automation.conditions.first().field, QStringLiteral("event.title"));
    QCOMPARE(automation.conditions.last().expected.toList(), QVariantList({1, 4}));
    QCOMPARE(automation.actions.size(), 2);
    QCOMPARE(automation.actions.first().type, QStringLiteral("integration"));
    QCOMPARE(automation.actions.first().parameters.value(QStringLiteral("payload")).toMap()
                 .value(QStringLiteral("sceneName")).toString(),
             QStringLiteral("Louvor"));
    QCOMPARE(automation.actions.last().type, QStringLiteral("command"));
    QCOMPARE(automation.debounceMs, 750);
    QCOMPARE(automation.budgetMs, 9000);
    QCOMPARE(automation.failureLimit, 3);
}

void AutomationRepositoryTest::replacesConditionsAndActionsOnUpdate()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    AutomationRepository repository(path);
    QVERIFY(repository.open());
    QVERIFY(repository.save(sample(QStringLiteral("a"))));

    auto updated = sample(QStringLiteral("a"));
    updated.conditions.removeLast();
    updated.actions = {Action{.type = QStringLiteral("wait"),
                              .parameters = {{QStringLiteral("milliseconds"), 250}}}};
    QVERIFY(repository.save(updated));

    const auto stored = repository.automations();
    QCOMPARE(stored.size(), 1);
    QCOMPARE(stored.first().conditions.size(), 1);
    QCOMPARE(stored.first().actions.size(), 1);
    QCOMPARE(stored.first().actions.first().type, QStringLiteral("wait"));

    QVERIFY(repository.remove(QStringLiteral("a")));
    QVERIFY(!repository.remove(QStringLiteral("a")));
    QVERIFY(repository.automations().isEmpty());
}

void AutomationRepositoryTest::updatesOnlyTheRuntimeState()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    AutomationRepository repository(path);
    QVERIFY(repository.open());
    QVERIFY(repository.save(sample(QStringLiteral("a"))));

    QVERIFY(repository.updateRuntimeState(QStringLiteral("a"), false, 3));
    const auto stored = repository.automations().first();
    QVERIFY(!stored.enabled);
    QCOMPARE(stored.consecutiveFailures, 3);
    // A definição continua intacta.
    QCOMPARE(stored.actions.size(), 2);
    QVERIFY(!repository.updateRuntimeState(QStringLiteral("inexistente"), true, 0));
}

void AutomationRepositoryTest::recordsRunsAndPrunesPerAutomation()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    AutomationRepository repository(path);
    QVERIFY(repository.open());
    QVERIFY(repository.save(sample(QStringLiteral("a"))));
    QVERIFY(repository.save(sample(QStringLiteral("b"))));

    const auto start = QDateTime::fromString(QStringLiteral("2026-08-23T19:00:00.000Z"),
                                             Qt::ISODateWithMs);
    for (int index = 0; index < 4; ++index) {
        for (const auto &id : {QStringLiteral("a"), QStringLiteral("b")}) {
            AutomationRun run;
            run.id = QStringLiteral("%1-%2").arg(id).arg(index);
            run.automationId = id;
            run.correlationId = QStringLiteral("origem/%1").arg(id);
            run.status = index % 2 == 0 ? QStringLiteral("completed") : QStringLiteral("failed");
            run.startedAt = start.addSecs(index);
            run.finishedAt = start.addSecs(index).addMSecs(120);
            run.outcomes = {ActionOutcome{.actionType = QStringLiteral("integration"),
                                          .accepted = index % 2 == 0,
                                          .errorCode = QStringLiteral("timeout"),
                                          .message = QStringLiteral("mensagem"),
                                          .durationMs = 42}};
            QVERIFY(repository.recordRun(run));
        }
    }

    const auto runs = repository.runs(QStringLiteral("a"), 10);
    QCOMPARE(runs.size(), 4);
    QCOMPARE(runs.first().id, QStringLiteral("a-3"));
    QCOMPARE(runs.first().outcomes.size(), 1);
    QCOMPARE(runs.first().outcomes.first().durationMs, 42);
    QCOMPARE(repository.runs({}, 100).size(), 8);

    QCOMPARE(repository.pruneRuns(2), 4);
    QCOMPARE(repository.runs(QStringLiteral("a"), 10).size(), 2);
    QCOMPARE(repository.runs(QStringLiteral("b"), 10).size(), 2);
}

void AutomationRepositoryTest::storesTheAuthorizedExecutables()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    AutomationRepository repository(path);
    QVERIFY(repository.open());

    QVERIFY(repository.authorizeExecutable({.canonicalPath = QStringLiteral("/usr/bin/true"),
                                            .label = QStringLiteral("Teste"),
                                            .authorizedAt = QDateTime::currentDateTimeUtc()}));
    QCOMPARE(repository.authorizedExecutables().size(), 1);
    QCOMPARE(repository.authorizedExecutables().first().label, QStringLiteral("Teste"));

    QVERIFY(repository.authorizeExecutable({.canonicalPath = QStringLiteral("/usr/bin/true"),
                                            .label = QStringLiteral("Renomeado")}));
    QCOMPARE(repository.authorizedExecutables().size(), 1);
    QCOMPARE(repository.authorizedExecutables().first().label, QStringLiteral("Renomeado"));

    QVERIFY(repository.revokeExecutable(QStringLiteral("/usr/bin/true")));
    QVERIFY(repository.authorizedExecutables().isEmpty());
}

void AutomationRepositoryTest::refusesToOpenADatabaseWithoutTheMigrations()
{
    QTemporaryDir directory;
    AutomationRepository repository(directory.filePath(QStringLiteral("vazio.db")));
    QTest::ignoreMessage(QtWarningMsg,
                         "The automation tables are missing; run the database migrations first.");
    QVERIFY(!repository.open());
    QVERIFY(repository.automations().isEmpty());
}

QTEST_MAIN(AutomationRepositoryTest)
#include "AutomationRepositoryTest.moc"
