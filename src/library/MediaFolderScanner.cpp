#include "library/MediaFolderScanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

bool pathIsInsideFolders(const QString &path, const QStringList &folders)
{
#if defined(Q_OS_WIN)
    constexpr auto caseSensitivity = Qt::CaseInsensitive;
#else
    constexpr auto caseSensitivity = Qt::CaseSensitive;
#endif
    const auto normalizedPath = QDir::fromNativeSeparators(QDir::cleanPath(path));
    for (const auto &folder : folders) {
        const auto normalizedFolder = QDir::fromNativeSeparators(QDir::cleanPath(folder));
        if (normalizedPath.compare(normalizedFolder, caseSensitivity) == 0
            || normalizedPath.startsWith(normalizedFolder + QStringLiteral("/"),
                                         caseSensitivity)) {
            return true;
        }
    }
    return false;
}

} // namespace

QVector<MediaCatalogEntry> MediaFolderScanner::scan(const QStringList &folders) const
{
    return scanSnapshot(folders).entries;
}

MediaCatalogSnapshot MediaFolderScanner::scanSnapshot(const QStringList &folders) const
{
    MediaCatalogSnapshot snapshot;
    QSet<QString> visitedFiles;
    QSet<QString> visitedDirectories;

    for (const auto &folder : folders) {
        const QFileInfo folderInfo(folder);
        const auto canonicalFolder = folderInfo.canonicalFilePath();
        if (!folderInfo.isDir() || canonicalFolder.isEmpty()) continue;
        visitedDirectories.insert(canonicalFolder);

        QDirIterator iterator(canonicalFolder,
                              QDir::Files | QDir::Dirs | QDir::Readable
                                  | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QFileInfo fileInfo(iterator.next());
            if (fileInfo.isDir()) {
                const auto directory = fileInfo.canonicalFilePath();
                if (!directory.isEmpty()) visitedDirectories.insert(directory);
                continue;
            }
            const auto type = mediaTypeForFile(fileInfo.fileName());
            const auto canonicalPath = fileInfo.canonicalFilePath();
            if (!type.has_value() || canonicalPath.isEmpty() || visitedFiles.contains(canonicalPath)) {
                continue;
            }
            visitedFiles.insert(canonicalPath);
            snapshot.entries.append(MediaCatalogEntry{
                .type = type.value(),
                .fileName = fileInfo.fileName(),
                .title = fileInfo.completeBaseName(),
                .path = canonicalPath,
                .folderPath = canonicalFolder,
            });
        }
    }

    std::sort(snapshot.entries.begin(), snapshot.entries.end(), [](const auto &left, const auto &right) {
        const auto byName = QString::localeAwareCompare(left.fileName, right.fileName);
        return byName == 0 ? left.path < right.path : byName < 0;
    });
    snapshot.directories = visitedDirectories.values();
    std::sort(snapshot.directories.begin(), snapshot.directories.end());
    return snapshot;
}

QVector<MediaCatalogEntry> MediaFolderScanner::filter(
    const QVector<MediaCatalogEntry> &entries, MediaType type, const QString &fileNameQuery)
{
    QVector<MediaCatalogEntry> result;
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

QByteArray MediaFolderScanner::encodeCache(const MediaCatalogSnapshot &snapshot,
                                           const QStringList &folders)
{
    QJsonArray encodedFolders;
    for (const auto &folder : folders) encodedFolders.append(folder);

    QJsonArray encodedDirectories;
    for (const auto &directory : snapshot.directories) encodedDirectories.append(directory);

    QJsonArray encodedEntries;
    for (const auto &entry : snapshot.entries) {
        encodedEntries.append(QJsonObject{
            {QStringLiteral("type"), static_cast<int>(entry.type)},
            {QStringLiteral("fileName"), entry.fileName},
            {QStringLiteral("title"), entry.title},
            {QStringLiteral("path"), entry.path},
            {QStringLiteral("folderPath"), entry.folderPath},
        });
    }
    return QJsonDocument(QJsonObject{
        {QStringLiteral("version"), 1},
        {QStringLiteral("folders"), encodedFolders},
        {QStringLiteral("directories"), encodedDirectories},
        {QStringLiteral("entries"), encodedEntries},
    }).toJson(QJsonDocument::Compact);
}

std::optional<MediaCatalogSnapshot> MediaFolderScanner::decodeCache(
    const QByteArray &payload, const QStringList &expectedFolders)
{
    constexpr qsizetype maximumCachedItems = 250'000;
    const auto document = QJsonDocument::fromJson(payload);
    if (!document.isObject()) return std::nullopt;
    const auto root = document.object();
    if (root.value(QStringLiteral("version")).toInt() != 1) return std::nullopt;

    QStringList folders;
    for (const auto &value : root.value(QStringLiteral("folders")).toArray()) {
        if (!value.isString()) return std::nullopt;
        folders.append(value.toString());
    }
    if (folders != expectedFolders) return std::nullopt;

    const auto encodedDirectories = root.value(QStringLiteral("directories")).toArray();
    const auto encodedEntries = root.value(QStringLiteral("entries")).toArray();
    if (encodedDirectories.size() > maximumCachedItems
        || encodedEntries.size() > maximumCachedItems) {
        return std::nullopt;
    }

    MediaCatalogSnapshot snapshot;
    snapshot.directories.reserve(encodedDirectories.size());
    for (const auto &value : encodedDirectories) {
        if (!value.isString() || !QDir::isAbsolutePath(value.toString())
            || !pathIsInsideFolders(value.toString(), expectedFolders)) {
            return std::nullopt;
        }
        snapshot.directories.append(value.toString());
    }

    snapshot.entries.reserve(encodedEntries.size());
    for (const auto &value : encodedEntries) {
        if (!value.isObject()) return std::nullopt;
        const auto object = value.toObject();
        const auto typeValue = object.value(QStringLiteral("type")).toInt(-1);
        const auto path = object.value(QStringLiteral("path")).toString();
        const auto fileName = object.value(QStringLiteral("fileName")).toString();
        const auto folderPath = object.value(QStringLiteral("folderPath")).toString();
        if (typeValue < static_cast<int>(MediaType::Audio)
            || typeValue > static_cast<int>(MediaType::Image)
            || fileName.isEmpty() || folderPath.isEmpty() || !QDir::isAbsolutePath(path)
            || !pathIsInsideFolders(path, expectedFolders)) {
            return std::nullopt;
        }
        snapshot.entries.append(MediaCatalogEntry{
            .type = static_cast<MediaType>(typeValue),
            .fileName = fileName,
            .title = object.value(QStringLiteral("title")).toString(),
            .path = path,
            .folderPath = folderPath,
        });
    }
    return snapshot;
}

} // namespace churchpresenter
