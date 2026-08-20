#pragma once

#include <QObject>
#include <QPointer>
#include <QVector>
#include <QVideoFrame>
#include <QVideoSink>

namespace churchpresenter {

class VideoFrameBus final : public QObject {
    Q_OBJECT

public:
    explicit VideoFrameBus(QObject *parent = nullptr);

    [[nodiscard]] QVideoSink *sourceSink();
    [[nodiscard]] int sinkCount() const;
    [[nodiscard]] quint64 frameCount() const;
    bool registerSink(QObject *sinkObject);
    void unregisterSink(QObject *sinkObject);
    void clear();

signals:
    void frameDistributed(quint64 frameNumber, qint64 startTimeUs);

private:
    void distribute(const QVideoFrame &frame);

    QVideoSink m_source;
    QVector<QPointer<QVideoSink>> m_sinks;
    quint64 m_frameCount = 0;
};

} // namespace churchpresenter
