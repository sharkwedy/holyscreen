#include <QtTest/QTest>

#include "core/CommandBus.h"
#include "presentation/TextPresentationController.h"

#include <QElapsedTimer>
#include <QImage>

using namespace churchpresenter;

class PerformanceBudgetTest final : public QObject {
    Q_OBJECT

private slots:
    void dispatchesCommandsWithinBudget();
    void changesSlidesWithinBudget();
    void fillsFullHdFramesWithinBudget();
};

void PerformanceBudgetTest::dispatchesCommandsWithinBudget()
{
    CommandBus bus;
    QVERIFY(bus.registerHandler(QStringLiteral("performance.noop"),
                                [](const Command &) { return CommandResult{.accepted = true}; }));
    const Command command{
        .id = QStringLiteral("performance-command"),
        .type = QStringLiteral("performance.noop"),
        .source = QStringLiteral("performance"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    };

    QElapsedTimer timer;
    timer.start();
    for (int index = 0; index < 100'000; ++index) {
        if (!bus.dispatch(command).accepted) QFAIL("CommandBus rejected a valid command");
    }
    const auto elapsed = timer.elapsed();
    qInfo("metric.command_dispatch_100k_ms=%lld", elapsed);
    QVERIFY2(elapsed < 5'000, "100k local commands exceeded the five-second RC budget");
}

void PerformanceBudgetTest::changesSlidesWithinBudget()
{
    Presentation presentation{
        .id = QStringLiteral("performance-presentation"),
        .type = PresentationType::Text,
        .title = QStringLiteral("Performance"),
    };
    for (int index = 0; index < 100; ++index) {
        presentation.slides.append(Slide{
            .id = QStringLiteral("slide-%1").arg(index),
            .label = QString::number(index + 1),
            .text = QStringLiteral("Conteúdo do slide %1").arg(index + 1),
            .order = index,
        });
    }
    TextPresentationController controller;
    controller.setPresentation(presentation);

    QElapsedTimer timer;
    timer.start();
    for (int index = 0; index < 50'000; ++index) {
        if (!controller.show(index % presentation.slides.size()))
            QFAIL("TextPresentationController rejected a valid slide");
    }
    const auto elapsed = timer.elapsed();
    qInfo("metric.slide_changes_50k_ms=%lld", elapsed);
    QVERIFY2(elapsed < 3'000, "50k slide changes exceeded the three-second RC budget");
}

void PerformanceBudgetTest::fillsFullHdFramesWithinBudget()
{
    QImage frame(QSize(1920, 1080), QImage::Format_RGBA8888_Premultiplied);
    QVERIFY(!frame.isNull());

    QElapsedTimer timer;
    timer.start();
    for (int index = 0; index < 120; ++index) {
        frame.fill(QColor::fromRgb(index % 255, (index * 2) % 255, (index * 3) % 255));
    }
    const auto elapsed = timer.elapsed();
    qInfo("metric.full_hd_fills_120_ms=%lld", elapsed);
    QVERIFY2(elapsed < 5'000, "120 Full HD frame fills exceeded the five-second RC budget");
    QVERIFY(frame.pixelColor(0, 0).isValid());
}

QTEST_GUILESS_MAIN(PerformanceBudgetTest)
#include "PerformanceBudgetTest.moc"
