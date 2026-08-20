#include <QtTest/QTest>

#include "presentation/PresentationController.h"

using namespace churchpresenter;

class PresentationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void blackoutRestoresThePresentationThatWasVisible();
    void clockSettingsChangeWithoutChangingWallpaper();
};

void PresentationControllerTest::blackoutRestoresThePresentationThatWasVisible()
{
    PresentationController controller;
    controller.setWallpaperColor(QStringLiteral("#123456"));
    controller.setBlackout(true);

    QVERIFY(controller.blackout());
    QCOMPARE(controller.visibleBackgroundColor(), QStringLiteral("#000000"));

    controller.restorePresentation();

    QVERIFY(!controller.blackout());
    QCOMPARE(controller.visibleBackgroundColor(), QStringLiteral("#123456"));
}

void PresentationControllerTest::clockSettingsChangeWithoutChangingWallpaper()
{
    PresentationController controller;
    controller.setWallpaperColor(QStringLiteral("#aabbcc"));
    controller.setClockVisible(false);

    QVERIFY(!controller.clockVisible());
    QCOMPARE(controller.wallpaperColor(), QStringLiteral("#aabbcc"));
}

QTEST_APPLESS_MAIN(PresentationControllerTest)
#include "PresentationControllerTest.moc"
