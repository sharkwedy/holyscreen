#pragma once

#include "presentation/MediaTypes.h"

#include <QString>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QSqlQuery)

namespace churchpresenter {

class MediaRepository final {
public:
    explicit MediaRepository(QString databasePath);
    ~MediaRepository();

    MediaRepository(const MediaRepository &) = delete;
    MediaRepository &operator=(const MediaRepository &) = delete;

    bool open();
    [[nodiscard]] QVector<MediaItem> items(MediaType type) const;
    [[nodiscard]] QVector<MediaItem> playlistItems() const;
    [[nodiscard]] MediaItem item(const QString &id) const;
    [[nodiscard]] QString add(MediaItem item);
    bool update(const MediaItem &item);
    bool remove(const QString &id);
    bool clearPlaylist();
    bool move(const QString &id, int newIndex);
    bool moveInPlaylist(const QString &id, int newIndex);

private:
    static MediaItem readItem(QSqlQuery &query);
    bool normalizePositions(MediaType type);
    bool normalizePlaylistPositions();

    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
