#pragma once

#include "presentation/MediaTypes.h"

#include <QObject>
#include <QQueue>
#include <QSet>
#include <QTimer>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QMediaPlayer)
QT_FORWARD_DECLARE_CLASS(QImage)
QT_FORWARD_DECLARE_CLASS(QVideoSink)

namespace churchpresenter {

//! Produces media thumbnails one at a time and keeps them in a disk cache.
//!
//! Images are returned directly. Video frames and embedded audio/video artwork
//! are decoded asynchronously so browsing a large folder never creates one
//! media player per visible row.
class MediaThumbnailer final : public QObject {
    Q_OBJECT

public:
    explicit MediaThumbnailer(QString cacheDirectory, QObject *parent = nullptr);
    ~MediaThumbnailer() override;

    [[nodiscard]] QUrl sourceFor(const QString &path, MediaType type) const;
    void request(const QString &path, MediaType type);

signals:
    void thumbnailReady(const QString &path, const QUrl &source);

private:
    struct Request {
        QString path;
        MediaType type = MediaType::Audio;
    };

    [[nodiscard]] QString cachePathFor(const QString &path) const;
    void startNext();
    void tryEmbeddedArtwork();
    bool saveThumbnail(const QImage &image);
    void scheduleFinish(bool succeeded);
    void finishCurrent(bool succeeded);

    QString m_cacheDirectory;
    QQueue<Request> m_pending;
    QSet<QString> m_scheduledPaths;
    QSet<QString> m_failedCachePaths;
    Request m_current;
    QMediaPlayer *m_player = nullptr;
    QVideoSink *m_videoSink = nullptr;
    QTimer m_timeout;
    bool m_busy = false;
    bool m_finishing = false;
};

} // namespace churchpresenter
