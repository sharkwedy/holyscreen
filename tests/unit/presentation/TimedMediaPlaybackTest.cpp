#include <QSignalSpy>
#include <QTest>

#include "presentation/TimedMediaPlayback.h"

using namespace churchpresenter;

class TimedMediaPlaybackTest final : public QObject {
    Q_OBJECT

private slots:
    void advancesAndFinishesAtTheConfiguredDuration();
    void pausesResumesAndSeeks();
};

void TimedMediaPlaybackTest::advancesAndFinishesAtTheConfiguredDuration()
{
    TimedMediaPlayback playback;
    QSignalSpy finished(&playback, &TimedMediaPlayback::finished);
    playback.start(120);

    QVERIFY(playback.active());
    QVERIFY(playback.playing());
    QCOMPARE(playback.durationMs(), 120);
    QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 600);
    QVERIFY(!playback.active());
    QCOMPARE(playback.positionMs(), 120);
}

void TimedMediaPlaybackTest::pausesResumesAndSeeks()
{
    TimedMediaPlayback playback;
    playback.start(1000);
    QTest::qWait(60);
    playback.pause();
    const auto pausedAt = playback.positionMs();
    QVERIFY(pausedAt > 0);
    QVERIFY(!playback.playing());
    QTest::qWait(60);
    QCOMPARE(playback.positionMs(), pausedAt);

    playback.seek(800);
    QCOMPARE(playback.positionMs(), 800);
    playback.resume();
    QTest::qWait(40);
    QVERIFY(playback.positionMs() > 800);
    playback.stop();
    QVERIFY(!playback.active());
    QCOMPARE(playback.positionMs(), 0);
}

QTEST_GUILESS_MAIN(TimedMediaPlaybackTest)
#include "TimedMediaPlaybackTest.moc"
