#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "presentation/AudioEngine.h"

#include <QFile>
#include <QTemporaryDir>

using namespace churchpresenter;

class AudioEngineTest final : public QObject {
    Q_OBJECT

private slots:
    void reportsMissingFilesWithoutStartingPlayback();
    void reportsFilesRemovedBeforePlayback();
    void mapsMissingCodecsToUnsupportedFormat();
    void clampsVolumeToSupportedRange();
};

void AudioEngineTest::reportsMissingFilesWithoutStartingPlayback()
{
    AudioEngine engine;
    QSignalSpy errors(&engine, &AudioEngine::errorOccurred);

    engine.loadFromPath(QStringLiteral("/path/that/does/not/exist.mp3"));

    QCOMPARE(engine.error(), AudioError::FileNotFound);
    QCOMPARE(engine.state(), AudioState::Error);
    QCOMPARE(errors.size(), 1);
}

void AudioEngineTest::reportsFilesRemovedBeforePlayback()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("removed.mp3"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("placeholder") > 0);
    file.close();

    AudioEngine engine;
    engine.loadFromPath(path);
    QVERIFY(QFile::remove(path));
    QSignalSpy errors(&engine, &AudioEngine::errorOccurred);

    engine.play();

    QCOMPARE(engine.error(), AudioError::FileNotFound);
    QCOMPARE(engine.state(), AudioState::Error);
    QCOMPARE(errors.size(), 1);
}

void AudioEngineTest::mapsMissingCodecsToUnsupportedFormat()
{
    AudioEngine engine;
    QVERIFY(QMetaObject::invokeMethod(&engine, "onErrorOccurred", Qt::DirectConnection,
                                      Q_ARG(QMediaPlayer::Error, QMediaPlayer::FormatError),
                                      Q_ARG(QString, QStringLiteral("codec unavailable"))));
    QCOMPARE(engine.error(), AudioError::UnsupportedFormat);
    QCOMPARE(engine.state(), AudioState::Error);
}

void AudioEngineTest::clampsVolumeToSupportedRange()
{
    AudioEngine engine;

    engine.setVolume(-1.0);
    QCOMPARE(engine.volume(), 0.0);
    engine.setVolume(2.0);
    QCOMPARE(engine.volume(), 1.0);
}

QTEST_APPLESS_MAIN(AudioEngineTest)
#include "AudioEngineTest.moc"
