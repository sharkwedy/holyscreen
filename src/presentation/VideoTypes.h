#pragma once

#include "presentation/MediaTypes.h"

#include <QString>
#include <QMetaType>

namespace churchpresenter {

enum class VideoState {
    Stopped,
    Loading,
    Ready,
    Playing,
    Paused,
    Buffering,
    Error
};

enum class VideoError {
    None,
    FileNotFound,
    ReadError,
    UnsupportedFormat,
    DecodeError,
    DeviceError,
    EndOfStream
};

struct VideoFrame {
    Q_GADGET
public:
    quint32 frameNumber = 0;
    qint64 timestampMs = 0;
    QString mediaId;

    Q_PROPERTY(quint32 frameNumber MEMBER frameNumber)
    Q_PROPERTY(qint64 timestampMs MEMBER timestampMs)
    Q_PROPERTY(QString mediaId MEMBER mediaId)
};

struct VideoPosition {
    int seconds = 0;
    int milliseconds = 0;

    VideoPosition() = default;
    VideoPosition(int sec, int ms = 0) : seconds(sec), milliseconds(ms) {}

    int totalMilliseconds() const { return seconds * 1000 + milliseconds; }
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::VideoState)
Q_DECLARE_METATYPE(churchpresenter::VideoError)
Q_DECLARE_METATYPE(churchpresenter::VideoFrame)
Q_DECLARE_METATYPE(churchpresenter::VideoPosition)
