#include "presentation/VideoFrameBus.h"

#include <algorithm>

namespace churchpresenter {

VideoFrameBus::VideoFrameBus(QObject *parent)
    : QObject(parent)
{
    connect(&m_source, &QVideoSink::videoFrameChanged, this, &VideoFrameBus::distribute);
}

QVideoSink *VideoFrameBus::sourceSink() { return &m_source; }
int VideoFrameBus::sinkCount() const { return m_sinks.size(); }
quint64 VideoFrameBus::frameCount() const { return m_frameCount; }

bool VideoFrameBus::registerSink(QObject *sinkObject)
{
    auto *sink = qobject_cast<QVideoSink *>(sinkObject);
    if (!sink || sink == &m_source) return false;
    const auto exists = std::any_of(m_sinks.cbegin(), m_sinks.cend(), [&](const auto &candidate) {
        return candidate == sink;
    });
    if (exists) return true;
    m_sinks.append(sink);
    connect(sink, &QObject::destroyed, this, [this, sink] { unregisterSink(sink); });
    return true;
}

void VideoFrameBus::unregisterSink(QObject *sinkObject)
{
    auto *sink = qobject_cast<QVideoSink *>(sinkObject);
    m_sinks.erase(std::remove_if(m_sinks.begin(), m_sinks.end(), [&](const auto &candidate) {
        return candidate.isNull() || candidate == sink;
    }), m_sinks.end());
}

void VideoFrameBus::clear()
{
    for (const auto &sink : std::as_const(m_sinks)) {
        if (sink) sink->setVideoFrame({});
    }
}

void VideoFrameBus::distribute(const QVideoFrame &frame)
{
    m_sinks.erase(std::remove_if(m_sinks.begin(), m_sinks.end(), [](const auto &sink) {
        return sink.isNull();
    }), m_sinks.end());
    if (frame.isValid()) ++m_frameCount;
    for (const auto &sink : std::as_const(m_sinks)) {
        sink->setVideoFrame(frame);
    }
    emit frameDistributed(m_frameCount, frame.startTime());
}

} // namespace churchpresenter
