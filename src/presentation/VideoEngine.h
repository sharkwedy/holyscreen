#pragma once

#include "presentation/IVideoEngine.h"
#include "presentation/VideoFrameBus.h"

#include <QMediaPlayer>

QT_FORWARD_DECLARE_CLASS(QAudioOutput)

namespace churchpresenter {

class VideoEngine final : public IVideoEngine {
    Q_OBJECT

public:
    explicit VideoEngine(QObject *parent = nullptr);
    ~VideoEngine() override;

    Q_INVOKABLE void load(const QString &mediaId) override;
    Q_INVOKABLE void loadFromPath(const QString &path) override;
    Q_INVOKABLE void play() override;
    Q_INVOKABLE void pause() override;
    Q_INVOKABLE void stop() override;
    Q_INVOKABLE void seek(int positionMs) override;

    [[nodiscard]] VideoPosition position() const override;
    [[nodiscard]] VideoPosition duration() const override;
    void setVolume(double volume) override;
    [[nodiscard]] double volume() const override;
    void setLoop(bool loop) override;
    [[nodiscard]] bool loop() const override;
    [[nodiscard]] VideoState state() const override { return m_state; }
    [[nodiscard]] VideoError error() const override { return m_error; }
    [[nodiscard]] MediaItem currentMedia() const override { return m_currentMedia; }
    [[nodiscard]] VideoFrameBus &frameBus() { return m_frameBus; }

private slots:
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onPositionChanged(qint64 position);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onErrorOccurred(QMediaPlayer::Error error, const QString &message);

private:
    void setState(VideoState state);
    void setError(VideoError error);

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    VideoFrameBus m_frameBus;
    VideoState m_state = VideoState::Stopped;
    VideoError m_error = VideoError::None;
    VideoPosition m_position;
    VideoPosition m_duration;
    MediaItem m_currentMedia;
    bool m_loop = false;
};

} // namespace churchpresenter
