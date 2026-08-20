#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "modules/UndoCommandModule.h"

using namespace churchpresenter;

class UndoCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void routesUndoAndRedoThroughCommandBus();
};

void UndoCommandModuleTest::routesUndoAndRedoThroughCommandBus()
{
    qRegisterMetaType<DomainEvent>();
    CommandBus commandBus;
    EventBus eventBus;
    UndoManager undoManager;
    UndoCommandModule module(commandBus, eventBus, undoManager);
    int value = 1;
    QVERIFY(undoManager.record(QStringLiteral("Alterar valor"),
                               [&value] { value = 0; return true; },
                               [&value] { value = 1; return true; }));
    QSignalSpy eventSpy(&eventBus, &EventBus::eventPublished);

    const auto undoResult = module.requestUndo();
    const auto redoResult = commandBus.dispatch(Command{
        .id = QStringLiteral("remote-redo-1"),
        .type = QStringLiteral("system.redo"),
        .source = QStringLiteral("remote"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });

    QVERIFY(undoResult.accepted);
    QVERIFY(redoResult.accepted);
    QCOMPARE(value, 1);
    QCOMPARE(eventSpy.count(), 2);
    const auto event = qvariant_cast<DomainEvent>(eventSpy.last().at(0));
    QCOMPARE(event.type, QStringLiteral("system.undo-state.changed"));
    QCOMPARE(event.correlationId, QStringLiteral("remote-redo-1"));
}

QTEST_APPLESS_MAIN(UndoCommandModuleTest)
#include "UndoCommandModuleTest.moc"
