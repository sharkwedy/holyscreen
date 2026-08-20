#include "presentation/OverlayController.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class OverlayControllerTest final : public QObject {
    Q_OBJECT
private slots:
    void managesCommunicationLayers();
    void advancesCountdownAndStopwatchDeterministically();
};

void OverlayControllerTest::managesCommunicationLayers()
{
    OverlayController controller;
    QSignalSpy changed(&controller, &OverlayController::changed);
    controller.setMessage(QStringLiteral("  Portas abertas  "));
    controller.setAlert(QStringLiteral("Atenção"));
    controller.setLowerThird(QStringLiteral("Pr. João"), QStringLiteral("Comunidade Esperança"));
    QCOMPARE(controller.message(), QStringLiteral("Portas abertas"));
    QCOMPARE(controller.alert(), QStringLiteral("Atenção"));
    QCOMPARE(controller.lowerThirdTitle(), QStringLiteral("Pr. João"));
    QCOMPARE(controller.lowerThirdSubtitle(), QStringLiteral("Comunidade Esperança"));
    QCOMPARE(changed.count(), 3);
}

void OverlayControllerTest::advancesCountdownAndStopwatchDeterministically()
{
    OverlayController controller;
    controller.startCountdown(2);
    controller.startStopwatch();
    QCOMPARE(controller.countdownText(), QStringLiteral("00:02"));
    controller.advanceOneSecond();
    QCOMPARE(controller.countdownText(), QStringLiteral("00:01"));
    QCOMPARE(controller.stopwatchText(), QStringLiteral("00:01"));
    controller.advanceOneSecond();
    QCOMPARE(controller.countdownText(), QStringLiteral("00:00"));
    QVERIFY(!controller.countdownRunning());
    QCOMPARE(controller.stopwatchText(), QStringLiteral("00:02"));
    controller.resetStopwatch();
    QCOMPARE(controller.stopwatchText(), QStringLiteral("00:00"));
}

QTEST_APPLESS_MAIN(OverlayControllerTest)
#include "OverlayControllerTest.moc"
