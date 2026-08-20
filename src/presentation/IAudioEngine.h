#pragma once

#include "AudioTypes.h"
#include <QObject>
#include <QUrl>
#include <memory>

namespace churchpresenter {

class IAudioEngine : public QObject {
    Q_OBJECT

public:
    explicit IAudioEngine(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IAudioEngine() = default;

    // Playback control
    virtual void load(const QString &mediaId) = 0;
    virtual void loadFromPath(const QString &path) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    // Position
    virtual void seek(int positionMs) = 0;
    [[nodiscard]] virtual AudioPosition position() const = 0;
    [[nodiscard]] virtual AudioPosition duration() const = 0;

    // Volume
    virtual void setVolume(double volume) = 0; // 0.0 - 1.0
    [[nodiscard]] virtual double volume() const = 0;

    // State
    [[nodiscard]] virtual AudioState state() const = 0;
    [[nodiscard]] virtual AudioError error() const = 0;

    // Media info
    [[nodiscard]] virtual MediaItem currentMedia() const = 0;

signals:
    void stateChanged(AudioState state);
    void positionChanged(int positionMs);
    void durationChanged(int durationMs);
    void errorOccurred(AudioError error);
    void mediaLoaded(const QString &mediaId);
    void mediaMetadataChanged(const churchpresenter::MediaItem &media);
    void playbackStarted();
    void playbackPaused();
    void playbackStopped();
    void playbackFinished();
};

} // namespace churchpresenter
