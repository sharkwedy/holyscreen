#pragma once
#include <QString>
#include <QVector>

namespace churchpresenter {
enum class PlaylistItemType { Song, Text, Image, Video, Audio };
struct PlaylistItem { QString id; PlaylistItemType type=PlaylistItemType::Text; QString referenceId; QString title; qint64 durationMs=0; int position=0; };
struct Event { QString id; QString title; QString scheduledAt; };
class EventRepository final {
public:
    explicit EventRepository(QString databasePath); ~EventRepository();
    bool open(); QVector<Event> events()const; Event event(const QString&id)const;
    QString saveEvent(Event event); bool removeEvent(const QString&id);
    QVector<PlaylistItem> items(const QString&eventId)const; bool addItem(const QString&eventId,PlaylistItem item);
    bool removeItem(const QString&id); bool moveItem(const QString&id,int index); qint64 totalDurationMs(const QString&eventId)const;
private: bool normalize(const QString&eventId);QString m_databasePath;QString m_connectionName;
};
}
