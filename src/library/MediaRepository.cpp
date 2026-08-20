#include "library/MediaRepository.h"

#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <algorithm>

namespace churchpresenter {

MediaRepository::MediaRepository(QString databasePath)
    : m_databasePath(std::move(databasePath))
{
}

MediaRepository::~MediaRepository()
{
    if (m_connectionName.isEmpty()) return;
    {
        auto database = QSqlDatabase::database(m_connectionName, false);
        if (database.isValid()) database.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool MediaRepository::open()
{
    if (!m_connectionName.isEmpty() && QSqlDatabase::database(m_connectionName, false).isOpen()) {
        return true;
    }
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) return false;

    m_connectionName = QStringLiteral("holyscreen-media-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_databasePath);
    if (!database.open()) {
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }

    QSqlQuery query(database);
    if (!query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS media_items ("
        "id TEXT PRIMARY KEY NOT NULL,"
        "type INTEGER NOT NULL,"
        "title TEXT NOT NULL,"
        "path TEXT NOT NULL,"
        "duration_ms INTEGER NOT NULL DEFAULT 0,"
        "artist TEXT NOT NULL DEFAULT '',"
        "album TEXT NOT NULL DEFAULT '',"
        "position INTEGER NOT NULL,"
        "playlist_position INTEGER NOT NULL DEFAULT 0,"
        "UNIQUE(type, path)"
        ")"))) {
        return false;
    }

    bool hasPlaylistPosition = false;
    if (query.exec(QStringLiteral("PRAGMA table_info(media_items)"))) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("playlist_position")) {
                hasPlaylistPosition = true;
                break;
            }
        }
    }
    if (!hasPlaylistPosition) {
        if (!query.exec(QStringLiteral(
                "ALTER TABLE media_items ADD COLUMN playlist_position INTEGER NOT NULL DEFAULT 0"))) {
            return false;
        }
        query.exec(QStringLiteral(
            "WITH ordered AS (SELECT id, ROW_NUMBER() OVER (ORDER BY rowid)-1 AS pos FROM media_items) "
            "UPDATE media_items SET playlist_position=(SELECT pos FROM ordered WHERE ordered.id=media_items.id)"));
    }
    return normalizePlaylistPositions();
}

QVector<MediaItem> MediaRepository::items(MediaType type) const
{
    QVector<MediaItem> result;
    if (m_connectionName.isEmpty()) return result;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "SELECT id,type,title,path,duration_ms,artist,album FROM media_items "
        "WHERE type=:type ORDER BY position,id"));
    query.bindValue(QStringLiteral(":type"), static_cast<int>(type));
    if (!query.exec()) {
        qWarning() << "Could not list media items:" << query.lastError().text();
        return result;
    }
    while (query.next()) result.append(readItem(query));
    return result;
}

QVector<MediaItem> MediaRepository::playlistItems() const
{
    QVector<MediaItem> result;
    if (m_connectionName.isEmpty()) return result;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    if (!query.exec(QStringLiteral(
            "SELECT id,type,title,path,duration_ms,artist,album FROM media_items "
            "ORDER BY playlist_position,id"))) {
        qWarning() << "Could not list media playlist:" << query.lastError().text();
        return result;
    }
    while (query.next()) result.append(readItem(query));
    return result;
}

MediaItem MediaRepository::item(const QString &id) const
{
    if (m_connectionName.isEmpty()) return {};
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "SELECT id,type,title,path,duration_ms,artist,album FROM media_items WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) return {};
    return readItem(query);
}

QString MediaRepository::add(MediaItem item)
{
    if (m_connectionName.isEmpty() && !open()) return {};
    const auto canonicalPath = QFileInfo(item.path).canonicalFilePath();
    if (canonicalPath.isEmpty()) return {};

    QSqlQuery existing(QSqlDatabase::database(m_connectionName, false));
    existing.prepare(QStringLiteral("SELECT id FROM media_items WHERE type=:type AND path=:path"));
    existing.bindValue(QStringLiteral(":type"), static_cast<int>(item.type));
    existing.bindValue(QStringLiteral(":path"), canonicalPath);
    if (existing.exec() && existing.next()) return existing.value(0).toString();

    if (item.id.isEmpty()) item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.path = canonicalPath;
    if (item.title.trimmed().isEmpty()) item.title = QFileInfo(canonicalPath).completeBaseName();
    if (item.artist.isNull()) item.artist = QStringLiteral("");
    if (item.album.isNull()) item.album = QStringLiteral("");

    QSqlQuery positionQuery(QSqlDatabase::database(m_connectionName, false));
    positionQuery.prepare(QStringLiteral(
        "SELECT COALESCE(MAX(position),-1)+1 FROM media_items WHERE type=:type"));
    positionQuery.bindValue(QStringLiteral(":type"), static_cast<int>(item.type));
    if (!positionQuery.exec() || !positionQuery.next()) {
        qWarning() << "Could not determine media position:" << positionQuery.lastError().text();
        return {};
    }

    QSqlQuery playlistPositionQuery(QSqlDatabase::database(m_connectionName, false));
    if (!playlistPositionQuery.exec(QStringLiteral(
            "SELECT COALESCE(MAX(playlist_position),-1)+1 FROM media_items"))
        || !playlistPositionQuery.next()) {
        return {};
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO media_items(id,type,title,path,duration_ms,artist,album,position,playlist_position) "
        "VALUES(:id,:type,:title,:path,:duration,:artist,:album,:position,:playlistPosition)"));
    query.bindValue(QStringLiteral(":id"), item.id);
    query.bindValue(QStringLiteral(":type"), static_cast<int>(item.type));
    query.bindValue(QStringLiteral(":title"), item.title);
    query.bindValue(QStringLiteral(":path"), item.path);
    query.bindValue(QStringLiteral(":duration"), item.durationMs);
    query.bindValue(QStringLiteral(":artist"), item.artist);
    query.bindValue(QStringLiteral(":album"), item.album);
    query.bindValue(QStringLiteral(":position"), positionQuery.value(0));
    query.bindValue(QStringLiteral(":playlistPosition"), playlistPositionQuery.value(0));
    if (!query.exec()) {
        qWarning() << "Could not add media item:" << query.lastError().text();
        return {};
    }
    return item.id;
}

bool MediaRepository::update(const MediaItem &item)
{
    if (m_connectionName.isEmpty() || item.id.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "UPDATE media_items SET title=:title,duration_ms=:duration,artist=:artist,album=:album "
        "WHERE id=:id"));
    query.bindValue(QStringLiteral(":title"), item.title);
    query.bindValue(QStringLiteral(":duration"), item.durationMs);
    query.bindValue(QStringLiteral(":artist"), item.artist);
    query.bindValue(QStringLiteral(":album"), item.album);
    query.bindValue(QStringLiteral(":id"), item.id);
    return query.exec() && query.numRowsAffected() == 1;
}

bool MediaRepository::remove(const QString &id)
{
    const auto existing = item(id);
    if (existing.id.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("DELETE FROM media_items WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), id);
    return query.exec() && normalizePositions(existing.type) && normalizePlaylistPositions();
}

bool MediaRepository::moveInPlaylist(const QString &id, int newIndex)
{
    auto ordered = playlistItems();
    const auto current = std::find_if(ordered.begin(), ordered.end(), [&](const auto &candidate) {
        return candidate.id == id;
    });
    if (current == ordered.end()) return false;
    const auto moved = *current;
    ordered.erase(current);
    newIndex = std::clamp(newIndex, 0, static_cast<int>(ordered.size()));
    ordered.insert(ordered.begin() + newIndex, moved);

    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.transaction()) return false;
    QSqlQuery query(database);
    query.prepare(QStringLiteral("UPDATE media_items SET playlist_position=:position WHERE id=:id"));
    for (int index = 0; index < ordered.size(); ++index) {
        query.bindValue(QStringLiteral(":position"), index);
        query.bindValue(QStringLiteral(":id"), ordered[index].id);
        if (!query.exec()) {
            database.rollback();
            return false;
        }
    }
    return database.commit();
}

bool MediaRepository::move(const QString &id, int newIndex)
{
    const auto existing = item(id);
    if (existing.id.isEmpty()) return false;
    auto ordered = items(existing.type);
    const auto current = std::find_if(ordered.begin(), ordered.end(), [&](const auto &candidate) {
        return candidate.id == id;
    });
    if (current == ordered.end()) return false;
    const auto moved = *current;
    ordered.erase(current);
    newIndex = std::clamp(newIndex, 0, static_cast<int>(ordered.size()));
    ordered.insert(ordered.begin() + newIndex, moved);

    auto database = QSqlDatabase::database(m_connectionName, false);
    if (!database.transaction()) return false;
    QSqlQuery query(database);
    query.prepare(QStringLiteral("UPDATE media_items SET position=:position WHERE id=:id"));
    for (int index = 0; index < ordered.size(); ++index) {
        query.bindValue(QStringLiteral(":position"), index);
        query.bindValue(QStringLiteral(":id"), ordered[index].id);
        if (!query.exec()) {
            database.rollback();
            return false;
        }
    }
    return database.commit();
}

MediaItem MediaRepository::readItem(QSqlQuery &query)
{
    return MediaItem{
        .id = query.value(0).toString(),
        .type = static_cast<MediaType>(query.value(1).toInt()),
        .title = query.value(2).toString(),
        .path = query.value(3).toString(),
        .durationMs = query.value(4).toLongLong(),
        .artist = query.value(5).toString(),
        .album = query.value(6).toString(),
    };
}

bool MediaRepository::normalizePositions(MediaType type)
{
    const auto ordered = items(type);
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("UPDATE media_items SET position=:position WHERE id=:id"));
    for (int index = 0; index < ordered.size(); ++index) {
        query.bindValue(QStringLiteral(":position"), index);
        query.bindValue(QStringLiteral(":id"), ordered[index].id);
        if (!query.exec()) return false;
    }
    return true;
}

bool MediaRepository::normalizePlaylistPositions()
{
    const auto ordered = playlistItems();
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("UPDATE media_items SET playlist_position=:position WHERE id=:id"));
    for (int index = 0; index < ordered.size(); ++index) {
        query.bindValue(QStringLiteral(":position"), index);
        query.bindValue(QStringLiteral(":id"), ordered[index].id);
        if (!query.exec()) return false;
    }
    return true;
}

} // namespace churchpresenter
