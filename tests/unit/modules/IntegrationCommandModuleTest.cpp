#include "modules/IntegrationCommandModule.h"

#include "core/CommandCatalog.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

namespace {

struct Fixture {
    QVariantMap definition{{QStringLiteral("id"), QStringLiteral("obs")},
                           {QStringLiteral("name"), QStringLiteral("OBS")}};
    QStringList executed;
    QStringList tested;
    QVariantMap result{{QStringLiteral("accepted"), true},
                       {QStringLiteral("message"), QStringLiteral("ok")},
                       {QStringLiteral("durationMs"), 12}};

    IntegrationCommandModule::Actions actions()
    {
        return {
            .execute = [this](const QString &integrationId, const QString &operation,
                              const QVariantMap &payload, const QString &) {
                executed.append(QStringLiteral("%1:%2:%3")
                                    .arg(integrationId, operation,
                                         payload.value(QStringLiteral("sceneName")).toString()));
                return result;
            },
            .test = [this](const QString &integrationId, const QString &) {
                tested.append(integrationId);
                return result;
            },
            .definition = [this](const QString &integrationId) {
                return integrationId == QStringLiteral("obs") ? definition : QVariantMap{};
            },
        };
    }
};

} // namespace

class IntegrationCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void isRegisteredInTheCatalogAsDesktopOnly();
    void testsAndExecutesPublishingSanitizedEvents();
    void rejectsUnknownIntegrationsAndEmptyOperations();
    void reportsFailuresFromTheEngine();
};

void IntegrationCommandModuleTest::isRegisteredInTheCatalogAsDesktopOnly()
{
    QVERIFY(CommandCatalog::contains(QStringLiteral("integration.test")));
    QVERIFY(CommandCatalog::contains(QStringLiteral("integration.execute")));
    // O controle remoto não pode disparar integrações arbitrárias na 0.12.
    QVERIFY(!CommandCatalog::isRemoteAllowed(QStringLiteral("integration.test")));
    QVERIFY(!CommandCatalog::isRemoteAllowed(QStringLiteral("integration.execute")));
}

void IntegrationCommandModuleTest::testsAndExecutesPublishingSanitizedEvents()
{
    CommandBus commands;
    EventBus events;
    Fixture fixture;
    IntegrationCommandModule module(commands, events, fixture.actions());
    QSignalSpy spy(&events, &EventBus::eventPublished);

    QVERIFY(module.requestTest(QStringLiteral("obs")).accepted);
    QCOMPARE(fixture.tested, QStringList{QStringLiteral("obs")});
    QCOMPARE(spy.count(), 1);
    const auto testEvent = spy.takeFirst().first().value<DomainEvent>();
    QCOMPARE(testEvent.type, QStringLiteral("integration.call.finished"));
    QCOMPARE(testEvent.payload.value(QStringLiteral("operation")).toString(),
             QStringLiteral("connection.test"));
    QVERIFY(!testEvent.correlationId.isEmpty());

    QVERIFY(module.requestExecute(QStringLiteral("obs"), QStringLiteral("scene.set"),
                                  {{QStringLiteral("sceneName"), QStringLiteral("Louvor")}})
                .accepted);
    QCOMPARE(fixture.executed, QStringList{QStringLiteral("obs:scene.set:Louvor")});
    QCOMPARE(spy.count(), 1);
    const auto executeEvent = spy.takeFirst().first().value<DomainEvent>();
    QCOMPARE(executeEvent.payload.value(QStringLiteral("integrationId")).toString(),
             QStringLiteral("obs"));
    QCOMPARE(executeEvent.payload.value(QStringLiteral("durationMs")).toInt(), 12);
}

void IntegrationCommandModuleTest::rejectsUnknownIntegrationsAndEmptyOperations()
{
    CommandBus commands;
    EventBus events;
    Fixture fixture;
    IntegrationCommandModule module(commands, events, fixture.actions());

    QCOMPARE(module.requestTest(QStringLiteral("inexistente")).errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestTest(QString{}).errorCode, QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestExecute(QStringLiteral("obs"), QString{}, {}).errorCode,
             QStringLiteral("invalid_payload"));
    QVERIFY(fixture.executed.isEmpty());
    QVERIFY(fixture.tested.isEmpty());
}

void IntegrationCommandModuleTest::reportsFailuresFromTheEngine()
{
    CommandBus commands;
    EventBus events;
    Fixture fixture;
    fixture.result = {{QStringLiteral("accepted"), false},
                      {QStringLiteral("errorCode"), QStringLiteral("timeout")},
                      {QStringLiteral("message"), QStringLiteral("A integração não respondeu.")}};
    IntegrationCommandModule module(commands, events, fixture.actions());

    const auto result = module.requestExecute(QStringLiteral("obs"),
                                              QStringLiteral("recording.start"), {});
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("timeout"));
    QCOMPARE(result.message, QStringLiteral("A integração não respondeu."));
}

QTEST_MAIN(IntegrationCommandModuleTest)
#include "IntegrationCommandModuleTest.moc"
