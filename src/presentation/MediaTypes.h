#pragma once

#include <QMetaType>
#include <QString>

namespace churchpresenter {

enum class MediaType {
    Audio,
    Video,
    Image,
};

struct MediaItem {
    Q_GADGET
public:
    QString id;
    MediaType type = MediaType::Audio;
    QString title;
    QString path;
    qint64 durationMs = 0;
    QString artist;
    QString album;

    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString path MEMBER path)
    Q_PROPERTY(qint64 durationMs MEMBER durationMs)
    Q_PROPERTY(QString artist MEMBER artist)
    Q_PROPERTY(QString album MEMBER album)
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::MediaType)
Q_DECLARE_METATYPE(churchpresenter::MediaItem)
