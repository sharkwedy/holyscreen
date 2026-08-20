#pragma once

#include "presentation/MediaTypes.h"

#include <QString>
#include <QMetaType>

namespace churchpresenter {

Q_NAMESPACE

enum class AudioState {
    Stopped,
    Loading,
    Ready,
    Playing,
    Paused,
    Buffering,
    Error
};
Q_ENUM_NS(AudioState)

enum class AudioError {
    None,
    FileNotFound,
    ReadError,
    UnsupportedFormat,
    DecodeError,
    DeviceError
};
Q_ENUM_NS(AudioError)

struct AudioPosition {
    int seconds = 0;
    int milliseconds = 0;

    AudioPosition() = default;
    AudioPosition(int sec, int ms = 0) : seconds(sec), milliseconds(ms) {}

    int totalMilliseconds() const { return seconds * 1000 + milliseconds; }
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::AudioState)
Q_DECLARE_METATYPE(churchpresenter::AudioError)
Q_DECLARE_METATYPE(churchpresenter::AudioPosition)
