#include "AudioEngine.h"

#include <QAudioOutput>
#include <QFile>
#include <QFileInfo>
#include <QMediaMetaData>
#include <QDebug>

#include <algorithm>

namespace churchpresenter {

AudioEngine::AudioEngine(QObject *parent)
    : IAudioEngine(parent)
    , m_player(new QMediaPlayer(this))
{
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);

    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &AudioEngine::onStateChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioEngine::onPositionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioEngine::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &AudioEngine::onErrorOccurred);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this] {
        const auto metadata = m_player->metaData();
        const auto title = metadata.value(QMediaMetaData::Title).toString();
        const auto album = metadata.value(QMediaMetaData::AlbumTitle).toString();
        const auto artists = metadata.value(QMediaMetaData::ContributingArtist).toStringList();
        if (!title.isEmpty()) m_currentMedia.title = title;
        if (!album.isEmpty()) m_currentMedia.album = album;
        if (!artists.isEmpty()) m_currentMedia.artist = artists.join(QStringLiteral(", "));
        emit mediaMetadataChanged(m_currentMedia);
    });
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 dur) {
        m_duration = AudioPosition(static_cast<int>(dur / 1000), static_cast<int>(dur % 1000));
        m_currentMedia.durationMs = dur;
        emit durationChanged(static_cast<int>(dur));
        emit mediaMetadataChanged(m_currentMedia);
    });
}

AudioEngine::~AudioEngine()
{
    stop();
}

void AudioEngine::load(const QString &mediaId)
{
    loadFromPath(mediaId);
}

void AudioEngine::loadFromPath(const QString &path)
{
    const QUrl requestedUrl(path);
    const auto localPath = requestedUrl.isLocalFile() ? requestedUrl.toLocalFile() : path;
    if (localPath.isEmpty() || !QFile::exists(localPath)) {
        setError(AudioError::FileNotFound);
        return;
    }

    m_error = AudioError::None;
    setState(AudioState::Loading);
    const auto canonicalPath = QFileInfo(localPath).canonicalFilePath();
    QUrl url = QUrl::fromLocalFile(canonicalPath);
    m_player->setSource(url);

    m_currentPath = canonicalPath;
    m_currentMedia = {};
    m_currentMedia.path = canonicalPath;
    m_currentMedia.id = QString::number(qHash(canonicalPath));
    m_currentMedia.title = QFileInfo(url.toLocalFile()).completeBaseName();

    emit mediaLoaded(m_currentMedia.id);
}

void AudioEngine::play()
{
    if (!m_currentPath.isEmpty() && !QFile::exists(m_currentPath)) {
        setError(AudioError::FileNotFound);
        return;
    }
    if (m_error != AudioError::None) return;
    if (m_state == AudioState::Playing) return;

    m_player->play();
}

void AudioEngine::pause()
{
    if (m_state != AudioState::Playing) return;
    m_player->pause();
}

void AudioEngine::stop()
{
    if (m_state == AudioState::Stopped) return;
    m_player->stop();
}

void AudioEngine::seek(int positionMs)
{
    m_player->setPosition(std::clamp<qint64>(positionMs, 0, m_player->duration()));
}

AudioPosition AudioEngine::position() const
{
    return m_position;
}

AudioPosition AudioEngine::duration() const
{
    return m_duration;
}

void AudioEngine::setVolume(double volume)
{
    m_audioOutput->setVolume(static_cast<float>(std::clamp(volume, 0.0, 1.0)));
}

double AudioEngine::volume() const
{
    return static_cast<double>(m_audioOutput->volume());
}

void AudioEngine::onStateChanged(QMediaPlayer::PlaybackState newState)
{
    AudioState oldState = m_state;

    switch (newState) {
    case QMediaPlayer::PlayingState:
        m_state = AudioState::Playing;
        break;
    case QMediaPlayer::PausedState:
        m_state = AudioState::Paused;
        break;
    case QMediaPlayer::StoppedState:
        m_state = AudioState::Stopped;
        break;
    }

    if (oldState != m_state) {
        emit stateChanged(m_state);
        if (m_state == AudioState::Playing) {
            emit playbackStarted();
        } else if (m_state == AudioState::Paused && oldState == AudioState::Playing) {
            emit playbackPaused();
        } else if (m_state == AudioState::Stopped) {
            emit playbackStopped();
        }
    }
}

void AudioEngine::onPositionChanged(qint64 pos)
{
    m_position = AudioPosition(static_cast<int>(pos / 1000), static_cast<int>(pos % 1000));
    emit positionChanged(static_cast<int>(pos));
}

void AudioEngine::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadingMedia:
        setState(AudioState::Loading);
        break;
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferedMedia:
        if (m_player->playbackState() == QMediaPlayer::StoppedState) {
            setState(AudioState::Ready);
        }
        break;
    case QMediaPlayer::InvalidMedia:
        setError(AudioError::UnsupportedFormat);
        break;
    case QMediaPlayer::EndOfMedia:
        setState(AudioState::Stopped);
        emit playbackFinished();
        break;
    default:
        break;
    }
}

void AudioEngine::onErrorOccurred(QMediaPlayer::Error playerError, const QString &errorString)
{
    qWarning() << "Audio error:" << errorString;
    AudioError err = AudioError::DecodeError;
    if (errorString.contains("not found", Qt::CaseInsensitive)) {
        err = AudioError::FileNotFound;
    } else if (playerError == QMediaPlayer::FormatError) {
        err = AudioError::UnsupportedFormat;
    } else if (playerError == QMediaPlayer::AccessDeniedError
               || playerError == QMediaPlayer::ResourceError
               || errorString.contains("read", Qt::CaseInsensitive)) {
        err = AudioError::ReadError;
    }
    setError(err);
}

void AudioEngine::setState(AudioState newState)
{
    if (m_state == newState) return;
    m_state = newState;
    emit stateChanged(newState);
}

void AudioEngine::setError(AudioError newError)
{
    if (m_error == newError) return;
    m_error = newError;
    emit errorOccurred(newError);
    if (newError != AudioError::None) {
        setState(AudioState::Error);
    }
}

} // namespace churchpresenter
