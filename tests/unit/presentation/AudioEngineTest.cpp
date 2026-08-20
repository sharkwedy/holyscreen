#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "presentation/AudioEngine.h"

using namespace churchpresenter;

class AudioEngineTest final : public QObject {
    Q_OBJECT

private slots:
    void reportsMissingFilesWithoutStartingPlayback();
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
