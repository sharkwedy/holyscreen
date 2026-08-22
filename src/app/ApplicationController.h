#pragma once

#include "screens/OutputManager.h"
#include "persistence/SettingsRepository.h"
#include "presentation/Clock.h"
#include "presentation/VideoEngine.h"
#include "presentation/ImagePresentationController.h"
#include "library/MediaRepository.h"
#include "library/PresentationRepository.h"
#include "library/ThemeRepository.h"
#include "library/EventRepository.h"
#include "library/HistoryRepository.h"
#include "library/MediaFolderScanner.h"
#include "app/DataRecoveryService.h"
#include "app/AutosaveCoordinator.h"
#include "app/UpdateChecker.h"
#include "presentation/TextPresentationController.h"
#include "presentation/TimedMediaPlayback.h"
#include "presentation/OverlayController.h"
#include "bible/BibleJsonImporter.h"
#include "bible/BibleReferenceParser.h"
#include "bible/BibleRepository.h"
#include "core/CommandBus.h"
#include "core/EventBus.h"
#include "modules/OutputModule.h"
#include "modules/OverlayCommandModule.h"
#include "modules/MediaCommandModule.h"
#include "modules/UndoCommandModule.h"

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

#include <memory>

namespace churchpresenter {

class ApplicationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList screens READ screens NOTIFY screensChanged)
    Q_PROPERTY(QVariantList outputWindows READ outputWindows NOTIFY outputWindowsChanged)
    Q_PROPERTY(QString wallpaperColor READ wallpaperColor WRITE setWallpaperColor NOTIFY wallpaperColorChanged)
    Q_PROPERTY(QUrl wallpaperSource READ wallpaperSource WRITE setWallpaperSource NOTIFY wallpaperSourceChanged)
    Q_PROPERTY(QString wallpaperFit READ wallpaperFit WRITE setWallpaperFit NOTIFY wallpaperFitChanged)
    Q_PROPERTY(bool clockVisible READ clockVisible WRITE setClockVisible NOTIFY clockVisibleChanged)
    Q_PROPERTY(QString clockPosition READ clockPosition WRITE setClockPosition NOTIFY clockPositionChanged)
    Q_PROPERTY(QString clockText READ clockText NOTIFY clockTextChanged)
    Q_PROPERTY(QString clockFormat READ clockFormat WRITE setClockFormat NOTIFY clockFormatChanged)
    Q_PROPERTY(QString clockFontFamily READ clockFontFamily WRITE setClockFontFamily NOTIFY clockFontFamilyChanged)
    Q_PROPERTY(int clockFontSize READ clockFontSize WRITE setClockFontSize NOTIFY clockFontSizeChanged)
    Q_PROPERTY(QString clockColor READ clockColor WRITE setClockColor NOTIFY clockColorChanged)
    Q_PROPERTY(bool clockFontBold READ clockFontBold WRITE setClockFontBold NOTIFY clockStyleChanged)
    Q_PROPERTY(bool clockFontItalic READ clockFontItalic WRITE setClockFontItalic NOTIFY clockStyleChanged)
    Q_PROPERTY(QString clockBackgroundColor READ clockBackgroundColor WRITE setClockBackgroundColor NOTIFY clockStyleChanged)
    Q_PROPERTY(double clockLineHeight READ clockLineHeight WRITE setClockLineHeight NOTIFY clockStyleChanged)
    Q_PROPERTY(int clockCornerRadius READ clockCornerRadius WRITE setClockCornerRadius NOTIFY clockStyleChanged)
    Q_PROPERTY(double clockTextOpacity READ clockTextOpacity WRITE setClockTextOpacity NOTIFY clockStyleChanged)
    Q_PROPERTY(double clockBackgroundOpacity READ clockBackgroundOpacity WRITE setClockBackgroundOpacity NOTIFY clockStyleChanged)
    Q_PROPERTY(int clockMarginHorizontal READ clockMarginHorizontal WRITE setClockMarginHorizontal NOTIFY clockStyleChanged)
    Q_PROPERTY(int clockMarginVertical READ clockMarginVertical WRITE setClockMarginVertical NOTIFY clockStyleChanged)
    Q_PROPERTY(QString clockEffect READ clockEffect WRITE setClockEffect NOTIFY clockStyleChanged)
    Q_PROPERTY(int simulatedOutputCount READ simulatedOutputCount WRITE setSimulatedOutputCount NOTIFY simulatedOutputCountChanged)
    Q_PROPERTY(bool debugEnabled READ debugEnabled WRITE setDebugEnabled NOTIFY debugOptionsChanged)
    Q_PROPERTY(bool debugSimulatedOutputs READ debugSimulatedOutputs WRITE setDebugSimulatedOutputs NOTIFY debugOptionsChanged)
    Q_PROPERTY(bool debugDiagnostics READ debugDiagnostics WRITE setDebugDiagnostics NOTIFY debugOptionsChanged)
    Q_PROPERTY(bool debugLogging READ debugLogging WRITE setDebugLogging NOTIFY debugOptionsChanged)
    Q_PROPERTY(bool blackout READ blackout NOTIFY blackoutChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStateChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStateChanged)
    Q_PROPERTY(QString undoLabel READ undoLabel NOTIFY undoStateChanged)
    Q_PROPERTY(QString redoLabel READ redoLabel NOTIFY undoStateChanged)
    Q_PROPERTY(bool autosavePending READ autosavePending NOTIFY autosaveChanged)
    Q_PROPERTY(QString autosaveStatus READ autosaveStatus NOTIFY autosaveChanged)
    Q_PROPERTY(bool identifyVisible READ identifyVisible NOTIFY identifyVisibleChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
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
    Q_PROPERTY(QVariantList audioLibrary READ audioLibrary NOTIFY audioLibraryChanged)
    Q_PROPERTY(QString currentAudioId READ currentAudioId NOTIFY currentAudioChanged)
    Q_PROPERTY(QString currentAudioTitle READ currentAudioTitle NOTIFY currentAudioChanged)
    Q_PROPERTY(QString audioState READ audioState NOTIFY audioStateChanged)
    Q_PROPERTY(int audioPositionMs READ audioPositionMs NOTIFY audioPositionChanged)
    Q_PROPERTY(int audioDurationMs READ audioDurationMs NOTIFY audioDurationChanged)
    Q_PROPERTY(double audioVolume READ audioVolume WRITE setAudioVolume NOTIFY audioVolumeChanged)
    Q_PROPERTY(QVariantList videoLibrary READ videoLibrary NOTIFY videoLibraryChanged)
    Q_PROPERTY(QString currentVideoId READ currentVideoId NOTIFY currentVideoChanged)
    Q_PROPERTY(QString currentVideoTitle READ currentVideoTitle NOTIFY currentVideoChanged)
    Q_PROPERTY(QString videoState READ videoState NOTIFY videoStateChanged)
    Q_PROPERTY(int videoPositionMs READ videoPositionMs NOTIFY videoPositionChanged)
    Q_PROPERTY(int videoDurationMs READ videoDurationMs NOTIFY videoDurationChanged)
    Q_PROPERTY(double videoVolume READ videoVolume WRITE setVideoVolume NOTIFY videoVolumeChanged)
    Q_PROPERTY(bool videoLoop READ videoLoop WRITE setVideoLoop NOTIFY videoLoopChanged)
    Q_PROPERTY(bool videoVisible READ videoVisible NOTIFY videoVisibleChanged)
    Q_PROPERTY(QVariantList imageLibrary READ imageLibrary NOTIFY imageLibraryChanged)
    Q_PROPERTY(QString currentImageId READ currentImageId NOTIFY currentImageChanged)
    Q_PROPERTY(QString currentImageTitle READ currentImageTitle NOTIFY currentImageChanged)
    Q_PROPERTY(QUrl presentationImageSource READ presentationImageSource NOTIFY presentationImageSourceChanged)
    Q_PROPERTY(bool imageVisible READ imageVisible NOTIFY imageVisibleChanged)
    Q_PROPERTY(QString imageFit READ imageFit WRITE setImageFit NOTIFY imageFitChanged)
    Q_PROPERTY(QString imageTransition READ imageTransition WRITE setImageTransition NOTIFY imageTransitionChanged)
    Q_PROPERTY(bool imageAutoplay READ imageAutoplay WRITE setImageAutoplay NOTIFY imageAutoplayChanged)
    Q_PROPERTY(int imageIntervalMs READ imageIntervalMs WRITE setImageIntervalMs NOTIFY imageIntervalChanged)
    Q_PROPERTY(QVariantList textPresentations READ textPresentations NOTIFY textPresentationsChanged)
    Q_PROPERTY(QVariantList textSlides READ textSlides NOTIFY textSlidesChanged)
    Q_PROPERTY(QString currentPresentationId READ currentPresentationId NOTIFY currentPresentationChanged)
    Q_PROPERTY(QString currentPresentationTitle READ currentPresentationTitle NOTIFY currentPresentationChanged)
    Q_PROPERTY(int currentSlideIndex READ currentSlideIndex NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentSlideId READ currentSlideId NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentSlideLabel READ currentSlideLabel NOTIFY currentSlideChanged)
    Q_PROPERTY(QString currentSlideText READ currentSlideText NOTIFY currentSlideChanged)
    Q_PROPERTY(QString nextSlideText READ nextSlideText NOTIFY currentSlideChanged)
    Q_PROPERTY(bool textVisible READ textVisible NOTIFY textVisibleChanged)
    Q_PROPERTY(QString stageMessage READ stageMessage WRITE setStageMessage NOTIFY stageMessageChanged)
    Q_PROPERTY(QString audienceMessage READ audienceMessage NOTIFY overlaysChanged)
    Q_PROPERTY(QString alertMessage READ alertMessage NOTIFY overlaysChanged)
    Q_PROPERTY(QString lowerThirdTitle READ lowerThirdTitle NOTIFY overlaysChanged)
    Q_PROPERTY(QString lowerThirdSubtitle READ lowerThirdSubtitle NOTIFY overlaysChanged)
    Q_PROPERTY(QString countdownText READ countdownText NOTIFY overlaysChanged)
    Q_PROPERTY(bool countdownRunning READ countdownRunning NOTIFY overlaysChanged)
    Q_PROPERTY(QString stopwatchText READ stopwatchText NOTIFY overlaysChanged)
    Q_PROPERTY(bool stopwatchRunning READ stopwatchRunning NOTIFY overlaysChanged)
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themesChanged)
    Q_PROPERTY(QVariantMap activeTheme READ activeTheme NOTIFY activeThemeChanged)
    Q_PROPERTY(QVariantList songs READ songs NOTIFY songsChanged)
    Q_PROPERTY(QString songSearch READ songSearch WRITE setSongSearch NOTIFY songSearchChanged)
    Q_PROPERTY(QString songSequence READ songSequence NOTIFY currentPresentationChanged)
    Q_PROPERTY(QString currentPresentationType READ currentPresentationType NOTIFY currentPresentationChanged)
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(QString currentEventId READ currentEventId NOTIFY currentEventChanged)
    Q_PROPERTY(QVariantList eventItems READ eventItems NOTIFY eventItemsChanged)
    Q_PROPERTY(qint64 eventDurationMs READ eventDurationMs NOTIFY eventItemsChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(QVariantMap historyReport READ historyReport NOTIFY historyChanged)
    Q_PROPERTY(QString lastBackupPath READ lastBackupPath NOTIFY maintenanceChanged)
    Q_PROPERTY(bool recoveredFromCrash READ recoveredFromCrash NOTIFY maintenanceChanged)
    Q_PROPERTY(QVariantMap diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QString updateStatus READ updateStatus NOTIFY updateChanged)
    Q_PROPERTY(QString updateEndpoint READ updateEndpoint WRITE setUpdateEndpoint NOTIFY updateChanged)
    Q_PROPERTY(QVariantList bibleTranslations READ bibleTranslations NOTIFY bibleTranslationsChanged)
    Q_PROPERTY(QVariantList bibleBooks READ bibleBooks CONSTANT)
    Q_PROPERTY(QString biblePrimaryTranslationId READ biblePrimaryTranslationId WRITE setBiblePrimaryTranslationId NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QString bibleSecondaryTranslationId READ bibleSecondaryTranslationId WRITE setBibleSecondaryTranslationId NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QString bibleTertiaryTranslationId READ bibleTertiaryTranslationId WRITE setBibleTertiaryTranslationId NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QString bibleReferenceInput READ bibleReferenceInput WRITE setBibleReferenceInput NOTIFY bibleSelectionChanged)
    Q_PROPERTY(QVariantList bibleResults READ bibleResults NOTIFY bibleResultsChanged)

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    [[nodiscard]] CommandBus &commandBus();
    [[nodiscard]] EventBus &eventBus();

    [[nodiscard]] QVariantList screens() const;
    [[nodiscard]] QVariantList outputWindows() const;
    [[nodiscard]] QString wallpaperColor() const;
    [[nodiscard]] QUrl wallpaperSource() const;
    [[nodiscard]] QString wallpaperFit() const;
    [[nodiscard]] bool clockVisible() const;
    [[nodiscard]] QString clockPosition() const;
    [[nodiscard]] QString clockText() const;
    [[nodiscard]] QString clockFormat() const;
    [[nodiscard]] QString clockFontFamily() const;
    [[nodiscard]] int clockFontSize() const;
    [[nodiscard]] QString clockColor() const;
    [[nodiscard]] bool clockFontBold() const;
    [[nodiscard]] bool clockFontItalic() const;
    [[nodiscard]] QString clockBackgroundColor() const;
    [[nodiscard]] double clockLineHeight() const;
    [[nodiscard]] int clockCornerRadius() const;
    [[nodiscard]] double clockTextOpacity() const;
    [[nodiscard]] double clockBackgroundOpacity() const;
    [[nodiscard]] int clockMarginHorizontal() const;
    [[nodiscard]] int clockMarginVertical() const;
    [[nodiscard]] QString clockEffect() const;
    [[nodiscard]] int simulatedOutputCount() const;
    [[nodiscard]] bool debugEnabled() const;
    [[nodiscard]] bool debugSimulatedOutputs() const;
    [[nodiscard]] bool debugDiagnostics() const;
    [[nodiscard]] bool debugLogging() const;
    [[nodiscard]] bool blackout() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    [[nodiscard]] QString undoLabel() const;
    [[nodiscard]] QString redoLabel() const;
    [[nodiscard]] bool autosavePending() const;
    [[nodiscard]] QString autosaveStatus() const;
    [[nodiscard]] bool identifyVisible() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QVariantList mediaPlaylist() const;
    [[nodiscard]] QVariantList mediaFolders() const;
    [[nodiscard]] QVariantList folderAudioFiles() const;
    [[nodiscard]] QVariantList folderVideoFiles() const;
    [[nodiscard]] QVariantList folderImageFiles() const;
    [[nodiscard]] QVariantList favoriteMedia() const;
    [[nodiscard]] QString audioFileSearch() const;
    [[nodiscard]] QString videoFileSearch() const;
    [[nodiscard]] QString imageFileSearch() const;
    [[nodiscard]] QString currentMediaId() const;
    [[nodiscard]] QString currentMediaTitle() const;
    [[nodiscard]] QString currentMediaType() const;
    [[nodiscard]] QString mediaState() const;
    [[nodiscard]] int mediaPositionMs() const;
    [[nodiscard]] int mediaDurationMs() const;
    [[nodiscard]] double mediaVolume() const;
    [[nodiscard]] QString mediaRepeatMode() const;
    [[nodiscard]] QVariantList audioLibrary() const;
    [[nodiscard]] QString currentAudioId() const;
    [[nodiscard]] QString currentAudioTitle() const;
    [[nodiscard]] QString audioState() const;
    [[nodiscard]] int audioPositionMs() const;
    [[nodiscard]] int audioDurationMs() const;
    [[nodiscard]] double audioVolume() const;
    [[nodiscard]] QVariantList videoLibrary() const;
    [[nodiscard]] QString currentVideoId() const;
    [[nodiscard]] QString currentVideoTitle() const;
    [[nodiscard]] QString videoState() const;
    [[nodiscard]] int videoPositionMs() const;
    [[nodiscard]] int videoDurationMs() const;
    [[nodiscard]] double videoVolume() const;
    [[nodiscard]] bool videoLoop() const;
    [[nodiscard]] bool videoVisible() const;
    [[nodiscard]] QVariantList imageLibrary() const;
    [[nodiscard]] QString currentImageId() const;
    [[nodiscard]] QString currentImageTitle() const;
    [[nodiscard]] QUrl presentationImageSource() const;
    [[nodiscard]] bool imageVisible() const;
    [[nodiscard]] QString imageFit() const;
    [[nodiscard]] QString imageTransition() const;
    [[nodiscard]] bool imageAutoplay() const;
    [[nodiscard]] int imageIntervalMs() const;
    [[nodiscard]] QVariantList textPresentations() const;
    [[nodiscard]] QVariantList textSlides() const;
    [[nodiscard]] QString currentPresentationId() const;
    [[nodiscard]] QString currentPresentationTitle() const;
    [[nodiscard]] int currentSlideIndex() const;
    [[nodiscard]] QString currentSlideId() const;
    [[nodiscard]] QString currentSlideLabel() const;
    [[nodiscard]] QString currentSlideText() const;
    [[nodiscard]] QString nextSlideText() const;
    [[nodiscard]] bool textVisible() const;
    [[nodiscard]] QString stageMessage() const;
    [[nodiscard]] QString audienceMessage() const;
    [[nodiscard]] QString alertMessage() const;
    [[nodiscard]] QString lowerThirdTitle() const;
    [[nodiscard]] QString lowerThirdSubtitle() const;
    [[nodiscard]] QString countdownText() const;
    [[nodiscard]] bool countdownRunning() const;
    [[nodiscard]] QString stopwatchText() const;
    [[nodiscard]] bool stopwatchRunning() const;
    [[nodiscard]] QVariantList themes() const;
    [[nodiscard]] QVariantMap activeTheme() const;
    [[nodiscard]] QVariantList songs() const;
    [[nodiscard]] QString songSearch() const;
    [[nodiscard]] QString songSequence() const;
    [[nodiscard]] QString currentPresentationType() const;
    [[nodiscard]] QVariantList events() const;
    [[nodiscard]] QString currentEventId() const;
    [[nodiscard]] QVariantList eventItems() const;
    [[nodiscard]] qint64 eventDurationMs() const;
    [[nodiscard]] QVariantList history() const;
    [[nodiscard]] QVariantMap historyReport() const;
    [[nodiscard]] QString lastBackupPath() const;
    [[nodiscard]] bool recoveredFromCrash() const;
    [[nodiscard]] QVariantMap diagnostics() const;
    [[nodiscard]] QString updateStatus() const;
    [[nodiscard]] QString updateEndpoint() const;
    [[nodiscard]] QVariantList bibleTranslations() const;
    [[nodiscard]] QVariantList bibleBooks() const;
    [[nodiscard]] QString biblePrimaryTranslationId() const;
    [[nodiscard]] QString bibleSecondaryTranslationId() const;
    [[nodiscard]] QString bibleTertiaryTranslationId() const;
    [[nodiscard]] QString bibleReferenceInput() const;
    [[nodiscard]] QVariantList bibleResults() const;

    Q_INVOKABLE bool toggleScreen(const QString &screenFingerprint, bool enabled);
    Q_INVOKABLE void enableAllScreens();
    Q_INVOKABLE bool setOutputBibleTranslation(const QString &screenFingerprint, const QString &translationId);
    Q_INVOKABLE bool setOutputRole(const QString &screenFingerprint, const QString &role);
    Q_INVOKABLE bool setOutputMediaEnabled(const QString &screenFingerprint, bool enabled);
    Q_INVOKABLE bool setOutputDisplayName(const QString &screenFingerprint, const QString &displayName);
    Q_INVOKABLE void setBlackout(bool enabled);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void identifyScreens();
    Q_INVOKABLE void clearStatusMessage();
    Q_INVOKABLE void moveMedia(const QString &id, int newIndex);
    Q_INVOKABLE bool addMediaFolder(const QUrl &folder);
    Q_INVOKABLE void removeMediaFolder(const QString &folderPath);
    Q_INVOKABLE void rescanMediaFolders();
    Q_INVOKABLE QString addCatalogFileToPlaylist(const QString &path);
    Q_INVOKABLE bool isFavoriteMedia(const QString &path) const;
    Q_INVOKABLE void toggleFavoriteMedia(const QString &path);
    Q_INVOKABLE bool openFileLocation(const QString &path);
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
    Q_INVOKABLE int importAudioFiles(const QVariantList &urls);
    Q_INVOKABLE void removeAudio(const QString &id);
    Q_INVOKABLE void moveAudio(const QString &id, int newIndex);
    Q_INVOKABLE void playAudio(const QString &id);
    Q_INVOKABLE void toggleAudioPause();
    Q_INVOKABLE void stopAudio();
    Q_INVOKABLE void seekAudio(int positionMs);
    Q_INVOKABLE void previousAudio();
    Q_INVOKABLE void nextAudio();
    Q_INVOKABLE int importVideoFiles(const QVariantList &urls);
    Q_INVOKABLE void removeVideo(const QString &id);
    Q_INVOKABLE void playVideo(const QString &id);
    Q_INVOKABLE void toggleVideoPause();
    Q_INVOKABLE void stopVideo();
    Q_INVOKABLE void seekVideo(int positionMs);
    Q_INVOKABLE bool registerVideoSink(QObject *sink);
    Q_INVOKABLE void unregisterVideoSink(QObject *sink);
    Q_INVOKABLE int importImageFiles(const QVariantList &urls);
    Q_INVOKABLE void removeImage(const QString &id);
    Q_INVOKABLE void moveImage(const QString &id, int newIndex);
    Q_INVOKABLE void showImage(const QString &id);
    Q_INVOKABLE void nextImage();
    Q_INVOKABLE void previousImage();
    Q_INVOKABLE void stopImage();
    Q_INVOKABLE QString createTextPresentation(const QString &title);
    Q_INVOKABLE void deleteTextPresentation(const QString &id);
    Q_INVOKABLE void selectTextPresentation(const QString &id);
    Q_INVOKABLE void addTextSlide(const QString &label, const QString &text);
    Q_INVOKABLE void updateTextSlide(const QString &id, const QString &label, const QString &text);
    Q_INVOKABLE void duplicateTextSlide(const QString &id);
    Q_INVOKABLE void splitTextSlide(const QString &id, int cursorPosition);
    Q_INVOKABLE void removeTextSlide(const QString &id);
    Q_INVOKABLE void moveTextSlide(const QString &id, int newIndex);
    Q_INVOKABLE void showTextSlide(int index);
    Q_INVOKABLE void nextTextSlide();
    Q_INVOKABLE void previousTextSlide();
    Q_INVOKABLE void firstTextSlide();
    Q_INVOKABLE void lastTextSlide();
    Q_INVOKABLE void stopTextPresentation();
    Q_INVOKABLE QString createTheme(const QString &name);
    Q_INVOKABLE void updateTheme(const QVariantMap &values);
    Q_INVOKABLE void deleteTheme(const QString &id);
    Q_INVOKABLE void applyTheme(const QString &id);
    Q_INVOKABLE QString createSong(const QString &title, const QString &author, const QString &structuredLyrics, const QString &sequence);
    Q_INVOKABLE void selectSong(const QString &id);
    Q_INVOKABLE void updateSongSequence(const QString &sequence);
    Q_INVOKABLE QString createEvent(const QString &title, const QString &scheduledAt);
    Q_INVOKABLE void selectEvent(const QString &id);
    Q_INVOKABLE void deleteEvent(const QString &id);
    Q_INVOKABLE void addEventItem(const QString &type, const QString &referenceId, const QString &title, qint64 durationMs = 0);
    Q_INVOKABLE void removeEventItem(const QString &id);
    Q_INVOKABLE void moveEventItem(const QString &id, int newIndex);
    Q_INVOKABLE void executeEventItem(const QString &id);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE QString createBackup();
    Q_INVOKABLE bool scheduleRestore(const QUrl &source);
    Q_INVOKABLE void runBenchmark();
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE int importBibleTranslation(const QUrl &source);
    Q_INVOKABLE bool searchBibleReference();
    Q_INVOKABLE void showBibleVerse(int index);
    Q_INVOKABLE QVariantList bibleChapterNumbers(int bookId) const;
    Q_INVOKABLE QVariantList bibleVerseNumbers(int bookId, int chapter) const;
    Q_INVOKABLE bool presentBibleReference(int bookId, int chapter, int verse);
    Q_INVOKABLE QString bibleTextForSlide(int slideIndex, const QString &translationId) const;
    Q_INVOKABLE void setAudienceMessage(const QString &message);
    Q_INVOKABLE void setAlertMessage(const QString &message);
    Q_INVOKABLE void setLowerThird(const QString &title, const QString &subtitle);
    Q_INVOKABLE void startCountdown(int seconds);
    Q_INVOKABLE void stopCountdown();
    Q_INVOKABLE void startStopwatch();
    Q_INVOKABLE void pauseStopwatch();
    Q_INVOKABLE void resetStopwatch();

public slots:
    void setWallpaperColor(const QString &color);
    void setWallpaperSource(const QUrl &source);
    void setWallpaperFit(const QString &fit);
    void setClockVisible(bool visible);
    void setClockPosition(const QString &position);
    void setClockFormat(const QString &format);
    void setClockFontFamily(const QString &family);
    void setClockFontSize(int size);
    void setClockColor(const QString &color);
    void setClockFontBold(bool bold);
    void setClockFontItalic(bool italic);
    void setClockBackgroundColor(const QString &color);
    void setClockLineHeight(double height);
    void setClockCornerRadius(int radius);
    void setClockTextOpacity(double opacity);
    void setClockBackgroundOpacity(double opacity);
    void setClockMarginHorizontal(int margin);
    void setClockMarginVertical(int margin);
    void setClockEffect(const QString &effect);
    void setSimulatedOutputCount(int count);
    void setAudioFileSearch(const QString &search);
    void setVideoFileSearch(const QString &search);
    void setImageFileSearch(const QString &search);
    void setDebugEnabled(bool enabled);
    void setDebugSimulatedOutputs(bool enabled);
    void setDebugDiagnostics(bool enabled);
    void setDebugLogging(bool enabled);
    void setMediaVolume(double volume);
    void setMediaRepeatMode(const QString &mode);
    void setAudioVolume(double volume);
    void setVideoVolume(double volume);
    void setVideoLoop(bool loop);
    void setImageFit(const QString &fit);
    void setImageTransition(const QString &transition);
    void setImageAutoplay(bool enabled);
    void setImageIntervalMs(int intervalMs);
    void setSongSearch(const QString &search);
    void setUpdateEndpoint(const QString &endpoint);
    void setBiblePrimaryTranslationId(const QString &id);
    void setBibleSecondaryTranslationId(const QString &id);
    void setBibleTertiaryTranslationId(const QString &id);
    void setBibleReferenceInput(const QString &reference);
    void setStageMessage(const QString &message);

signals:
    void quickBibleSearchRequested(const QString &initialText);
    void screensChanged();
    void outputWindowsChanged();
    void wallpaperColorChanged();
    void wallpaperSourceChanged();
    void wallpaperFitChanged();
    void clockVisibleChanged();
    void clockPositionChanged();
    void clockTextChanged();
    void clockFormatChanged();
    void clockFontFamilyChanged();
    void clockFontSizeChanged();
    void clockColorChanged();
    void clockStyleChanged();
    void simulatedOutputCountChanged();
    void debugOptionsChanged();
    void blackoutChanged(bool active);
    void undoStateChanged();
    void autosaveChanged();
    void identifyVisibleChanged();
    void statusMessageChanged();
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
    void audioLibraryChanged();
    void currentAudioChanged();
    void audioStateChanged();
    void audioPositionChanged();
    void audioDurationChanged();
    void audioVolumeChanged();
    void videoLibraryChanged();
    void currentVideoChanged();
    void videoStateChanged();
    void videoPositionChanged();
    void videoDurationChanged();
    void videoVolumeChanged();
    void videoLoopChanged();
    void videoVisibleChanged();
    void imageLibraryChanged();
    void currentImageChanged();
    void presentationImageSourceChanged();
    void imageVisibleChanged();
    void imageFitChanged();
    void imageTransitionChanged();
    void imageAutoplayChanged();
    void imageIntervalChanged();
    void textPresentationsChanged();
    void textSlidesChanged();
    void currentPresentationChanged();
    void currentSlideChanged();
    void textVisibleChanged();
    void themesChanged();
    void activeThemeChanged();
    void songsChanged();
    void songSearchChanged();
    void eventsChanged();
    void currentEventChanged();
    void eventItemsChanged();
    void historyChanged();
    void maintenanceChanged();
    void diagnosticsChanged();
    void updateChanged();
    void bibleTranslationsChanged();
    void bibleSelectionChanged();
    void bibleResultsChanged();
    void stageMessageChanged();
    void overlaysChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    CommandBus m_commandBus;
    EventBus m_eventBus;
    UndoManager m_undoManager;
    OutputModule m_outputModule;
    UndoCommandModule m_undoCommands;
    void refreshScreens();
    void loadSettings();
    void saveSetting(const QString &key, const QVariant &value);
    void saveOutputs();
    void setStatusMessage(const QString &message);
    void refreshMediaPlaylist();
    void refreshMediaCatalog();
    void refreshMediaCatalogViews();
    void refreshFavoriteMedia();
    void saveFavoriteMedia();
    void saveMediaFolders();
    void rebuildMediaFolderWatcher();
    void updateCurrentMediaMetadata(const MediaItem &metadata);
    void advanceMediaAfterFinish();
    bool applyPlayMedia(const QString &id);
    bool applyToggleMediaPause();
    bool applyStopMedia();
    bool applySeekMedia(int positionMs);
    bool applyPreviousMedia();
    bool applyNextMedia();
    void refreshAudioLibrary();
    void updateCurrentAudioMetadata(const MediaItem &metadata);
    void refreshVideoLibrary();
    void updateCurrentVideoMetadata(const MediaItem &metadata);
    void refreshImageLibrary();
    void refreshTextPresentations();
    void saveCurrentPresentation();
    bool persistCurrentPresentation();
    void refreshThemes();
    void loadActiveTheme();
    void refreshSongs();
    void refreshEvents();
    void refreshEventItems();
    void refreshHistory();
    void recordHistory(const QString &type,const QString &referenceId,const QString &title);
    void refreshBibleTranslations();

    std::unique_ptr<QtScreenProvider> m_screenProvider;
    std::unique_ptr<ScreenManager> m_screenManager;
    OutputManager m_outputs;
    QVariantList m_screens;
    QString m_wallpaperColor = QStringLiteral("#000000");
    QUrl m_wallpaperSource;
    QString m_wallpaperFit = QStringLiteral("cover");
    bool m_clockVisible = true;
    QString m_clockPosition = QStringLiteral("bottomRight");
    ClockController m_clock;
    VideoEngine m_video;
    TimedMediaPlayback m_stillMedia;
    ImagePresentationController m_images;
    QString m_clockFontFamily;
    int m_clockFontSize = 64;
    QString m_clockColor = QStringLiteral("#ffffff");
    bool m_clockFontBold = true;
    bool m_clockFontItalic = false;
    QString m_clockBackgroundColor = QStringLiteral("#000000");
    double m_clockLineHeight = 1.0;
    int m_clockCornerRadius = 12;
    double m_clockTextOpacity = 1.0;
    double m_clockBackgroundOpacity = 0.5;
    int m_clockMarginHorizontal = 0;
    int m_clockMarginVertical = 0;
    QString m_clockEffect = QStringLiteral("outline");
    int m_simulatedOutputCount = 2;
    bool m_debugEnabled = false;
    bool m_debugSimulatedOutputs = true;
    bool m_debugDiagnostics = true;
    bool m_debugLogging = false;
    bool m_identifyVisible = false;
    QString m_statusMessage;
    std::unique_ptr<SettingsRepository> m_settings;
    std::unique_ptr<MediaRepository> m_mediaRepository;
    std::unique_ptr<PresentationRepository> m_presentationRepository;
    std::unique_ptr<ThemeRepository> m_themeRepository;
    std::unique_ptr<EventRepository> m_eventRepository;
    std::unique_ptr<HistoryRepository> m_historyRepository;
    std::unique_ptr<DataRecoveryService> m_recovery;
    std::unique_ptr<AutosaveCoordinator> m_autosave;
    QString m_autosaveStatus = QStringLiteral("Salvo");
    QVariantList m_mediaPlaylist;
    MediaFolderScanner m_mediaFolderScanner;
    QFileSystemWatcher m_mediaFolderWatcher;
    QTimer m_mediaCatalogDebounce;
    QStringList m_mediaFolderPaths;
    QVector<MediaCatalogEntry> m_mediaCatalogEntries;
    QVariantList m_folderAudioFiles;
    QVariantList m_folderVideoFiles;
    QVariantList m_folderImageFiles;
    QStringList m_favoriteMediaPaths;
    QVariantList m_favoriteMedia;
    QString m_audioFileSearch;
    QString m_videoFileSearch;
    QString m_imageFileSearch;
    QString m_currentMediaId;
    QString m_mediaRepeatMode = QStringLiteral("off");
    std::unique_ptr<MediaCommandModule> m_mediaCommands;
    QVariantList m_audioLibrary;
    QString m_currentAudioId;
    QVariantList m_videoLibrary;
    QString m_currentVideoId;
    bool m_videoVisible = false;
    QVariantList m_imageLibrary;
    TextPresentationController m_textPresentation;
    OverlayController m_overlays;
    std::unique_ptr<OverlayCommandModule> m_overlayCommands;
    QVariantList m_textPresentations;
    QVariantList m_themes;
    Theme m_activeTheme;
    QVariantList m_songs;
    QString m_songSearch;
    QVariantList m_events;
    QVariantList m_eventItems;
    QString m_currentEventId;
    QVariantList m_history;
    QString m_dataDirectory;
    QString m_lastBackupPath;
    QVariantMap m_diagnostics;
    UpdateChecker m_updateChecker;
    QString m_updateStatus=QStringLiteral("Não verificado");
    QString m_updateEndpoint;
    std::unique_ptr<BibleRepository> m_bibleRepository;
    BibleReferenceParser m_bibleReferenceParser;
    BibleJsonImporter m_bibleJsonImporter;
    QVariantList m_bibleTranslations;
    QString m_biblePrimaryTranslationId;
    QString m_bibleSecondaryTranslationId;
    QString m_bibleTertiaryTranslationId;
    QString m_bibleReferenceInput;
    QVariantList m_bibleResults;
    BibleReference m_currentBibleReference;
    QString m_stageMessage;
};

} // namespace churchpresenter
