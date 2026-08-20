#pragma once

#include "presentation/MediaTypes.h"

#include <optional>
#include <QStringList>
#include <QVector>

namespace churchpresenter {

struct MediaCatalogEntry {
    MediaType type = MediaType::Audio;
    QString fileName;
    QString title;
    QString path;
    QString folderPath;

    bool operator==(const MediaCatalogEntry &) const = default;
};

class MediaFolderScanner final {
public:
    [[nodiscard]] QVector<MediaCatalogEntry> scan(const QStringList &folders) const;
    [[nodiscard]] static QVector<MediaCatalogEntry> filter(
        const QVector<MediaCatalogEntry> &entries, MediaType type, const QString &fileNameQuery = {});
    [[nodiscard]] static std::optional<MediaType> mediaTypeForFile(const QString &path);
};

} // namespace churchpresenter
