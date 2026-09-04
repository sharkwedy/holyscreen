#include "library/MediaFolderScanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

#include <algorithm>

namespace churchpresenter {
namespace {

const QSet<QString> audioExtensions{
    QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac"), QStringLiteral("m4a"),
    QStringLiteral("aac"), QStringLiteral("ogg"), QStringLiteral("opus"), QStringLiteral("wma"),
    QStringLiteral("aiff"), QStringLiteral("aif"),
};
const QSet<QString> videoExtensions{
    QStringLiteral("mp4"), QStringLiteral("mov"), QStringLiteral("m4v"), QStringLiteral("mkv"),
    QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("wmv"), QStringLiteral("mpeg"),
    QStringLiteral("mpg"),
};
const QSet<QString> imageExtensions{
    QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"), QStringLiteral("webp"),
    QStringLiteral("bmp"), QStringLiteral("gif"), QStringLiteral("tif"), QStringLiteral("tiff"),
    QStringLiteral("heic"),
};

} // namespace

QVector<MediaCatalogEntry> MediaFolderScanner::scan(const QStringList &folders) const
{
    QVector<MediaCatalogEntry> result;
    result.reserve(256);
    QSet<QString> visitedFiles;

    for (const auto &folder : folders) {
        const QFileInfo folderInfo(folder);
        const auto canonicalFolder = folderInfo.canonicalFilePath();
        if (!folderInfo.isDir() || canonicalFolder.isEmpty()) continue;

        QDirIterator iterator(canonicalFolder,
                              QDir::Files | QDir::Readable | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QFileInfo fileInfo(iterator.next());
            const auto type = mediaTypeForFile(fileInfo.fileName());
            // NoSymLinks excludes symbolic links, so resolving every file through
            // canonicalFilePath() only adds a filesystem lookup per entry. A
            // normalized absolute path is sufficient for deduplication here.
            const auto normalizedPath = QDir::cleanPath(fileInfo.absoluteFilePath());
            if (!type.has_value() || normalizedPath.isEmpty() || visitedFiles.contains(normalizedPath)) {
                continue;
            }
            visitedFiles.insert(normalizedPath);
            result.append(MediaCatalogEntry{
                .type = type.value(),
                .fileName = fileInfo.fileName(),
                .title = fileInfo.completeBaseName(),
                .path = normalizedPath,
                .folderPath = canonicalFolder,
            });
        }
    }

    std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        const auto byName = QString::localeAwareCompare(left.fileName, right.fileName);
        return byName == 0 ? left.path < right.path : byName < 0;
    });
    return result;
}

QVector<MediaCatalogEntry> MediaFolderScanner::filter(
    const QVector<MediaCatalogEntry> &entries, MediaType type, const QString &fileNameQuery)
{
    QVector<MediaCatalogEntry> result;
    result.reserve(entries.size());
    const auto query = fileNameQuery.trimmed();
    for (const auto &entry : entries) {
        if (entry.type != type) continue;
        if (!query.isEmpty() && !entry.fileName.contains(query, Qt::CaseInsensitive)) continue;
        result.append(entry);
    }
    return result;
}

std::optional<MediaType> MediaFolderScanner::mediaTypeForFile(const QString &path)
{
    const auto suffix = QFileInfo(path).suffix().toLower();
    if (audioExtensions.contains(suffix)) return MediaType::Audio;
    if (videoExtensions.contains(suffix)) return MediaType::Video;
    if (imageExtensions.contains(suffix)) return MediaType::Image;
    return std::nullopt;
}

} // namespace churchpresenter
