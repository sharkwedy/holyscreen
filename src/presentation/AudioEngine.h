#pragma once

#include "presentation/IAudioEngine.h"

#include <QMediaPlayer>

QT_FORWARD_DECLARE_CLASS(QAudioOutput)

namespace churchpresenter {

class AudioEngine final : public IAudioEngine {
    Q_OBJECT

public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine() override;

    Q_INVOKABLE void load(const QString &mediaId) override;
    Q_INVOKABLE void loadFromPath(const QString &path) override;
    Q_INVOKABLE void play() override;
    Q_INVOKABLE void pause() override;
    Q_INVOKABLE void stop() override;

    Q_INVOKABLE void seek(int positionMs) override;
    [[nodiscard]] AudioPosition position() const override;
    [[nodiscard]] AudioPosition duration() const override;

    void setVolume(double volume) override;
    [[nodiscard]] double volume() const override;

    [[nodiscard]] AudioState state() const override { return m_state; }
    [[nodiscard]] AudioError error() const override { return m_error; }
    [[nodiscard]] MediaItem currentMedia() const override { return m_currentMedia; }

private slots:
    void onStateChanged(QMediaPlayer::PlaybackState);
    void onPositionChanged(qint64);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus);
    void onErrorOccurred(QMediaPlayer::Error, const QString &);

private:
    void setState(AudioState newState);
    void setError(AudioError newError);

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    AudioState m_state = AudioState::Stopped;
    AudioError m_error = AudioError::None;
    AudioPosition m_position;
    AudioPosition m_duration;
    MediaItem m_currentMedia;
    QString m_currentPath;
    QStringList m_mediaPaths;
};

} // namespace churchpresenter
