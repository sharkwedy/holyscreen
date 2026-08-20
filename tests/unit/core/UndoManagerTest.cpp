#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "core/UndoManager.h"

using namespace churchpresenter;

class UndoManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void undoesAndRedoesRecordedOperation();
};

void UndoManagerTest::undoesAndRedoesRecordedOperation()
{
    UndoManager manager;
    QSignalSpy stateSpy(&manager, &UndoManager::stateChanged);
    int value = 1;

    QVERIFY(manager.record(QStringLiteral("Ativar blackout"),
                           [&value] { value = 0; return true; },
                           [&value] { value = 1; return true; }));

    QVERIFY(manager.canUndo());
    QVERIFY(!manager.canRedo());
    QCOMPARE(manager.undoLabel(), QStringLiteral("Ativar blackout"));
    const auto undoResult = manager.undo();
    QVERIFY2(undoResult.success, qPrintable(undoResult.error));
    QCOMPARE(value, 0);
    QVERIFY(!manager.canUndo());
    QVERIFY(manager.canRedo());

    const auto redoResult = manager.redo();
    QVERIFY2(redoResult.success, qPrintable(redoResult.error));
    QCOMPARE(value, 1);
    QVERIFY(manager.canUndo());
    QVERIFY(!manager.canRedo());
    QCOMPARE(stateSpy.count(), 3);
}

QTEST_APPLESS_MAIN(UndoManagerTest)
#include "UndoManagerTest.moc"
