#pragma once

#include "VideoTypes.h"
#include <QObject>
#include <memory>

namespace churchpresenter {

class IVideoEngine : public QObject {
    Q_OBJECT

public:
    explicit IVideoEngine(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IVideoEngine() = default;

    // Playback control
    virtual void load(const QString &mediaId) = 0;
    virtual void loadFromPath(const QString &path) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    // Position
    virtual void seek(int positionMs) = 0;
    [[nodiscard]] virtual VideoPosition position() const = 0;
    [[nodiscard]] virtual VideoPosition duration() const = 0;

    // Volume
    virtual void setVolume(double volume) = 0;
    [[nodiscard]] virtual double volume() const = 0;
    virtual void setLoop(bool loop) = 0;
    [[nodiscard]] virtual bool loop() const = 0;

    // State
    [[nodiscard]] virtual VideoState state() const = 0;
    [[nodiscard]] virtual VideoError error() const = 0;
    [[nodiscard]] virtual MediaItem currentMedia() const = 0;

signals:
    void stateChanged(VideoState state);
    void positionChanged(int positionMs);
    void durationChanged(int durationMs);
    void errorOccurred(VideoError error);
    void mediaLoaded(const QString &mediaId);
    void mediaMetadataChanged(const churchpresenter::MediaItem &media);
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void playbackFinished();
    void frameReady(const VideoFrame &frame);
};

} // namespace churchpresenter
