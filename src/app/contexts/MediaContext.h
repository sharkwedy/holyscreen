#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>

namespace churchpresenter {

class ApplicationController;

class MediaContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList songs READ songs NOTIFY songsChanged)
    Q_PROPERTY(QString songSearch READ songSearch WRITE setSongSearch NOTIFY songSearchChanged)
    Q_PROPERTY(QVariantList mediaPlaylist READ mediaPlaylist NOTIFY mediaPlaylistChanged)
    Q_PROPERTY(QVariantList mediaFolders READ mediaFolders NOTIFY mediaFoldersChanged)
    Q_PROPERTY(QVariantList folderAudioFiles READ folderAudioFiles NOTIFY mediaCatalogChanged)
    Q_PROPERTY(QVariantList folderVideoFiles READ folderVideoFiles NOTIFY mediaCatalogChanged)
    Q_PROPERTY(QVariantList folderImageFiles READ folderImageFiles NOTIFY mediaCatalogChanged)
    Q_PROPERTY(QVariantList favoriteMedia READ favoriteMedia NOTIFY favoriteMediaChanged)
    Q_PROPERTY(QString audioFileSearch READ audioFileSearch WRITE setAudioFileSearch NOTIFY mediaCatalogChanged)
    Q_PROPERTY(QString videoFileSearch READ videoFileSearch WRITE setVideoFileSearch NOTIFY mediaCatalogChanged)
    Q_PROPERTY(QString imageFileSearch READ imageFileSearch WRITE setImageFileSearch NOTIFY mediaCatalogChanged)
    Q_PROPERTY(QString currentMediaId READ currentMediaId NOTIFY currentMediaChanged)
    Q_PROPERTY(QString currentMediaTitle READ currentMediaTitle NOTIFY currentMediaChanged)
    Q_PROPERTY(QString currentMediaType READ currentMediaType NOTIFY currentMediaChanged)
    Q_PROPERTY(QString mediaState READ mediaState NOTIFY mediaStateChanged)
    Q_PROPERTY(int mediaPositionMs READ mediaPositionMs NOTIFY mediaPositionChanged)
    Q_PROPERTY(int mediaDurationMs READ mediaDurationMs NOTIFY mediaDurationChanged)
    Q_PROPERTY(double mediaVolume READ mediaVolume WRITE setMediaVolume NOTIFY mediaVolumeChanged)
    Q_PROPERTY(QString mediaRepeatMode READ mediaRepeatMode WRITE setMediaRepeatMode NOTIFY mediaRepeatModeChanged)
    Q_PROPERTY(QVariantList audioOutputs READ audioOutputs NOTIFY audioOutputsChanged)
    Q_PROPERTY(QString audioOutputId READ audioOutputId WRITE setAudioOutputId NOTIFY audioOutputsChanged)
    Q_PROPERTY(bool audioOutputConfigured READ audioOutputConfigured NOTIFY audioOutputsChanged)

public:
    explicit MediaContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QVariantList songs() const;
    [[nodiscard]] QString songSearch() const;
    void setSongSearch(const QString &search);
    [[nodiscard]] QVariantList mediaPlaylist() const;
    [[nodiscard]] QVariantList mediaFolders() const;
    [[nodiscard]] QVariantList folderAudioFiles() const;
    [[nodiscard]] QVariantList folderVideoFiles() const;
    [[nodiscard]] QVariantList folderImageFiles() const;
    [[nodiscard]] QVariantList favoriteMedia() const;
    [[nodiscard]] QString audioFileSearch() const;
    void setAudioFileSearch(const QString &search);
    [[nodiscard]] QString videoFileSearch() const;
    void setVideoFileSearch(const QString &search);
    [[nodiscard]] QString imageFileSearch() const;
    void setImageFileSearch(const QString &search);
    [[nodiscard]] QString currentMediaId() const;
    [[nodiscard]] QString currentMediaTitle() const;
    [[nodiscard]] QString currentMediaType() const;
    [[nodiscard]] QString mediaState() const;
    [[nodiscard]] int mediaPositionMs() const;
    [[nodiscard]] int mediaDurationMs() const;
    [[nodiscard]] double mediaVolume() const;
    void setMediaVolume(double volume);
    [[nodiscard]] QString mediaRepeatMode() const;
    void setMediaRepeatMode(const QString &mode);
    [[nodiscard]] QVariantList audioOutputs() const;
    [[nodiscard]] QString audioOutputId() const;
    void setAudioOutputId(const QString &id);
    [[nodiscard]] bool audioOutputConfigured() const;

    Q_INVOKABLE int importAudioFiles(const QVariantList &urls);
    Q_INVOKABLE int importVideoFiles(const QVariantList &urls);
    Q_INVOKABLE int importImageFiles(const QVariantList &urls);
    Q_INVOKABLE void selectSong(const QString &id);
    Q_INVOKABLE bool addMediaFolder(const QUrl &folder);
    Q_INVOKABLE void removeMediaFolder(const QString &folderPath);
    Q_INVOKABLE void rescanMediaFolders();
    Q_INVOKABLE QString addCatalogFileToPlaylist(const QString &path);
    Q_INVOKABLE bool isFavoriteMedia(const QString &path) const;
    Q_INVOKABLE void toggleFavoriteMedia(const QString &path);
    Q_INVOKABLE bool openFileLocation(const QString &path);
    Q_INVOKABLE void moveMedia(const QString &id, int newIndex);
    Q_INVOKABLE void removeMedia(const QString &id);
    Q_INVOKABLE void playMedia(const QString &id);
    Q_INVOKABLE void toggleMediaPause();
    Q_INVOKABLE void stopMedia();
    Q_INVOKABLE void seekMedia(int positionMs);
    Q_INVOKABLE void previousMedia();
    Q_INVOKABLE void nextMedia();
    Q_INVOKABLE void shuffleMediaPlaylist();
    Q_INVOKABLE void clearMediaPlaylist();
    Q_INVOKABLE bool saveMediaPlaylist(const QUrl &destination);

signals:
    void songsChanged();
    void songSearchChanged();
    void mediaPlaylistChanged();
    void mediaFoldersChanged();
    void mediaCatalogChanged();
    void favoriteMediaChanged();
    void currentMediaChanged();
    void mediaStateChanged();
    void mediaPositionChanged();
    void mediaDurationChanged();
    void mediaVolumeChanged();
    void mediaRepeatModeChanged();
    void audioOutputsChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
