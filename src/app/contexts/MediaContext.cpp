#include "app/contexts/MediaContext.h"

#include "app/ApplicationController.h"

namespace churchpresenter {

MediaContext::MediaContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller)
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
void MediaContext::playMedia(const QString &id) { m_controller.playMedia(id); }
void MediaContext::toggleMediaPause() { m_controller.toggleMediaPause(); }
void MediaContext::stopMedia() { m_controller.stopMedia(); }
void MediaContext::seekMedia(int positionMs) { m_controller.seekMedia(positionMs); }
void MediaContext::previousMedia() { m_controller.previousMedia(); }
void MediaContext::nextMedia() { m_controller.nextMedia(); }
void MediaContext::shuffleMediaPlaylist() { m_controller.shuffleMediaPlaylist(); }
void MediaContext::clearMediaPlaylist() { m_controller.clearMediaPlaylist(); }
bool MediaContext::saveMediaPlaylist(const QUrl &destination) { return m_controller.saveMediaPlaylist(destination); }

} // namespace churchpresenter
