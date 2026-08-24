#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "presentation/VideoEngine.h"

#include <QFile>
#include <QTemporaryDir>

using namespace churchpresenter;

class VideoEngineTest final : public QObject {
    Q_OBJECT

private slots:
    void reportsMissingFiles();
    void reportsFilesRemovedBeforePlayback();
    void mapsMissingCodecsToUnsupportedFormat();
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

void VideoEngineTest::reportsFilesRemovedBeforePlayback()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("removed.mp4"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("placeholder") > 0);
    file.close();

    VideoEngine engine;
    engine.loadFromPath(path);
    QVERIFY(QFile::remove(path));
    QSignalSpy errors(&engine, &VideoEngine::errorOccurred);

    engine.play();

    QCOMPARE(engine.error(), VideoError::FileNotFound);
    QCOMPARE(engine.state(), VideoState::Error);
    QCOMPARE(errors.size(), 1);
}

void VideoEngineTest::mapsMissingCodecsToUnsupportedFormat()
{
    VideoEngine engine;
    QVERIFY(QMetaObject::invokeMethod(&engine, "onErrorOccurred", Qt::DirectConnection,
                                      Q_ARG(QMediaPlayer::Error, QMediaPlayer::FormatError),
                                      Q_ARG(QString, QStringLiteral("codec unavailable"))));
    QCOMPARE(engine.error(), VideoError::UnsupportedFormat);
    QCOMPARE(engine.state(), VideoState::Error);
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
