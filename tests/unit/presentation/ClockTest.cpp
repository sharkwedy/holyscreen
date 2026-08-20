#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "presentation/Clock.h"

using namespace churchpresenter;

class FixedClock final : public IClock {
public:
    QDateTime value;
    QDateTime now() const override { return value; }
};

class ClockTest final : public QObject {
    Q_OBJECT

private slots:
    void formatsClockWithoutReadingSystemTime();
    void crossesMidnightDeterministically();
};

void ClockTest::formatsClockWithoutReadingSystemTime()
{
    auto fixed = std::make_unique<FixedClock>();
    fixed->value = QDateTime(QDate(2026, 8, 13), QTime(19, 30, 17));
    auto *clockValue = fixed.get();
    ClockController controller(std::move(fixed));

    QCOMPARE(controller.text(), QStringLiteral("19:30"));
    controller.setFormat(QStringLiteral("24h-seconds"));
    QCOMPARE(controller.text(), QStringLiteral("19:30:17"));
    controller.setFormat(QStringLiteral("12h"));
    QCOMPARE(controller.text(), QStringLiteral("07:30 PM"));
    QVERIFY(clockValue != nullptr);
}

void ClockTest::crossesMidnightDeterministically()
{
    auto fixed = std::make_unique<FixedClock>();
    fixed->value = QDateTime(QDate(2026, 8, 13), QTime(23, 59));
    auto *clockValue = fixed.get();
    ClockController controller(std::move(fixed));
    QSignalSpy changed(&controller, &ClockController::textChanged);

    clockValue->value = QDateTime(QDate(2026, 8, 14), QTime(0, 0));
    controller.refresh();

    QCOMPARE(controller.text(), QStringLiteral("00:00"));
    QCOMPARE(changed.size(), 1);
}

QTEST_APPLESS_MAIN(ClockTest)
#include "ClockTest.moc"
