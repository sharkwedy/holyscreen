#include "app/MediaThumbnailer.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QSaveFile>
#include <QVideoFrame>
#include <QVideoSink>

#include <utility>

namespace churchpresenter {

MediaThumbnailer::MediaThumbnailer(QString cacheDirectory, QObject *parent)
    : QObject(parent)
    , m_cacheDirectory(std::move(cacheDirectory))
    , m_player(new QMediaPlayer(this))
    , m_videoSink(new QVideoSink(this))
{
    QDir().mkpath(m_cacheDirectory);
    m_player->setVideoSink(m_videoSink);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(7000);

    connect(&m_timeout, &QTimer::timeout, this, [this] { finishCurrent(false); });
    connect(m_player, &QMediaPlayer::metaDataChanged,
            this, &MediaThumbnailer::tryEmbeddedArtwork);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
        if (!m_busy || m_finishing) return;
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            tryEmbeddedArtwork();
            if (!m_busy || m_current.type != MediaType::Video) return;
            const auto target = m_player->duration() > 0
                ? qMin<qint64>(10'000, m_player->duration() / 10) : 0;
            m_player->setPosition(target);
            m_player->play();
        } else if (status == QMediaPlayer::InvalidMedia) {
            finishCurrent(false);
        }
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error error, const QString &) {
        if (error != QMediaPlayer::NoError) finishCurrent(false);
    });
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
        if (!m_busy || m_finishing || m_current.type != MediaType::Video
            || !frame.isValid()) {
            return;
        }
        const auto image = frame.toImage();
        if (!image.isNull() && saveThumbnail(image)) finishCurrent(true);
    });
}

MediaThumbnailer::~MediaThumbnailer()
{
    m_timeout.stop();
    m_player->stop();
}

QUrl MediaThumbnailer::sourceFor(const QString &path, MediaType type) const
{
    const QFileInfo info(path);
    const auto canonicalPath = info.canonicalFilePath();
    if (!info.isFile() || canonicalPath.isEmpty()) return {};
    if (type == MediaType::Image) return QUrl::fromLocalFile(canonicalPath);

    const auto cachedPath = cachePathFor(canonicalPath);
    if (QFileInfo::exists(cachedPath)) return QUrl::fromLocalFile(cachedPath);
    return {};
}

void MediaThumbnailer::request(const QString &path, MediaType type)
{
    const QFileInfo info(path);
    const auto canonicalPath = info.canonicalFilePath();
    if (!info.isFile() || canonicalPath.isEmpty() || type == MediaType::Image
        || !sourceFor(canonicalPath, type).isEmpty()) {
        return;
    }
    if (m_failedCachePaths.contains(cachePathFor(canonicalPath))) return;
    if (!m_scheduledPaths.contains(canonicalPath)) {
        m_scheduledPaths.insert(canonicalPath);
        m_pending.enqueue({canonicalPath, type});
        QTimer::singleShot(0, this, &MediaThumbnailer::startNext);
    }
}

QString MediaThumbnailer::cachePathFor(const QString &path) const
{
    const QFileInfo info(path);
    const auto fingerprint = path.toUtf8() + '\0'
        + QByteArray::number(info.size()) + '\0'
        + QByteArray::number(info.lastModified().toMSecsSinceEpoch());
    const auto digest = QCryptographicHash::hash(fingerprint, QCryptographicHash::Sha256).toHex();
    return QDir(m_cacheDirectory).filePath(QString::fromLatin1(digest) + QStringLiteral(".jpg"));
}

void MediaThumbnailer::startNext()
{
    if (m_busy || m_pending.isEmpty()) return;
    m_current = m_pending.dequeue();
    m_busy = true;
    m_finishing = false;
    m_timeout.start();
    m_player->setSource(QUrl::fromLocalFile(m_current.path));
}

void MediaThumbnailer::tryEmbeddedArtwork()
{
    if (!m_busy || m_finishing) return;
    const auto metadata = m_player->metaData();
    auto image = metadata.value(QMediaMetaData::ThumbnailImage).value<QImage>();
    if (image.isNull()) image = metadata.value(QMediaMetaData::CoverArtImage).value<QImage>();
    if (!image.isNull() && saveThumbnail(image)) finishCurrent(true);
    else if (m_current.type == MediaType::Audio
             && m_player->mediaStatus() == QMediaPlayer::LoadedMedia) {
        finishCurrent(false);
    }
}

bool MediaThumbnailer::saveThumbnail(const QImage &image)
{
    if (image.isNull() || m_current.path.isEmpty()) return false;
    const auto scaled = image.scaled(QSize(320, 180), Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation);
    const auto thumbnail = scaled.copy(QRect((scaled.width() - 320) / 2,
                                             (scaled.height() - 180) / 2,
                                             320, 180));
    if (thumbnail.isNull()) return false;
    QSaveFile file(cachePathFor(m_current.path));
    if (!file.open(QIODevice::WriteOnly)) return false;
    if (!thumbnail.save(&file, "JPG", 82)) return false;
    return file.commit();
}

void MediaThumbnailer::finishCurrent(bool succeeded)
{
    if (!m_busy || m_finishing) return;
    m_finishing = true;
    m_timeout.stop();
    const auto completed = m_current;
    const auto source = succeeded ? QUrl::fromLocalFile(cachePathFor(completed.path)) : QUrl{};
    m_player->stop();
    m_player->setSource({});
    if (!succeeded) m_failedCachePaths.insert(cachePathFor(completed.path));
    m_scheduledPaths.remove(completed.path);
    m_current = {};
    m_busy = false;
    m_finishing = false;
    if (!source.isEmpty()) emit thumbnailReady(completed.path, source);
    QTimer::singleShot(0, this, &MediaThumbnailer::startNext);
}

} // namespace churchpresenter
