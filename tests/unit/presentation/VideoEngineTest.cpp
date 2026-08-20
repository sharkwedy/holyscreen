#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "presentation/VideoEngine.h"

using namespace churchpresenter;

class VideoEngineTest final : public QObject {
    Q_OBJECT

private slots:
    void reportsMissingFiles();
    void clampsVolumeAndConfiguresLoop();
};

void VideoEngineTest::reportsMissingFiles()
{
    VideoEngine engine;
    QSignalSpy errors(&engine, &VideoEngine::errorOccurred);
    engine.loadFromPath(QStringLiteral("/missing/video.mp4"));
    QCOMPARE(engine.error(), VideoError::FileNotFound);
    QCOMPARE(engine.state(), VideoState::Error);
    QCOMPARE(errors.size(), 1);
}

void VideoEngineTest::clampsVolumeAndConfiguresLoop()
{
    VideoEngine engine;
    engine.setVolume(-1.0);
    QCOMPARE(engine.volume(), 0.0);
    engine.setVolume(2.0);
    QCOMPARE(engine.volume(), 1.0);
    engine.setLoop(true);
    QVERIFY(engine.loop());
    engine.setLoop(false);
    QVERIFY(!engine.loop());
}

QTEST_APPLESS_MAIN(VideoEngineTest)
#include "VideoEngineTest.moc"
