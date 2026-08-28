#pragma once

#include "app/OnlineLyricsService.h"

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
    Q_PROPERTY(QVariantList onlineLyricsResults READ onlineLyricsResults NOTIFY onlineLyricsChanged)
    Q_PROPERTY(bool onlineLyricsBusy READ onlineLyricsBusy NOTIFY onlineLyricsChanged)
    Q_PROPERTY(QString onlineLyricsError READ onlineLyricsError NOTIFY onlineLyricsChanged)
    Q_PROPERTY(QString onlineLyricsStatus READ onlineLyricsStatus NOTIFY onlineLyricsChanged)
    Q_PROPERTY(bool vagalumeApiKeyConfigured READ vagalumeApiKeyConfigured NOTIFY onlineLyricsChanged)
    Q_PROPERTY(bool lyricsSecretStoragePersistent READ lyricsSecretStoragePersistent NOTIFY onlineLyricsChanged)
    Q_PROPERTY(QString lyricsSecretStorageName READ lyricsSecretStorageName NOTIFY onlineLyricsChanged)

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
    [[nodiscard]] QVariantList onlineLyricsResults() const;
    [[nodiscard]] bool onlineLyricsBusy() const;
    [[nodiscard]] QString onlineLyricsError() const;
    [[nodiscard]] QString onlineLyricsStatus() const;
    [[nodiscard]] bool vagalumeApiKeyConfigured() const;
    [[nodiscard]] bool lyricsSecretStoragePersistent() const;
    [[nodiscard]] QString lyricsSecretStorageName() const;

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
    Q_INVOKABLE void requestMediaThumbnail(const QString &path, const QString &type);
    Q_INVOKABLE void playMedia(const QString &id);
    Q_INVOKABLE void toggleMediaPause();
    Q_INVOKABLE void stopMedia();
    Q_INVOKABLE void seekMedia(int positionMs);
    Q_INVOKABLE void previousMedia();
    Q_INVOKABLE void nextMedia();
    Q_INVOKABLE void shuffleMediaPlaylist();
    Q_INVOKABLE void clearMediaPlaylist();
    Q_INVOKABLE bool saveMediaPlaylist(const QUrl &destination);
    Q_INVOKABLE void searchOnlineLyrics(const QString &query);
    Q_INVOKABLE void cancelOnlineLyricsSearch();
    Q_INVOKABLE void loadOnlineLyrics(const QString &key);
    Q_INVOKABLE QVariantMap onlineLyricsResult(const QString &key) const;
    Q_INVOKABLE QString saveOnlineLyrics(const QString &key);
    Q_INVOKABLE QString saveEditedOnlineLyrics(const QString &key, const QString &title,
                                               const QString &artist, const QString &lyrics);
    Q_INVOKABLE bool setVagalumeApiKey(const QString &apiKey);
    Q_INVOKABLE bool clearVagalumeApiKey();
    Q_INVOKABLE bool openOnlineLyricsSource(const QString &key);

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
    void onlineLyricsChanged();
    void onlineLyricsLoaded(const QString &key);

private:
    QString persistOnlineLyrics(const QString &key, const QString &title,
                                const QString &artist, const QString &lyrics);

    ApplicationController &m_controller;
    OnlineLyricsService m_onlineLyrics;
    QString m_onlineLyricsStatus;
    QString m_pendingQuickSaveKey;
};

} // namespace churchpresenter
