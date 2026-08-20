#include "presentation/ImagePresentationController.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class ImagePresentationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void selectsAndNavigatesCircularPlaylist()
    {
        ImagePresentationController controller;
        controller.setPlaylist({
            MediaItem{.id = QStringLiteral("one"), .type = MediaType::Image, .title = QStringLiteral("One"), .path = QStringLiteral("/one.png")},
            MediaItem{.id = QStringLiteral("two"), .type = MediaType::Image, .title = QStringLiteral("Two"), .path = QStringLiteral("/two.png")},
        });

        QVERIFY(controller.show(QStringLiteral("one")));
        QCOMPARE(controller.current().id, QStringLiteral("one"));
        QVERIFY(controller.visible());

        controller.next();
        QCOMPARE(controller.current().id, QStringLiteral("two"));
        controller.next();
        QCOMPARE(controller.current().id, QStringLiteral("one"));
        controller.previous();
        QCOMPARE(controller.current().id, QStringLiteral("two"));
    }

    void rejectsUnknownAndStopsCleanly()
    {
        ImagePresentationController controller;
        controller.setPlaylist({MediaItem{.id = QStringLiteral("known"), .type = MediaType::Image}});

        QVERIFY(!controller.show(QStringLiteral("missing")));
        QVERIFY(!controller.visible());
        QVERIFY(controller.show(QStringLiteral("known")));
        controller.stop();
        QVERIFY(!controller.visible());
        QCOMPARE(controller.current().id, QStringLiteral("known"));
    }

    void autoplayAdvancesUsingConfiguredInterval()
    {
        ImagePresentationController controller;
        controller.setPlaylist({
            MediaItem{.id = QStringLiteral("one"), .type = MediaType::Image},
            MediaItem{.id = QStringLiteral("two"), .type = MediaType::Image},
        });
        controller.setAutoplayIntervalMs(20);
        controller.setAutoplay(true);
        QVERIFY(controller.show(QStringLiteral("one")));

        QCOMPARE(controller.autoplayIntervalMs(), 250);
        QTRY_COMPARE_WITH_TIMEOUT(controller.current().id, QStringLiteral("two"), 1000);
        QVERIFY(controller.autoplay());
        controller.stop();
        QVERIFY(!controller.autoplayTimerActive());
    }

    void normalizesPresentationOptions()
    {
        ImagePresentationController controller;
        controller.setFit(ImageFit::Stretch);
        controller.setTransition(ImageTransition::Fade);
        controller.setAutoplayIntervalMs(50);

        QCOMPARE(controller.fit(), ImageFit::Stretch);
        QCOMPARE(controller.transition(), ImageTransition::Fade);
        QCOMPARE(controller.autoplayIntervalMs(), 250);
    }
};

QTEST_MAIN(ImagePresentationControllerTest)
#include "ImagePresentationControllerTest.moc"
