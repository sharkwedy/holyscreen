#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "library/MediaFolderScanner.h"

using namespace churchpresenter;

class MediaFolderScannerTest final : public QObject {
    Q_OBJECT

private slots:
    void recursivelyClassifiesSupportedFilesAndIgnoresUnknownOnes();
    void deduplicatesOverlappingSelectedFolders();
    void filtersByTypeAndFileNameWithoutCaseSensitivity();
};

static QString createFile(const QString &directory, const QString &relativePath)
{
    const QFileInfo info(QDir(directory).filePath(relativePath));
    QDir().mkpath(info.absolutePath());
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly)) return {};
    file.write("media");
    return info.absoluteFilePath();
}

void MediaFolderScannerTest::recursivelyClassifiesSupportedFilesAndIgnoresUnknownOnes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    createFile(directory.path(), QStringLiteral("Culto/Entrada.MP3"));
    createFile(directory.path(), QStringLiteral("Culto/Videos/Avisos.mp4"));
    createFile(directory.path(), QStringLiteral("Culto/Artes/Tela.PNG"));
    createFile(directory.path(), QStringLiteral("Culto/notas.txt"));

    const auto entries = MediaFolderScanner{}.scan({directory.filePath(QStringLiteral("Culto"))});
    QCOMPARE(entries.size(), 3);
    QCOMPARE(MediaFolderScanner::filter(entries, MediaType::Audio).size(), 1);
    QCOMPARE(MediaFolderScanner::filter(entries, MediaType::Video).size(), 1);
    QCOMPARE(MediaFolderScanner::filter(entries, MediaType::Image).size(), 1);
}

void MediaFolderScannerTest::deduplicatesOverlappingSelectedFolders()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto nested = directory.filePath(QStringLiteral("midias/videos"));
    createFile(directory.path(), QStringLiteral("midias/videos/loop.webm"));

    const auto entries = MediaFolderScanner{}.scan({directory.filePath(QStringLiteral("midias")), nested});
    QCOMPARE(entries.size(), 1);
    QCOMPARE(entries.front().fileName, QStringLiteral("loop.webm"));
}

void MediaFolderScannerTest::filtersByTypeAndFileNameWithoutCaseSensitivity()
{
    const QVector<MediaCatalogEntry> entries{
        {MediaType::Audio, QStringLiteral("Grande É.mp3"), {}, QStringLiteral("/a"), {}},
        {MediaType::Audio, QStringLiteral("Santo.wav"), {}, QStringLiteral("/b"), {}},
        {MediaType::Video, QStringLiteral("GRANDE abertura.mp4"), {}, QStringLiteral("/c"), {}},
    };

    const auto filtered = MediaFolderScanner::filter(entries, MediaType::Audio, QStringLiteral("grande"));
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered.front().fileName, QStringLiteral("Grande É.mp3"));
}

QTEST_APPLESS_MAIN(MediaFolderScannerTest)
#include "MediaFolderScannerTest.moc"
