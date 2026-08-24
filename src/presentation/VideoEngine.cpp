#include "presentation/VideoEngine.h"

#include <QAudioOutput>
#include <QFile>
#include <QFileInfo>
#include <QMediaMetaData>

#include <algorithm>

namespace churchpresenter {

VideoEngine::VideoEngine(QObject *parent)
    : IVideoEngine(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_frameBus(this)
{
    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoSink(m_frameBus.sourceSink());
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &VideoEngine::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoEngine::onPositionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &VideoEngine::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &VideoEngine::onErrorOccurred);
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        m_duration = VideoPosition(static_cast<int>(duration / 1000), static_cast<int>(duration % 1000));
        m_currentMedia.durationMs = duration;
        emit durationChanged(static_cast<int>(duration));
        emit mediaMetadataChanged(m_currentMedia);
    });
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this] {
        const auto title = m_player->metaData().value(QMediaMetaData::Title).toString();
        if (!title.isEmpty()) m_currentMedia.title = title;
        emit mediaMetadataChanged(m_currentMedia);
    });
}

VideoEngine::~VideoEngine() { stop(); }
void VideoEngine::load(const QString &mediaId) { loadFromPath(mediaId); }

void VideoEngine::loadFromPath(const QString &path)
{
    const QUrl requested(path);
    const auto localPath = requested.isLocalFile() ? requested.toLocalFile() : path;
    if (localPath.isEmpty() || !QFile::exists(localPath)) {
        setError(VideoError::FileNotFound);
        return;
    }
    const auto canonicalPath = QFileInfo(localPath).canonicalFilePath();
    m_error = VideoError::None;
    m_currentMedia = MediaItem{
        .id = QString::number(qHash(canonicalPath)),
        .type = MediaType::Video,
        .title = QFileInfo(canonicalPath).completeBaseName(),
        .path = canonicalPath,
    };
    setState(VideoState::Loading);
    m_player->setSource(QUrl::fromLocalFile(canonicalPath));
    emit mediaLoaded(m_currentMedia.id);
}

void VideoEngine::play()
{
    if (!m_currentMedia.path.isEmpty() && !QFile::exists(m_currentMedia.path)) {
        m_frameBus.clear();
        setError(VideoError::FileNotFound);
        return;
    }
    if (m_error == VideoError::None && m_state != VideoState::Playing) m_player->play();
}

void VideoEngine::pause()
{
    if (m_state == VideoState::Playing) m_player->pause();
}

void VideoEngine::stop()
{
    m_player->stop();
    m_frameBus.clear();
    setState(VideoState::Stopped);
}

void VideoEngine::seek(int positionMs)
{
    m_player->setPosition(std::clamp<qint64>(positionMs, 0, m_player->duration()));
}

VideoPosition VideoEngine::position() const { return m_position; }
VideoPosition VideoEngine::duration() const { return m_duration; }
void VideoEngine::setVolume(double volume) { m_audioOutput->setVolume(std::clamp(volume, 0.0, 1.0)); }
double VideoEngine::volume() const { return m_audioOutput->volume(); }
void VideoEngine::setAudioDevice(const QAudioDevice &device) { m_audioOutput->setDevice(device); }
QAudioDevice VideoEngine::audioDevice() const { return m_audioOutput->device(); }

void VideoEngine::setLoop(bool loop)
{
    m_loop = loop;
    m_player->setLoops(loop ? QMediaPlayer::Infinite : QMediaPlayer::Once);
}

bool VideoEngine::loop() const { return m_loop; }

void VideoEngine::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        setState(VideoState::Playing);
        emit playbackStarted();
        break;
    case QMediaPlayer::PausedState:
        setState(VideoState::Paused);
        emit playbackPaused();
        break;
    case QMediaPlayer::StoppedState:
        setState(VideoState::Stopped);
        emit playbackStopped();
        break;
    }
}

void VideoEngine::onPositionChanged(qint64 position)
{
    m_position = VideoPosition(static_cast<int>(position / 1000), static_cast<int>(position % 1000));
    emit positionChanged(static_cast<int>(position));
}

void VideoEngine::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadingMedia: setState(VideoState::Loading); break;
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferedMedia:
        if (m_player->playbackState() == QMediaPlayer::PlayingState)
            setState(VideoState::Playing);
        else if (m_player->playbackState() == QMediaPlayer::PausedState)
            setState(VideoState::Paused);
        else
            setState(VideoState::Ready);
        break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::StalledMedia:
        if (m_player->playbackState() == QMediaPlayer::PlayingState) {
            setState(VideoState::Buffering);
        }
        break;
    case QMediaPlayer::InvalidMedia: setError(VideoError::UnsupportedFormat); break;
    case QMediaPlayer::EndOfMedia:
        if (!m_loop) {
            setState(VideoState::Stopped);
            emit playbackFinished();
        }
        break;
    default: break;
    }
}

void VideoEngine::onErrorOccurred(QMediaPlayer::Error playerError, const QString &message)
{
    auto error = VideoError::DecodeError;
    if (message.contains(QStringLiteral("not found"), Qt::CaseInsensitive)) {
        error = VideoError::FileNotFound;
    } else if (playerError == QMediaPlayer::FormatError) {
        error = VideoError::UnsupportedFormat;
    } else if (playerError == QMediaPlayer::AccessDeniedError
               || playerError == QMediaPlayer::ResourceError
               || message.contains(QStringLiteral("read"), Qt::CaseInsensitive)) {
        error = VideoError::ReadError;
    }
    setError(error);
}

void VideoEngine::setState(VideoState state)
{
    if (m_state == state) return;
    m_state = state;
    emit stateChanged(state);
}

void VideoEngine::setError(VideoError error)
{
    if (m_error == error) return;
    m_error = error;
    emit errorOccurred(error);
    if (error != VideoError::None) setState(VideoState::Error);
}

} // namespace churchpresenter
