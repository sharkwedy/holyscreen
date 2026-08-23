#include "library/IntegrationRepository.h"
#include "persistence/ApplicationDatabase.h"

#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

namespace {

IntegrationDefinition definition(const QString &id, IntegrationType type = IntegrationType::Http)
{
    return IntegrationDefinition{
        .id = id,
        .name = QStringLiteral("Integração %1").arg(id),
        .type = type,
        .enabled = true,
        .configuration = {{QStringLiteral("url"), QStringLiteral("https://exemplo.local")},
                          {QStringLiteral("headers"),
                           QVariantMap{{QStringLiteral("Authorization"),
                                        QStringLiteral("%1/token").arg(id)}}}},
        .secretReferences = {QStringLiteral("%1/token").arg(id)},
        .timeoutMs = 2500,
        .retryPolicy = {.maximumAttempts = 2, .backoffMs = 100},
    };
}

} // namespace

class IntegrationRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void savesAndReloadsDefinitionsWithTheirConfiguration();
    void removingAnIntegrationAlsoRemovesItsHistory();
    void keepsTheMostRecentCallsPerIntegrationWhenPruning();
    void refusesToOpenADatabaseWithoutTheMigrations();
};

void IntegrationRepositoryTest::savesAndReloadsDefinitionsWithTheirConfiguration()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);

    IntegrationRepository repository(path);
    QVERIFY(repository.open());
    QVERIFY(repository.save(definition(QStringLiteral("obs"), IntegrationType::Obs)));
    QVERIFY(repository.save(definition(QStringLiteral("hook"))));

    IntegrationRepository reopened(path);
    QVERIFY(reopened.open());
    const auto stored = reopened.definitions();
    QCOMPARE(stored.size(), 2);
    const auto obs = std::find_if(stored.cbegin(), stored.cend(),
                                  [](const IntegrationDefinition &candidate) {
        return candidate.id == QStringLiteral("obs");
    });
    QVERIFY(obs != stored.cend());
    QCOMPARE(obs->type, IntegrationType::Obs);
    QCOMPARE(obs->timeoutMs, 2500);
    QCOMPARE(obs->retryPolicy.maximumAttempts, 2);
    QCOMPARE(obs->retryPolicy.backoffMs, 100);
    QCOMPARE(obs->secretReferences, QStringList{QStringLiteral("obs/token")});
    QCOMPARE(obs->configuration.value(QStringLiteral("headers")).toMap()
                 .value(QStringLiteral("Authorization")).toString(),
             QStringLiteral("obs/token"));

    auto updated = definition(QStringLiteral("obs"), IntegrationType::Obs);
    updated.enabled = false;
    updated.name = QStringLiteral("OBS do palco");
    QVERIFY(repository.save(updated));
    QCOMPARE(repository.definitions().size(), 2);
    QVERIFY(!repository.definitions().first().enabled
            || !repository.definitions().last().enabled);
}

void IntegrationRepositoryTest::removingAnIntegrationAlsoRemovesItsHistory()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    IntegrationRepository repository(path);
    QVERIFY(repository.open());
    QVERIFY(repository.save(definition(QStringLiteral("hook"))));

    QVERIFY(repository.recordCall(IntegrationCall{
        .id = QStringLiteral("call-1"),
        .integrationId = QStringLiteral("hook"),
        .operation = QStringLiteral("request.send"),
        .accepted = true,
        .durationMs = 42,
        .occurredAt = QDateTime::currentDateTimeUtc(),
    }));
    QCOMPARE(repository.history(QStringLiteral("hook"), 10).size(), 1);
    QCOMPARE(repository.history({}, 10).size(), 1);

    QVERIFY(repository.remove(QStringLiteral("hook")));
    QVERIFY(!repository.remove(QStringLiteral("hook")));
    QVERIFY(repository.definitions().isEmpty());
    QVERIFY(repository.history({}, 10).isEmpty());
}

void IntegrationRepositoryTest::keepsTheMostRecentCallsPerIntegrationWhenPruning()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    IntegrationRepository repository(path);
    QVERIFY(repository.open());
    QVERIFY(repository.save(definition(QStringLiteral("hook"))));
    QVERIFY(repository.save(definition(QStringLiteral("obs"), IntegrationType::Obs)));

    const auto start = QDateTime::fromString(QStringLiteral("2026-08-23T10:00:00.000Z"),
                                             Qt::ISODateWithMs);
    for (int index = 0; index < 5; ++index) {
        for (const auto &integration : {QStringLiteral("hook"), QStringLiteral("obs")}) {
            QVERIFY(repository.recordCall(IntegrationCall{
                .id = QStringLiteral("%1-%2").arg(integration).arg(index),
                .integrationId = integration,
                .operation = QStringLiteral("request.send"),
                .accepted = index % 2 == 0,
                .durationMs = index,
                .occurredAt = start.addSecs(index),
            }));
        }
    }

    QCOMPARE(repository.pruneHistory(2), 6);
    QCOMPARE(repository.history(QStringLiteral("hook"), 10).size(), 2);
    QCOMPARE(repository.history(QStringLiteral("obs"), 10).size(), 2);
    // Os mais recentes permanecem.
    QCOMPARE(repository.history(QStringLiteral("hook"), 10).first().id,
             QStringLiteral("hook-4"));
    QCOMPARE(repository.pruneHistory(2), 0);
}

void IntegrationRepositoryTest::refusesToOpenADatabaseWithoutTheMigrations()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("empty.db"));
    IntegrationRepository repository(path);
    QTest::ignoreMessage(QtWarningMsg,
                         "The integration tables are missing; run the database migrations first.");
    QVERIFY(!repository.open());
    QVERIFY(repository.definitions().isEmpty());
}

QTEST_MAIN(IntegrationRepositoryTest)
#include "IntegrationRepositoryTest.moc"
