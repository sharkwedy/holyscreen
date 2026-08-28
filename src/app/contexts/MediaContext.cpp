#include "app/contexts/MediaContext.h"

#include "app/ApplicationController.h"

#include <QDesktopServices>

namespace churchpresenter {

MediaContext::MediaContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller), m_onlineLyrics(this)
{
    connect(&controller, &ApplicationController::songsChanged, this, &MediaContext::songsChanged);
    connect(&controller, &ApplicationController::songSearchChanged, this, &MediaContext::songSearchChanged);
    connect(&controller, &ApplicationController::mediaPlaylistChanged, this, &MediaContext::mediaPlaylistChanged);
    connect(&controller, &ApplicationController::mediaFoldersChanged, this, &MediaContext::mediaFoldersChanged);
    connect(&controller, &ApplicationController::mediaCatalogChanged, this, &MediaContext::mediaCatalogChanged);
    connect(&controller, &ApplicationController::favoriteMediaChanged, this, &MediaContext::favoriteMediaChanged);
    connect(&controller, &ApplicationController::currentMediaChanged, this, &MediaContext::currentMediaChanged);
    connect(&controller, &ApplicationController::mediaStateChanged, this, &MediaContext::mediaStateChanged);
    connect(&controller, &ApplicationController::mediaPositionChanged, this, &MediaContext::mediaPositionChanged);
    connect(&controller, &ApplicationController::mediaDurationChanged, this, &MediaContext::mediaDurationChanged);
    connect(&controller, &ApplicationController::mediaVolumeChanged, this, &MediaContext::mediaVolumeChanged);
    connect(&controller, &ApplicationController::mediaRepeatModeChanged, this, &MediaContext::mediaRepeatModeChanged);
    connect(&controller, &ApplicationController::audioOutputsChanged, this, &MediaContext::audioOutputsChanged);
    connect(&m_onlineLyrics, &OnlineLyricsService::changed,
            this, &MediaContext::onlineLyricsChanged);
    connect(&m_onlineLyrics, &OnlineLyricsService::lyricsLoaded, this,
            [this](const QString &key) {
        emit onlineLyricsLoaded(key);
        if (m_pendingQuickSaveKey != key) return;
        m_pendingQuickSaveKey.clear();
        const auto item = m_onlineLyrics.result(key);
        persistOnlineLyrics(key, item.value(QStringLiteral("title")).toString(),
                            item.value(QStringLiteral("artist")).toString(),
                            item.value(QStringLiteral("lyrics")).toString());
    });
    connect(&m_onlineLyrics, &OnlineLyricsService::lyricsLoadFailed, this,
            [this](const QString &key, const QString &message) {
        if (m_pendingQuickSaveKey == key) m_pendingQuickSaveKey.clear();
        m_onlineLyricsStatus = message;
        emit onlineLyricsChanged();
    });
}

QVariantList MediaContext::songs() const { return m_controller.songs(); }
QString MediaContext::songSearch() const { return m_controller.songSearch(); }
void MediaContext::setSongSearch(const QString &value) { m_controller.setSongSearch(value); }
QVariantList MediaContext::mediaPlaylist() const { return m_controller.mediaPlaylist(); }
QVariantList MediaContext::mediaFolders() const { return m_controller.mediaFolders(); }
QVariantList MediaContext::folderAudioFiles() const { return m_controller.folderAudioFiles(); }
QVariantList MediaContext::folderVideoFiles() const { return m_controller.folderVideoFiles(); }
QVariantList MediaContext::folderImageFiles() const { return m_controller.folderImageFiles(); }
QVariantList MediaContext::favoriteMedia() const { return m_controller.favoriteMedia(); }
QString MediaContext::audioFileSearch() const { return m_controller.audioFileSearch(); }
void MediaContext::setAudioFileSearch(const QString &value) { m_controller.setAudioFileSearch(value); }
QString MediaContext::videoFileSearch() const { return m_controller.videoFileSearch(); }
void MediaContext::setVideoFileSearch(const QString &value) { m_controller.setVideoFileSearch(value); }
QString MediaContext::imageFileSearch() const { return m_controller.imageFileSearch(); }
void MediaContext::setImageFileSearch(const QString &value) { m_controller.setImageFileSearch(value); }
QString MediaContext::currentMediaId() const { return m_controller.currentMediaId(); }
QString MediaContext::currentMediaTitle() const { return m_controller.currentMediaTitle(); }
QString MediaContext::currentMediaType() const { return m_controller.currentMediaType(); }
QString MediaContext::mediaState() const { return m_controller.mediaState(); }
int MediaContext::mediaPositionMs() const { return m_controller.mediaPositionMs(); }
int MediaContext::mediaDurationMs() const { return m_controller.mediaDurationMs(); }
double MediaContext::mediaVolume() const { return m_controller.mediaVolume(); }
void MediaContext::setMediaVolume(double value) { m_controller.setMediaVolume(value); }
QString MediaContext::mediaRepeatMode() const { return m_controller.mediaRepeatMode(); }
void MediaContext::setMediaRepeatMode(const QString &value) { m_controller.setMediaRepeatMode(value); }
QVariantList MediaContext::audioOutputs() const { return m_controller.audioOutputs(); }
QString MediaContext::audioOutputId() const { return m_controller.audioOutputId(); }
void MediaContext::setAudioOutputId(const QString &id) { m_controller.setAudioOutputId(id); }
bool MediaContext::audioOutputConfigured() const { return m_controller.audioOutputConfigured(); }
QVariantList MediaContext::onlineLyricsResults() const { return m_onlineLyrics.results(); }
bool MediaContext::onlineLyricsBusy() const { return m_onlineLyrics.busy(); }
QString MediaContext::onlineLyricsError() const { return m_onlineLyrics.error(); }
QString MediaContext::onlineLyricsStatus() const { return m_onlineLyricsStatus; }
bool MediaContext::vagalumeApiKeyConfigured() const
{
    return !m_controller.vagalumeApiKey().isEmpty();
}
bool MediaContext::lyricsSecretStoragePersistent() const
{
    return m_controller.secretStoragePersistent();
}
QString MediaContext::lyricsSecretStorageName() const
{
    return m_controller.secretStorageName();
}
int MediaContext::importAudioFiles(const QVariantList &urls) { return m_controller.importAudioFiles(urls); }
int MediaContext::importVideoFiles(const QVariantList &urls) { return m_controller.importVideoFiles(urls); }
int MediaContext::importImageFiles(const QVariantList &urls) { return m_controller.importImageFiles(urls); }
void MediaContext::selectSong(const QString &id) { m_controller.selectSong(id); }
bool MediaContext::addMediaFolder(const QUrl &folder) { return m_controller.addMediaFolder(folder); }
void MediaContext::removeMediaFolder(const QString &path) { m_controller.removeMediaFolder(path); }
void MediaContext::rescanMediaFolders() { m_controller.rescanMediaFolders(); }
QString MediaContext::addCatalogFileToPlaylist(const QString &path) { return m_controller.addCatalogFileToPlaylist(path); }
bool MediaContext::isFavoriteMedia(const QString &path) const { return m_controller.isFavoriteMedia(path); }
void MediaContext::toggleFavoriteMedia(const QString &path) { m_controller.toggleFavoriteMedia(path); }
bool MediaContext::openFileLocation(const QString &path) { return m_controller.openFileLocation(path); }
void MediaContext::moveMedia(const QString &id, int index) { m_controller.moveMedia(id, index); }
void MediaContext::removeMedia(const QString &id) { m_controller.removeMedia(id); }
void MediaContext::requestMediaThumbnail(const QString &path, const QString &type)
{
    m_controller.requestMediaThumbnail(path, type);
}
void MediaContext::playMedia(const QString &id) { m_controller.playMedia(id); }
void MediaContext::toggleMediaPause() { m_controller.toggleMediaPause(); }
void MediaContext::stopMedia() { m_controller.stopMedia(); }
void MediaContext::seekMedia(int positionMs) { m_controller.seekMedia(positionMs); }
void MediaContext::previousMedia() { m_controller.previousMedia(); }
void MediaContext::nextMedia() { m_controller.nextMedia(); }
void MediaContext::shuffleMediaPlaylist() { m_controller.shuffleMediaPlaylist(); }
void MediaContext::clearMediaPlaylist() { m_controller.clearMediaPlaylist(); }
bool MediaContext::saveMediaPlaylist(const QUrl &destination) { return m_controller.saveMediaPlaylist(destination); }

void MediaContext::searchOnlineLyrics(const QString &query)
{
    m_onlineLyricsStatus.clear();
    m_onlineLyrics.setVagalumeApiKey(m_controller.vagalumeApiKey());
    m_onlineLyrics.search(query);
    emit onlineLyricsChanged();
}

void MediaContext::cancelOnlineLyricsSearch()
{
    m_onlineLyrics.cancel();
}

void MediaContext::loadOnlineLyrics(const QString &key)
{
    m_onlineLyricsStatus.clear();
    m_onlineLyrics.setVagalumeApiKey(m_controller.vagalumeApiKey());
    m_onlineLyrics.loadLyrics(key);
}

QVariantMap MediaContext::onlineLyricsResult(const QString &key) const
{
    return m_onlineLyrics.result(key);
}

QString MediaContext::saveOnlineLyrics(const QString &key)
{
    const auto item = m_onlineLyrics.result(key);
    if (item.isEmpty()) {
        m_onlineLyricsStatus = tr("O resultado online não está mais disponível.");
        emit onlineLyricsChanged();
        return {};
    }
    if (!item.value(QStringLiteral("hasLyrics")).toBool()) {
        m_pendingQuickSaveKey = key;
        m_onlineLyricsStatus = tr("Baixando a letra completa…");
        emit onlineLyricsChanged();
        loadOnlineLyrics(key);
        return {};
    }
    return persistOnlineLyrics(key, item.value(QStringLiteral("title")).toString(),
                               item.value(QStringLiteral("artist")).toString(),
                               item.value(QStringLiteral("lyrics")).toString());
}

QString MediaContext::saveEditedOnlineLyrics(const QString &key, const QString &title,
                                             const QString &artist, const QString &lyrics)
{
    return persistOnlineLyrics(key, title, artist, lyrics);
}

QString MediaContext::persistOnlineLyrics(const QString &key, const QString &title,
                                          const QString &artist, const QString &lyrics)
{
    const auto structured = OnlineLyricsService::toStructuredLyrics(lyrics);
    if (title.trimmed().isEmpty() || structured.isEmpty()) {
        m_onlineLyricsStatus = tr("Informe o título e uma letra válida antes de salvar.");
        emit onlineLyricsChanged();
        return {};
    }
    const auto id = m_controller.saveSongToLibrary(title, artist, structured);
    if (id.isEmpty()) {
        m_onlineLyricsStatus = tr("Não foi possível salvar a letra na biblioteca.");
        emit onlineLyricsChanged();
        return {};
    }
    m_onlineLyrics.markSaved(key);
    m_onlineLyricsStatus = tr("Letra salva na biblioteca sem alterar a apresentação atual.");
    emit onlineLyricsChanged();
    return id;
}

bool MediaContext::setVagalumeApiKey(const QString &apiKey)
{
    if (!m_controller.storeVagalumeApiKey(apiKey)) {
        m_onlineLyricsStatus = tr("Não foi possível guardar a chave do Vagalume.");
        emit onlineLyricsChanged();
        return false;
    }
    m_onlineLyrics.setVagalumeApiKey(apiKey);
    m_onlineLyricsStatus = m_controller.secretStoragePersistent()
        ? tr("Chave do Vagalume guardada no %1.").arg(m_controller.secretStorageName())
        : tr("Sem cofre persistente: a chave valerá somente nesta sessão.");
    emit onlineLyricsChanged();
    return true;
}

bool MediaContext::clearVagalumeApiKey()
{
    const auto cleared = m_controller.clearVagalumeApiKey();
    m_onlineLyrics.setVagalumeApiKey({});
    m_onlineLyricsStatus = cleared ? tr("Chave do Vagalume removida.")
                                   : tr("Nenhuma chave do Vagalume estava configurada.");
    emit onlineLyricsChanged();
    return cleared;
}

bool MediaContext::openOnlineLyricsSource(const QString &key)
{
    const auto url = QUrl(m_onlineLyrics.result(key).value(QStringLiteral("sourceUrl")).toString());
    if (!url.isValid() || url.scheme() != QStringLiteral("https")) return false;
    return QDesktopServices::openUrl(url);
}

} // namespace churchpresenter
