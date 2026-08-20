#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/AutosaveCoordinator.h"

using namespace churchpresenter;

class AutosaveCoordinatorTest final : public QObject {
    Q_OBJECT

private slots:
    void debouncesChangesAndFlushesOnlyOnce();
};

void AutosaveCoordinatorTest::debouncesChangesAndFlushesOnlyOnce()
{
    int saves = 0;
    AutosaveCoordinator coordinator([&saves] { ++saves; return true; });
    coordinator.setIntervalMs(15);
    QSignalSpy savedSpy(&coordinator, &AutosaveCoordinator::saved);

    coordinator.markDirty();
    coordinator.markDirty();

    QVERIFY(coordinator.dirty());
    QTRY_COMPARE_WITH_TIMEOUT(saves, 1, 250);
    QVERIFY(!coordinator.dirty());
    QCOMPARE(savedSpy.count(), 1);
}

QTEST_MAIN(AutosaveCoordinatorTest)
#include "AutosaveCoordinatorTest.moc"
