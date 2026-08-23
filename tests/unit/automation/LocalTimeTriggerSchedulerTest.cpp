#include "automation/LocalTimeTriggerScheduler.h"

#include <QSignalSpy>
#include <QTest>
#include <QTimeZone>

using namespace churchpresenter;

class LocalTimeTriggerSchedulerTest final : public QObject {
    Q_OBJECT

private slots:
    void emitsOnlyOnceForEachLocalMinuteOccurrence();
};

void LocalTimeTriggerSchedulerTest::emitsOnlyOnceForEachLocalMinuteOccurrence()
{
    LocalTimeTriggerScheduler scheduler;
    auto now = QDateTime(QDate(2026, 8, 23), QTime(19, 45, 1),
                         QTimeZone::systemTimeZone());
    scheduler.setClock([&now] { return now; });
    QSignalSpy spy(&scheduler, &LocalTimeTriggerScheduler::localTimeOccurred);

    scheduler.checkNow();
    QCOMPARE(spy.count(), 1);
    const auto first = spy.first().first().toMap();
    QCOMPARE(first.value(QStringLiteral("localTime")).toString(), QStringLiteral("19:45"));
    QCOMPARE(first.value(QStringLiteral("dayOfWeek")).toInt(), 7);

    now = now.addSecs(40);
    scheduler.checkNow();
    QCOMPARE(spy.count(), 1);

    now = now.addSecs(20);
    scheduler.checkNow();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.last().first().toMap().value(QStringLiteral("localTime")).toString(),
             QStringLiteral("19:46"));
}

QTEST_APPLESS_MAIN(LocalTimeTriggerSchedulerTest)
#include "LocalTimeTriggerSchedulerTest.moc"
