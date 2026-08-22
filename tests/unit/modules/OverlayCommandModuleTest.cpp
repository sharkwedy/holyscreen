#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "modules/OverlayCommandModule.h"

using namespace churchpresenter;

class OverlayCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void routesOperatorAndRemoteActionsThroughTheSameCommands();
    void overlayMessagesAreUndoable();
};

void OverlayCommandModuleTest::routesOperatorAndRemoteActionsThroughTheSameCommands()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();
    CommandBus commandBus;
    EventBus eventBus;
    OverlayController overlays;
    OverlayCommandModule module(commandBus, eventBus, overlays);
    QSignalSpy eventSpy(&eventBus, &EventBus::eventPublished);

    const auto messageResult = module.requestAudienceMessage(
        QStringLiteral("  Recepção no salão  "), QStringLiteral("operator"));
    const auto countdownResult = commandBus.dispatch(Command{
        .id = QStringLiteral("remote-command-1"),
        .type = QStringLiteral("timer.countdown.start"),
        .payload = {{QStringLiteral("seconds"), 90}},
        .source = QStringLiteral("remote"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });

    QVERIFY(messageResult.accepted);
    QVERIFY(countdownResult.accepted);
    QCOMPARE(overlays.message(), QStringLiteral("Recepção no salão"));
    QVERIFY(overlays.countdownRunning());
    QCOMPARE(overlays.countdownText(), QStringLiteral("01:30"));
    QCOMPARE(eventSpy.count(), 2);
    const auto remoteEvent = qvariant_cast<DomainEvent>(eventSpy.last().at(0));
    QCOMPARE(remoteEvent.type, QStringLiteral("overlay.state.changed"));
    QCOMPARE(remoteEvent.correlationId, QStringLiteral("remote-command-1"));
}

void OverlayCommandModuleTest::overlayMessagesAreUndoable()
{
    CommandBus commandBus;
    EventBus eventBus;
    UndoManager undo;
    OverlayController overlays;
    OverlayCommandModule module(commandBus, eventBus, overlays, &undo);

    QVERIFY(module.requestAudienceMessage(QStringLiteral("Primeiro aviso")).accepted);
    QVERIFY(module.requestAudienceMessage(QStringLiteral("Segundo aviso")).accepted);
    QCOMPARE(overlays.message(), QStringLiteral("Segundo aviso"));
    QVERIFY(undo.canUndo());
    QVERIFY(undo.undo().success);
    QCOMPARE(overlays.message(), QStringLiteral("Primeiro aviso"));
    QVERIFY(undo.redo().success);
    QCOMPARE(overlays.message(), QStringLiteral("Segundo aviso"));
}

QTEST_APPLESS_MAIN(OverlayCommandModuleTest)
#include "OverlayCommandModuleTest.moc"
