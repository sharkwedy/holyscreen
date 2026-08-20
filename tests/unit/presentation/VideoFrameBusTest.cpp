#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "presentation/VideoFrameBus.h"

#include <QCoreApplication>
#include <QImage>

#include <memory>

using namespace churchpresenter;

class VideoFrameBusTest final : public QObject {
    Q_OBJECT

private slots:
    void distributesOneSharedFrameToEveryRegisteredSink();
    void ignoresDuplicateAndDestroyedSinks();
    void clearsFramesFromEveryRegisteredSink();
};

void VideoFrameBusTest::distributesOneSharedFrameToEveryRegisteredSink()
{
    VideoFrameBus bus;
    QVideoSink first;
    QVideoSink second;
    QSignalSpy firstFrames(&first, &QVideoSink::videoFrameChanged);
    QSignalSpy secondFrames(&second, &QVideoSink::videoFrameChanged);
    QVERIFY(bus.registerSink(&first));
    QVERIFY(bus.registerSink(&second));

    const QVideoFrame frame(QImage(64, 36, QImage::Format_RGBA8888));
    bus.sourceSink()->setVideoFrame(frame);

    QCOMPARE(bus.frameCount(), quint64{1});
    QCOMPARE(firstFrames.size(), 1);
    QCOMPARE(secondFrames.size(), 1);
    QCOMPARE(first.videoFrame().size(), QSize(64, 36));
    QCOMPARE(second.videoFrame().size(), QSize(64, 36));
}

void VideoFrameBusTest::ignoresDuplicateAndDestroyedSinks()
{
    VideoFrameBus bus;
    auto sink = std::make_unique<QVideoSink>();
    QVERIFY(bus.registerSink(sink.get()));
    QVERIFY(bus.registerSink(sink.get()));
    QCOMPARE(bus.sinkCount(), 1);
    sink.reset();
    QCOMPARE(bus.sinkCount(), 0);
}

void VideoFrameBusTest::clearsFramesFromEveryRegisteredSink()
{
    VideoFrameBus bus;
    QVideoSink first;
    QVideoSink second;
    QVERIFY(bus.registerSink(&first));
    QVERIFY(bus.registerSink(&second));

    const QVideoFrame frame(QImage(64, 36, QImage::Format_RGBA8888));
    bus.sourceSink()->setVideoFrame(frame);
    QVERIFY(first.videoFrame().isValid());
    QVERIFY(second.videoFrame().isValid());

    bus.clear();

    QVERIFY(!first.videoFrame().isValid());
    QVERIFY(!second.videoFrame().isValid());
    QCOMPARE(bus.sinkCount(), 2);
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    VideoFrameBusTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "VideoFrameBusTest.moc"
