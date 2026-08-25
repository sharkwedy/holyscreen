#include "app/MediaThumbnailer.h"

#include <QImage>
#include <QTemporaryDir>
#include <QtTest/QTest>

using namespace churchpresenter;

class MediaThumbnailerTest final : public QObject {
    Q_OBJECT

private slots:
    void returnsImageFilesDirectly();
    void rejectsMissingFiles();
};

void MediaThumbnailerTest::returnsImageFilesDirectly()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("slide.png"));
    QImage image(32, 18, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QVERIFY(image.save(path));

    MediaThumbnailer thumbnailer(directory.filePath(QStringLiteral("cache")));
    const auto source = thumbnailer.sourceFor(path, MediaType::Image);

    QVERIFY(source.isLocalFile());
    QCOMPARE(source.toLocalFile(), QFileInfo(path).canonicalFilePath());
}

void MediaThumbnailerTest::rejectsMissingFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    MediaThumbnailer thumbnailer(directory.filePath(QStringLiteral("cache")));
    QVERIFY(thumbnailer.sourceFor(directory.filePath(QStringLiteral("missing.mp4")),
                                  MediaType::Video).isEmpty());
}

QTEST_MAIN(MediaThumbnailerTest)
#include "MediaThumbnailerTest.moc"
