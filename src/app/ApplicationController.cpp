#include "app/ApplicationController.h"
#include "app/AppLogger.h"
#include "persistence/ApplicationDatabase.h"
#include "screens/OutputRouting.h"

#include <algorithm>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantMap>
#include <QRegularExpression>
#include <QUuid>
#include <QHash>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QSysInfo>
#include <QDateTime>
#include <QDirIterator>
#include <QSet>

namespace churchpresenter {

namespace {
QVariantMap mapScreen(const ScreenDescriptor &screen, bool selected)
{
    return {
        {QStringLiteral("id"), screen.fingerprint},
        {QStringLiteral("screenName"), screen.id},
        {QStringLiteral("name"), screen.displayName},
        {QStringLiteral("selected"), selected},
        {QStringLiteral("primary"), screen.primary},
        {QStringLiteral("fingerprint"), screen.fingerprint},
    };
}

QVariantMap mapMedia(const MediaItem &item)
{
    const auto type = item.type == MediaType::Video ? QStringLiteral("video")
                    : item.type == MediaType::Image ? QStringLiteral("image")
                    : QStringLiteral("audio");
    return {
        {QStringLiteral("id"), item.id},
        {QStringLiteral("title"), item.title},
        {QStringLiteral("path"), item.path},
        {QStringLiteral("durationMs"), item.durationMs},
        {QStringLiteral("artist"), item.artist},
        {QStringLiteral("album"), item.album},
        {QStringLiteral("type"), type},
        {QStringLiteral("typeLabel"), type == QStringLiteral("video")
             ? QStringLiteral("VÍDEO") : type == QStringLiteral("audio")
             ? QStringLiteral("ÁUDIO") : QStringLiteral("IMAGEM")},
    };
}

QVariantMap mapCatalogEntry(const MediaCatalogEntry &entry, bool inPlaylist)
{
    const auto type = entry.type == MediaType::Video ? QStringLiteral("video")
                    : entry.type == MediaType::Image ? QStringLiteral("image")
                    : QStringLiteral("audio");
    return {
        {QStringLiteral("fileName"), entry.fileName},
        {QStringLiteral("title"), entry.title},
        {QStringLiteral("path"), entry.path},
        {QStringLiteral("folderPath"), entry.folderPath},
        {QStringLiteral("type"), type},
        {QStringLiteral("inPlaylist"), inPlaylist},
    };
}

QVariantMap mapBibleTranslation(const BibleTranslation &translation)
{
    return {
        {QStringLiteral("id"), translation.id},
        {QStringLiteral("name"), translation.name},
        {QStringLiteral("abbreviation"), translation.abbreviation},
        {QStringLiteral("language"), translation.language},
        {QStringLiteral("displayName"), QStringLiteral("%1 — %2")
             .arg(translation.abbreviation, translation.name)},
    };
}

QString outputRoleName(OutputRole role)
{
    return role == OutputRole::Stage ? QStringLiteral("stage") : QStringLiteral("audience");
}

OutputRole outputRoleFromName(const QString &name)
{
    return name == QStringLiteral("stage") ? OutputRole::Stage : OutputRole::Audience;
}

QString videoStateName(VideoState state)
{
    switch (state) {
    case VideoState::Stopped: return QStringLiteral("stopped");
    case VideoState::Loading: return QStringLiteral("loading");
    case VideoState::Ready: return QStringLiteral("ready");
    case VideoState::Playing: return QStringLiteral("playing");
    case VideoState::Paused: return QStringLiteral("paused");
    case VideoState::Buffering: return QStringLiteral("buffering");
    case VideoState::Error: return QStringLiteral("error");
    }
    return QStringLiteral("stopped");
}

QString imageFitName(ImageFit fit)
{
    switch (fit) {
    case ImageFit::Contain: return QStringLiteral("contain");
    case ImageFit::Cover: return QStringLiteral("cover");
    case ImageFit::Stretch: return QStringLiteral("stretch");
    case ImageFit::Center: return QStringLiteral("center");
    }
    return QStringLiteral("contain");
}

QString imageTransitionName(ImageTransition transition)
{
    return transition == ImageTransition::Fade ? QStringLiteral("fade") : QStringLiteral("none");
}

QVariantMap mapTheme(const Theme &t)
{
    return {{"id",t.id},{"name",t.name},{"backgroundType",static_cast<int>(t.backgroundType)},
            {"backgroundColor",t.backgroundColor},{"backgroundImage",t.backgroundImage},
            {"fontFamily",t.fontFamily},{"fontSize",t.fontSize},{"minimumFontSize",t.minimumFontSize},
            {"fontWeight",t.fontWeight},{"textColor",t.textColor},{"horizontalAlignment",t.horizontalAlignment},
            {"verticalAlignment",t.verticalAlignment},{"lineSpacing",t.lineSpacing},{"margin",t.margin},
            {"outline",t.outline},{"outlineColor",t.outlineColor},{"shadow",t.shadow},
            {"shadowColor",t.shadowColor},{"transition",t.transition}};
}
} // namespace

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
    , m_outputModule(m_commandBus, m_eventBus, &m_undoManager)
    , m_undoCommands(m_commandBus, m_eventBus, m_undoManager)
    , m_screenProvider(std::make_unique<QtScreenProvider>())
    , m_screenManager(std::make_unique<ScreenManager>(*m_screenProvider))
    , m_clock(std::make_unique<SystemClock>())
{
    connect(&m_outputModule, &OutputModule::blackoutChanged,
            this, &ApplicationController::blackoutChanged);
    connect(&m_undoManager, &UndoManager::stateChanged,
            this, &ApplicationController::undoStateChanged);
    m_overlayCommands = std::make_unique<OverlayCommandModule>(
        m_commandBus, m_eventBus, m_overlays, this);
    m_mediaCommands = std::make_unique<MediaCommandModule>(
        m_commandBus, m_eventBus,
        MediaCommandModule::Actions{
            .play = [this](const QString &id) { return applyPlayMedia(id); },
            .togglePause = [this] { return applyToggleMediaPause(); },
            .stop = [this] { return applyStopMedia(); },
            .seek = [this](int positionMs) { return applySeekMedia(positionMs); },
            .previous = [this] { return applyPreviousMedia(); },
            .next = [this] { return applyNextMedia(); },
            .stateSnapshot = [this] {
                return QVariantMap{
                    {QStringLiteral("mediaId"), currentMediaId()},
                    {QStringLiteral("mediaType"), currentMediaType()},
                    {QStringLiteral("state"), mediaState()},
                    {QStringLiteral("positionMs"), mediaPositionMs()},
                    {QStringLiteral("durationMs"), mediaDurationMs()},
                    {QStringLiteral("repeatMode"), mediaRepeatMode()},
                };
            },
        }, this);
    m_clockFontFamily = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    m_mediaCatalogDebounce.setSingleShot(true);
    m_mediaCatalogDebounce.setInterval(350);
    connect(&m_mediaCatalogDebounce, &QTimer::timeout,
            this, &ApplicationController::refreshMediaCatalog);
    connect(&m_mediaFolderWatcher, &QFileSystemWatcher::directoryChanged,
            this, [this] { m_mediaCatalogDebounce.start(); });
    loadSettings();
    refreshScreens();

    connect(m_screenManager.get(), &ScreenManager::screenConfigurationChanged,
            this, [this] {
                // OutputWindow instances own QVideoSinks backed by the current
                // screen's Metal context. Release their last frame before QML
                // destroys/recreates windows after a topology change.
                m_video.frameBus().clear();
                refreshScreens();
            });
    connect(&m_clock, &ClockController::textChanged, this, &ApplicationController::clockTextChanged);
    connect(&m_clock, &ClockController::formatChanged, this, &ApplicationController::clockFormatChanged);
    connect(&m_video, &VideoEngine::stateChanged, this, [this] {
        emit mediaStateChanged();
        emit audioStateChanged();
        emit videoStateChanged();
    });
    connect(&m_video, &VideoEngine::positionChanged, this, [this] {
        emit mediaPositionChanged();
        emit audioPositionChanged();
        emit videoPositionChanged();
    });
    connect(&m_video, &VideoEngine::durationChanged, this, [this] {
        emit mediaDurationChanged();
        emit audioDurationChanged();
        emit videoDurationChanged();
    });
    connect(&m_video, &VideoEngine::mediaMetadataChanged,
            this, &ApplicationController::updateCurrentMediaMetadata);
    connect(&m_video, &VideoEngine::playbackFinished,
            this, &ApplicationController::advanceMediaAfterFinish);
    connect(&m_stillMedia, &TimedMediaPlayback::stateChanged, this, [this] {
        emit mediaStateChanged();
        emit mediaPositionChanged();
        emit mediaDurationChanged();
    });
    connect(&m_stillMedia, &TimedMediaPlayback::positionChanged,
            this, &ApplicationController::mediaPositionChanged);
    connect(&m_stillMedia, &TimedMediaPlayback::finished,
            this, &ApplicationController::advanceMediaAfterFinish);
    connect(&m_video, &VideoEngine::errorOccurred, this, [this](VideoError error) {
        if (error == VideoError::None) return;
        m_videoVisible = false;
        emit videoVisibleChanged();
        setStatusMessage(error == VideoError::FileNotFound
                             ? QStringLiteral("O arquivo de mídia não foi encontrado.")
                             : QStringLiteral("Não foi possível reproduzir a mídia selecionada."));
    });
    connect(&m_images, &ImagePresentationController::currentChanged, this, [this] {
        emit currentImageChanged();
        emit presentationImageSourceChanged();
    });
    connect(&m_images, &ImagePresentationController::visibleChanged, this, [this] {
        emit imageVisibleChanged();
    });
    connect(&m_textPresentation, &TextPresentationController::presentationChanged,
            this, &ApplicationController::textSlidesChanged);
    connect(&m_textPresentation, &TextPresentationController::currentSlideChanged, this, [this] {
        emit currentSlideChanged();
    });
    connect(&m_textPresentation, &TextPresentationController::visibleChanged, this, [this] {
        emit textVisibleChanged();
    });
    connect(&m_overlays, &OverlayController::changed, this, &ApplicationController::overlaysChanged);
    connect(&m_updateChecker,&UpdateChecker::completed,this,[this](const QString&latest,const QUrl&url,bool available,const QString&error){
        if(!error.isEmpty())m_updateStatus=QStringLiteral("Falha ao verificar: %1").arg(error);
        else if(available)m_updateStatus=QStringLiteral("Versão %1 disponível: %2").arg(latest,url.toString());
        else m_updateStatus=QStringLiteral("HolyScreen está atualizado (%1).").arg(QCoreApplication::applicationVersion());
        emit updateChanged();
    });
}

ApplicationController::~ApplicationController(){if(m_recovery)m_recovery->endSession();}

CommandBus &ApplicationController::commandBus() { return m_commandBus; }
EventBus &ApplicationController::eventBus() { return m_eventBus; }

QVariantList ApplicationController::screens() const { return m_screens; }

QVariantList ApplicationController::outputWindows() const
{
    QVariantList result;
    int identifier = 1;
    const auto placements = routeOutputs(m_outputs.activeOutputs(), m_screenManager->screens());
    for (const auto &placement : placements) {
        result.append(QVariantMap{
            {QStringLiteral("id"), placement.screenFingerprint},
            {QStringLiteral("screenName"), placement.screenId},
            {QStringLiteral("displayName"), placement.displayName},
            {QStringLiteral("screenIndex"), placement.screenIndex},
            {QStringLiteral("screenX"), placement.geometry.x()},
            {QStringLiteral("screenY"), placement.geometry.y()},
            {QStringLiteral("screenWidth"), placement.geometry.width()},
            {QStringLiteral("screenHeight"), placement.geometry.height()},
            {QStringLiteral("identifier"), identifier++},
            {QStringLiteral("bibleTranslationId"), placement.bibleTranslationId},
            {QStringLiteral("role"), outputRoleName(placement.role)},
        });
    }
    return result;
}

QString ApplicationController::wallpaperColor() const { return m_wallpaperColor; }
QUrl ApplicationController::wallpaperSource() const { return m_wallpaperSource; }
QString ApplicationController::wallpaperFit() const { return m_wallpaperFit; }
bool ApplicationController::clockVisible() const { return m_clockVisible; }
QString ApplicationController::clockPosition() const { return m_clockPosition; }
QString ApplicationController::clockText() const { return m_clock.text(); }
QString ApplicationController::clockFormat() const { return m_clock.format(); }
QString ApplicationController::clockFontFamily() const { return m_clockFontFamily; }
int ApplicationController::clockFontSize() const { return m_clockFontSize; }
QString ApplicationController::clockColor() const { return m_clockColor; }
int ApplicationController::simulatedOutputCount() const { return m_simulatedOutputCount; }
bool ApplicationController::debugEnabled() const { return m_debugEnabled; }
bool ApplicationController::debugSimulatedOutputs() const { return m_debugSimulatedOutputs; }
bool ApplicationController::debugDiagnostics() const { return m_debugDiagnostics; }
bool ApplicationController::debugLogging() const { return m_debugLogging; }
bool ApplicationController::blackout() const { return m_outputModule.blackout(); }
bool ApplicationController::canUndo() const { return m_undoManager.canUndo(); }
bool ApplicationController::canRedo() const { return m_undoManager.canRedo(); }
QString ApplicationController::undoLabel() const { return m_undoManager.undoLabel(); }
QString ApplicationController::redoLabel() const { return m_undoManager.redoLabel(); }
bool ApplicationController::identifyVisible() const { return m_identifyVisible; }
QString ApplicationController::statusMessage() const { return m_statusMessage; }
QVariantList ApplicationController::mediaPlaylist() const { return m_mediaPlaylist; }
QVariantList ApplicationController::mediaFolders() const
{
    QVariantList result;
    for (const auto &path : m_mediaFolderPaths) {
        const QFileInfo info(path);
        result.append(QVariantMap{
            {QStringLiteral("path"), path},
            {QStringLiteral("name"), info.fileName().isEmpty() ? path : info.fileName()},
            {QStringLiteral("exists"), info.isDir()},
        });
    }
    return result;
}
QVariantList ApplicationController::folderAudioFiles() const { return m_folderAudioFiles; }
QVariantList ApplicationController::folderVideoFiles() const { return m_folderVideoFiles; }
QVariantList ApplicationController::folderImageFiles() const { return m_folderImageFiles; }
QString ApplicationController::audioFileSearch() const { return m_audioFileSearch; }
QString ApplicationController::videoFileSearch() const { return m_videoFileSearch; }
QString ApplicationController::imageFileSearch() const { return m_imageFileSearch; }
QString ApplicationController::currentMediaId() const { return m_currentMediaId; }

QString ApplicationController::currentMediaTitle() const
{
    for (const auto &entry : m_mediaPlaylist) {
        const auto item = entry.toMap();
        if (item.value(QStringLiteral("id")).toString() == m_currentMediaId) {
            return item.value(QStringLiteral("title")).toString();
        }
    }
    return {};
}

QString ApplicationController::currentMediaType() const
{
    for (const auto &entry : m_mediaPlaylist) {
        const auto item = entry.toMap();
        if (item.value(QStringLiteral("id")).toString() == m_currentMediaId) {
            return item.value(QStringLiteral("type")).toString();
        }
    }
    return {};
}

QString ApplicationController::mediaState() const
{
    if (currentMediaType() == QStringLiteral("image")) {
        return m_stillMedia.playing() ? QStringLiteral("playing")
             : m_stillMedia.active() ? QStringLiteral("paused") : QStringLiteral("stopped");
    }
    return videoStateName(m_video.state());
}
int ApplicationController::mediaPositionMs() const
{
    return currentMediaType() == QStringLiteral("image")
        ? m_stillMedia.positionMs() : m_video.position().totalMilliseconds();
}
int ApplicationController::mediaDurationMs() const
{
    return currentMediaType() == QStringLiteral("image")
        ? m_stillMedia.durationMs() : m_video.duration().totalMilliseconds();
}
double ApplicationController::mediaVolume() const { return m_video.volume(); }
QString ApplicationController::mediaRepeatMode() const { return m_mediaRepeatMode; }
QVariantList ApplicationController::audioLibrary() const { return m_audioLibrary; }
QString ApplicationController::currentAudioId() const { return m_currentAudioId; }

QString ApplicationController::currentAudioTitle() const
{
    for (const auto &entry : m_audioLibrary) {
        const auto item = entry.toMap();
        if (item.value(QStringLiteral("id")).toString() == m_currentAudioId) {
            return item.value(QStringLiteral("title")).toString();
        }
    }
    return {};
}

QString ApplicationController::audioState() const { return mediaState(); }
int ApplicationController::audioPositionMs() const { return mediaPositionMs(); }
int ApplicationController::audioDurationMs() const { return mediaDurationMs(); }
double ApplicationController::audioVolume() const { return mediaVolume(); }
QVariantList ApplicationController::videoLibrary() const { return m_videoLibrary; }
QString ApplicationController::currentVideoId() const { return m_currentVideoId; }

QString ApplicationController::currentVideoTitle() const
{
    for (const auto &entry : m_videoLibrary) {
        const auto item = entry.toMap();
        if (item.value(QStringLiteral("id")).toString() == m_currentVideoId) {
            return item.value(QStringLiteral("title")).toString();
        }
    }
    return {};
}

QString ApplicationController::videoState() const { return videoStateName(m_video.state()); }
int ApplicationController::videoPositionMs() const { return m_video.position().totalMilliseconds(); }
int ApplicationController::videoDurationMs() const { return m_video.duration().totalMilliseconds(); }
double ApplicationController::videoVolume() const { return m_video.volume(); }
bool ApplicationController::videoLoop() const { return m_video.loop(); }
bool ApplicationController::videoVisible() const { return m_videoVisible; }
QVariantList ApplicationController::imageLibrary() const { return m_imageLibrary; }
QString ApplicationController::currentImageId() const { return m_images.current().id; }
QString ApplicationController::currentImageTitle() const { return m_images.current().title; }
QUrl ApplicationController::presentationImageSource() const
{
    const auto path = m_images.current().path;
    return path.isEmpty() ? QUrl{} : QUrl::fromLocalFile(path);
}
bool ApplicationController::imageVisible() const { return m_images.visible(); }
QString ApplicationController::imageFit() const { return imageFitName(m_images.fit()); }
QString ApplicationController::imageTransition() const { return imageTransitionName(m_images.transition()); }
bool ApplicationController::imageAutoplay() const { return m_images.autoplay(); }
int ApplicationController::imageIntervalMs() const { return m_images.autoplayIntervalMs(); }
QVariantList ApplicationController::textPresentations() const { return m_textPresentations; }
QVariantList ApplicationController::textSlides() const
{
    QVariantList result;
    for (const auto &slide : m_textPresentation.presentation().slides) {
        result.append(QVariantMap{{"id",slide.id},{"label",slide.label},{"text",slide.text},{"order",slide.order}});
    }
    return result;
}
QString ApplicationController::currentPresentationId() const { return m_textPresentation.presentation().id; }
QString ApplicationController::currentPresentationTitle() const { return m_textPresentation.presentation().title; }
int ApplicationController::currentSlideIndex() const { return m_textPresentation.currentIndex(); }
QString ApplicationController::currentSlideId() const { return m_textPresentation.currentSlide().id; }
QString ApplicationController::currentSlideLabel() const { return m_textPresentation.currentSlide().label; }
QString ApplicationController::currentSlideText() const { return m_textPresentation.currentSlide().text; }
QString ApplicationController::nextSlideText() const { return m_textPresentation.nextSlide().text; }
bool ApplicationController::textVisible() const { return m_textPresentation.visible(); }
QString ApplicationController::stageMessage() const { return m_stageMessage; }
QString ApplicationController::audienceMessage() const { return m_overlays.message(); }
QString ApplicationController::alertMessage() const { return m_overlays.alert(); }
QString ApplicationController::lowerThirdTitle() const { return m_overlays.lowerThirdTitle(); }
QString ApplicationController::lowerThirdSubtitle() const { return m_overlays.lowerThirdSubtitle(); }
QString ApplicationController::countdownText() const { return m_overlays.countdownText(); }
bool ApplicationController::countdownRunning() const { return m_overlays.countdownRunning(); }
QString ApplicationController::stopwatchText() const { return m_overlays.stopwatchText(); }
bool ApplicationController::stopwatchRunning() const { return m_overlays.stopwatchRunning(); }
QVariantList ApplicationController::themes() const { return m_themes; }
QVariantMap ApplicationController::activeTheme() const { return mapTheme(m_activeTheme); }
QVariantList ApplicationController::songs() const { return m_songs; }
QString ApplicationController::songSearch() const { return m_songSearch; }
QString ApplicationController::songSequence() const
{
    QStringList labels;
    const auto &presentation=m_textPresentation.presentation();
    for(const auto&id:presentation.sequence){for(const auto&s:presentation.slides)if(s.id==id){labels.append(s.label);break;}}
    return labels.join(QLatin1Char(' '));
}
QString ApplicationController::currentPresentationType() const
{
    if (currentPresentationId().isEmpty()) return {};
    if (m_textPresentation.presentation().type == PresentationType::Song) return QStringLiteral("song");
    if (m_textPresentation.presentation().type == PresentationType::Bible) return QStringLiteral("bible");
    return QStringLiteral("text");
}
QVariantList ApplicationController::events() const{return m_events;}
QString ApplicationController::currentEventId() const{return m_currentEventId;}
QVariantList ApplicationController::eventItems() const{return m_eventItems;}
qint64 ApplicationController::eventDurationMs() const{return m_eventRepository&&!m_currentEventId.isEmpty()?m_eventRepository->totalDurationMs(m_currentEventId):0;}
QVariantList ApplicationController::history()const{return m_history;}
QVariantMap ApplicationController::historyReport()const
{
    QVariantMap result;if(!m_historyRepository)return result;const auto r=m_historyRepository->report();QVariantMap byType;for(auto it=r.byType.cbegin();it!=r.byType.cend();++it)byType[it.key()]=it.value();result["totalExecutions"]=r.totalExecutions;result["byType"]=byType;result["mostExecutedTitle"]=r.mostExecutedTitle;return result;
}
QString ApplicationController::lastBackupPath()const{return m_lastBackupPath;}
bool ApplicationController::recoveredFromCrash()const{return m_recovery&&m_recovery->recoveredFromCrash();}
QVariantMap ApplicationController::diagnostics()const{return m_diagnostics;}
QString ApplicationController::updateStatus()const{return m_updateStatus;}
QString ApplicationController::updateEndpoint()const{return m_updateEndpoint;}
QVariantList ApplicationController::bibleTranslations() const{return m_bibleTranslations;}
QString ApplicationController::biblePrimaryTranslationId() const{return m_biblePrimaryTranslationId;}
QString ApplicationController::bibleSecondaryTranslationId() const{return m_bibleSecondaryTranslationId;}
QString ApplicationController::bibleTertiaryTranslationId() const{return m_bibleTertiaryTranslationId;}
QString ApplicationController::bibleReferenceInput() const{return m_bibleReferenceInput;}
QVariantList ApplicationController::bibleResults() const{return m_bibleResults;}

bool ApplicationController::toggleScreen(const QString &screenFingerprint, bool enabled)
{
    const auto found = std::find_if(m_screens.cbegin(), m_screens.cend(), [&](const QVariant &entry) {
        return entry.toMap().value(QStringLiteral("id")).toString() == screenFingerprint;
    });
    if (found == m_screens.cend()) {
        return false;
    }

    const auto item = found->toMap();
    if (item.value(QStringLiteral("primary")).toBool()) {
        setStatusMessage(QStringLiteral("A tela principal é reservada para o operador."));
        return false;
    }

    if (enabled) {
        const auto result = m_outputs.enable(ScreenDescriptor{
            .id = item.value(QStringLiteral("screenName")).toString(),
            .fingerprint = item.value(QStringLiteral("fingerprint")).toString(),
            .displayName = item.value(QStringLiteral("name")).toString(),
            .connected = true,
        });
        if (!result.accepted) {
            setStatusMessage(result.reason == EnableOutputResult::LimitReached
                                 ? QStringLiteral("O limite de cinco saídas foi atingido.")
                                 : QStringLiteral("A tela não está mais conectada."));
            return false;
        }
    } else {
        QVector<OutputDescriptor> retained;
        for (const auto &output : m_outputs.activeOutputs()) {
            if (output.screenFingerprint != screenFingerprint) {
                retained.append(output);
            }
        }
        m_outputs.replaceOutputs(retained);
    }

    m_video.frameBus().clear();
    refreshScreens();
    saveOutputs();
    setStatusMessage({});
    return true;
}

void ApplicationController::enableAllScreens()
{
    const auto enabledCount = m_outputs.enableAllAudienceScreens(m_screenManager->screens());
    m_video.frameBus().clear();
    refreshScreens();
    saveOutputs();
    setStatusMessage(enabledCount > 0
                         ? QStringLiteral("Todas as telas externas conectadas estão ativas.")
                         : QStringLiteral("Nenhuma tela externa conectada foi encontrada."));
}

bool ApplicationController::setOutputBibleTranslation(
    const QString &screenFingerprint, const QString &translationId)
{
    if (!translationId.isEmpty()) {
        const auto exists = std::any_of(m_bibleTranslations.cbegin(), m_bibleTranslations.cend(),
                                        [&](const auto &entry) {
            return entry.toMap().value(QStringLiteral("id")).toString() == translationId;
        });
        if (!exists) return false;
    }
    if (!m_outputs.setBibleTranslation(screenFingerprint, translationId)) return false;
    saveOutputs();
    refreshScreens();
    return true;
}

bool ApplicationController::setOutputRole(const QString &screenFingerprint, const QString &role)
{
    const auto normalized = outputRoleFromName(role);
    if (!m_outputs.setRole(screenFingerprint, normalized)) return false;
    saveOutputs();
    refreshScreens();
    return true;
}

void ApplicationController::setBlackout(bool enabled)
{
    m_outputModule.requestBlackout(enabled);
}

void ApplicationController::undo() { m_undoCommands.requestUndo(); }
void ApplicationController::redo() { m_undoCommands.requestRedo(); }

void ApplicationController::identifyScreens()
{
    m_identifyVisible = true;
    emit identifyVisibleChanged();
    QTimer::singleShot(3000, this, [this] {
        if (!m_identifyVisible) return;
        m_identifyVisible = false;
        emit identifyVisibleChanged();
    });
}

void ApplicationController::clearStatusMessage() { setStatusMessage({}); }

bool ApplicationController::addMediaFolder(const QUrl &folder)
{
    const auto requestedPath = folder.isLocalFile() ? folder.toLocalFile() : folder.toString();
    const QFileInfo info(requestedPath);
    const auto canonicalPath = info.canonicalFilePath();
    if (!info.isDir() || canonicalPath.isEmpty()) {
        setStatusMessage(QStringLiteral("A pasta selecionada não está disponível."));
        return false;
    }
    if (m_mediaFolderPaths.contains(canonicalPath)) {
        setStatusMessage(QStringLiteral("Essa pasta já faz parte da biblioteca."));
        return true;
    }
    m_mediaFolderPaths.append(canonicalPath);
    saveMediaFolders();
    emit mediaFoldersChanged();
    refreshMediaCatalog();
    setStatusMessage(QStringLiteral("Pasta adicionada à biblioteca."));
    return true;
}

void ApplicationController::removeMediaFolder(const QString &folderPath)
{
    const auto normalizedPath = QDir::cleanPath(folderPath);
    const auto removed = m_mediaFolderPaths.removeAll(normalizedPath);
    if (removed == 0) return;
    saveMediaFolders();
    emit mediaFoldersChanged();
    refreshMediaCatalog();
    setStatusMessage(QStringLiteral("Pasta removida da biblioteca. Os itens já adicionados à playlist foram mantidos."));
}

void ApplicationController::rescanMediaFolders()
{
    refreshMediaCatalog();
    setStatusMessage(QStringLiteral("Biblioteca de pastas atualizada."));
}

QString ApplicationController::addCatalogFileToPlaylist(const QString &path)
{
    if (!m_mediaRepository) return {};
    const auto catalogEntry = std::find_if(
        m_mediaCatalogEntries.cbegin(), m_mediaCatalogEntries.cend(), [&](const auto &entry) {
            return entry.path == path;
        });
    if (catalogEntry == m_mediaCatalogEntries.cend() || !QFileInfo::exists(path)) {
        setStatusMessage(QStringLiteral("O arquivo não está mais disponível na biblioteca."));
        return {};
    }

    const auto id = m_mediaRepository->add(MediaItem{
        .type = catalogEntry->type,
        .title = catalogEntry->title,
        .path = catalogEntry->path,
        .durationMs = catalogEntry->type == MediaType::Image ? m_images.autoplayIntervalMs() : 0,
    });
    if (id.isEmpty()) {
        setStatusMessage(QStringLiteral("Não foi possível adicionar o arquivo à playlist."));
        return {};
    }

    if (catalogEntry->type == MediaType::Audio) refreshAudioLibrary();
    else if (catalogEntry->type == MediaType::Video) refreshVideoLibrary();
    else refreshImageLibrary();
    refreshMediaPlaylist();
    setStatusMessage(QStringLiteral("%1 adicionado à playlist.").arg(catalogEntry->fileName));
    return id;
}

void ApplicationController::removeMedia(const QString &id)
{
    if (!m_mediaRepository) return;
    const auto item = m_mediaRepository->item(id);
    if (item.type == MediaType::Video) removeVideo(id);
    else if (item.type == MediaType::Image) removeImage(id);
    else removeAudio(id);
}

void ApplicationController::moveMedia(const QString &id, int newIndex)
{
    if (m_mediaRepository && m_mediaRepository->moveInPlaylist(id, newIndex)) {
        refreshMediaPlaylist();
    }
}

void ApplicationController::playMedia(const QString &id)
{
    m_mediaCommands->requestPlay(id);
}

bool ApplicationController::applyPlayMedia(const QString &id)
{
    if (!m_mediaRepository) return false;
    const auto item = m_mediaRepository->item(id);
    if (item.id.isEmpty()) return false;
    if (!QFileInfo::exists(item.path)) {
        setStatusMessage(QStringLiteral("O arquivo não existe mais: %1").arg(item.title));
        return false;
    }

    const auto previousItem = m_mediaRepository->item(m_currentMediaId);
    if (previousItem.type == MediaType::Image) {
        m_stillMedia.stop();
        m_images.stop();
    }
    m_currentMediaId = id;
    const bool isVideo = item.type == MediaType::Video;
    const bool isImage = item.type == MediaType::Image;
    if (isImage) {
        m_video.stop();
        m_currentAudioId.clear();
        m_currentVideoId.clear();
        m_textPresentation.stop();
        if (m_videoVisible) {
            m_videoVisible = false;
            emit videoVisibleChanged();
        }
        if (!m_images.show(id)) {
            setStatusMessage(QStringLiteral("Não foi possível exibir a imagem selecionada."));
            return false;
        }
        m_stillMedia.start(item.durationMs > 0 ? static_cast<int>(item.durationMs)
                                               : m_images.autoplayIntervalMs());
        emit currentMediaChanged();
        emit currentAudioChanged();
        emit currentVideoChanged();
        emit mediaStateChanged();
        emit mediaPositionChanged();
        emit mediaDurationChanged();
        recordHistory(QStringLiteral("image"), id, item.title);
        setStatusMessage({});
        return true;
    }

    m_stillMedia.stop();
    if (isVideo) {
        m_currentVideoId = id;
        m_images.stop();
        m_textPresentation.stop();
    } else {
        m_currentAudioId = id;
    }

    if (m_videoVisible != isVideo) {
        m_videoVisible = isVideo;
        emit videoVisibleChanged();
    }
    emit currentMediaChanged();
    emit currentAudioChanged();
    emit currentVideoChanged();

    m_video.setLoop(m_mediaRepeatMode == QStringLiteral("one"));
    m_video.loadFromPath(item.path);
    m_video.play();
    recordHistory(isVideo ? QStringLiteral("video") : QStringLiteral("audio"), id, item.title);
    setStatusMessage({});
    return true;
}

void ApplicationController::toggleMediaPause()
{
    m_mediaCommands->requestTogglePause();
}

bool ApplicationController::applyToggleMediaPause()
{
    if (currentMediaType() == QStringLiteral("image")) {
        if (m_stillMedia.playing()) m_stillMedia.pause();
        else m_stillMedia.resume();
        return true;
    }
    if (m_video.state() == VideoState::Playing) {
        m_video.pause();
        return true;
    }
    if (m_video.state() == VideoState::Paused || m_video.state() == VideoState::Ready) {
        m_video.play();
        return true;
    }
    return false;
}

void ApplicationController::stopMedia()
{
    m_mediaCommands->requestStop();
}

bool ApplicationController::applyStopMedia()
{
    if (currentMediaType() == QStringLiteral("image")) {
        m_stillMedia.stop();
        m_images.stop();
    }
    m_video.stop();
    if (m_videoVisible) {
        m_videoVisible = false;
        emit videoVisibleChanged();
    }
    return true;
}

void ApplicationController::seekMedia(int positionMs)
{
    m_mediaCommands->requestSeek(positionMs);
}

bool ApplicationController::applySeekMedia(int positionMs)
{
    if (currentMediaType() == QStringLiteral("image")) m_stillMedia.seek(positionMs);
    else m_video.seek(positionMs);
    return true;
}

void ApplicationController::previousMedia()
{
    m_mediaCommands->requestPrevious();
}

bool ApplicationController::applyPreviousMedia()
{
    if (m_mediaPlaylist.isEmpty()) return false;
    int currentIndex = 0;
    for (int index = 0; index < m_mediaPlaylist.size(); ++index) {
        if (m_mediaPlaylist[index].toMap().value(QStringLiteral("id")).toString() == m_currentMediaId) {
            currentIndex = index;
            break;
        }
    }
    const auto target = currentIndex > 0 ? currentIndex - 1 : m_mediaPlaylist.size() - 1;
    return applyPlayMedia(m_mediaPlaylist[target].toMap().value(QStringLiteral("id")).toString());
}

void ApplicationController::nextMedia()
{
    m_mediaCommands->requestNext();
}

bool ApplicationController::applyNextMedia()
{
    if (m_mediaPlaylist.isEmpty()) return false;
    int currentIndex = -1;
    for (int index = 0; index < m_mediaPlaylist.size(); ++index) {
        if (m_mediaPlaylist[index].toMap().value(QStringLiteral("id")).toString() == m_currentMediaId) {
            currentIndex = index;
            break;
        }
    }
    const auto target = (currentIndex + 1) % m_mediaPlaylist.size();
    return applyPlayMedia(m_mediaPlaylist[target].toMap().value(QStringLiteral("id")).toString());
}

void ApplicationController::advanceMediaAfterFinish()
{
    if (m_mediaRepeatMode == QStringLiteral("one")) {
        if (currentMediaType() == QStringLiteral("image")) {
            m_mediaCommands->requestPlay(m_currentMediaId, QStringLiteral("system"));
        }
        return;
    }
    if (m_mediaPlaylist.isEmpty()) {
        m_mediaCommands->requestStop(QStringLiteral("system"));
        return;
    }

    int currentIndex = -1;
    for (int index = 0; index < m_mediaPlaylist.size(); ++index) {
        if (m_mediaPlaylist[index].toMap().value(QStringLiteral("id")).toString() == m_currentMediaId) {
            currentIndex = index;
            break;
        }
    }
    if (currentIndex >= 0 && currentIndex + 1 < m_mediaPlaylist.size()) {
        m_mediaCommands->requestPlay(
            m_mediaPlaylist[currentIndex + 1].toMap().value(QStringLiteral("id")).toString(),
            QStringLiteral("system"));
    } else if (m_mediaRepeatMode == QStringLiteral("all")) {
        m_mediaCommands->requestPlay(
            m_mediaPlaylist.front().toMap().value(QStringLiteral("id")).toString(),
            QStringLiteral("system"));
    } else {
        m_mediaCommands->requestStop(QStringLiteral("system"));
    }
}

int ApplicationController::importAudioFiles(const QVariantList &urls)
{
    if (!m_mediaRepository) return 0;
    static const QStringList supported{
        QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac"), QStringLiteral("m4a"),
        QStringLiteral("aac"), QStringLiteral("ogg"), QStringLiteral("opus"), QStringLiteral("wma"),
        QStringLiteral("aiff"), QStringLiteral("aif"),
    };
    int imported = 0;
    for (const auto &entry : urls) {
        const QUrl url = entry.toUrl();
        const auto path = url.isLocalFile() ? url.toLocalFile() : entry.toString();
        const QFileInfo info(path);
        if (!info.isFile() || !supported.contains(info.suffix().toLower())) continue;
        const auto before = m_mediaRepository->items(MediaType::Audio).size();
        const auto id = m_mediaRepository->add(MediaItem{
            .type = MediaType::Audio,
            .title = info.completeBaseName(),
            .path = info.absoluteFilePath(),
        });
        if (!id.isEmpty() && m_mediaRepository->items(MediaType::Audio).size() > before) ++imported;
    }
    refreshAudioLibrary();
    refreshMediaPlaylist();
    setStatusMessage(imported > 0
                         ? QStringLiteral("%1 áudio(s) importado(s).").arg(imported)
                         : QStringLiteral("Nenhum áudio novo foi importado."));
    return imported;
}

void ApplicationController::removeAudio(const QString &id)
{
    if (!m_mediaRepository) return;
    if (m_currentAudioId == id) {
        if (m_currentMediaId == id) stopMedia();
        m_currentAudioId.clear();
        if (m_currentMediaId == id) m_currentMediaId.clear();
        emit currentAudioChanged();
        emit currentMediaChanged();
    }
    if (m_mediaRepository->remove(id)) {
        refreshAudioLibrary();
        refreshMediaPlaylist();
    }
}

void ApplicationController::moveAudio(const QString &id, int newIndex)
{
    moveMedia(id, newIndex);
}

void ApplicationController::playAudio(const QString &id)
{
    playMedia(id);
}

void ApplicationController::toggleAudioPause()
{
    toggleMediaPause();
}

void ApplicationController::stopAudio() { stopMedia(); }
void ApplicationController::seekAudio(int positionMs) { seekMedia(positionMs); }

void ApplicationController::previousAudio()
{
    previousMedia();
}

void ApplicationController::nextAudio()
{
    nextMedia();
}

int ApplicationController::importVideoFiles(const QVariantList &urls)
{
    if (!m_mediaRepository) return 0;
    static const QStringList supported{
        QStringLiteral("mp4"), QStringLiteral("mov"), QStringLiteral("m4v"), QStringLiteral("mkv"),
        QStringLiteral("webm"), QStringLiteral("avi"), QStringLiteral("wmv"), QStringLiteral("mpeg"),
        QStringLiteral("mpg"),
    };
    int imported = 0;
    for (const auto &entry : urls) {
        const QUrl url = entry.toUrl();
        const auto path = url.isLocalFile() ? url.toLocalFile() : entry.toString();
        const QFileInfo info(path);
        if (!info.isFile() || !supported.contains(info.suffix().toLower())) continue;
        const auto before = m_mediaRepository->items(MediaType::Video).size();
        const auto id = m_mediaRepository->add(MediaItem{
            .type = MediaType::Video,
            .title = info.completeBaseName(),
            .path = info.absoluteFilePath(),
        });
        if (!id.isEmpty() && m_mediaRepository->items(MediaType::Video).size() > before) ++imported;
    }
    refreshVideoLibrary();
    refreshMediaPlaylist();
    setStatusMessage(imported > 0
                         ? QStringLiteral("%1 vídeo(s) importado(s).").arg(imported)
                         : QStringLiteral("Nenhum vídeo novo foi importado."));
    return imported;
}

void ApplicationController::removeVideo(const QString &id)
{
    if (!m_mediaRepository) return;
    if (m_currentVideoId == id) {
        if (m_currentMediaId == id) stopMedia();
        m_currentVideoId.clear();
        if (m_currentMediaId == id) m_currentMediaId.clear();
        emit currentVideoChanged();
        emit currentMediaChanged();
    }
    if (m_mediaRepository->remove(id)) {
        refreshVideoLibrary();
        refreshMediaPlaylist();
    }
}

void ApplicationController::playVideo(const QString &id)
{
    playMedia(id);
}

void ApplicationController::toggleVideoPause()
{
    toggleMediaPause();
}

void ApplicationController::stopVideo()
{
    stopMedia();
}

void ApplicationController::seekVideo(int positionMs) { seekMedia(positionMs); }
bool ApplicationController::registerVideoSink(QObject *sink) { return m_video.frameBus().registerSink(sink); }
void ApplicationController::unregisterVideoSink(QObject *sink) { m_video.frameBus().unregisterSink(sink); }

int ApplicationController::importImageFiles(const QVariantList &urls)
{
    if (!m_mediaRepository) return 0;
    static const QStringList supported{
        QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"), QStringLiteral("webp"),
        QStringLiteral("bmp"), QStringLiteral("gif"), QStringLiteral("tif"), QStringLiteral("tiff"),
        QStringLiteral("heic"),
    };
    int imported = 0;
    for (const auto &entry : urls) {
        const QUrl url = entry.toUrl();
        const auto path = url.isLocalFile() ? url.toLocalFile() : entry.toString();
        const QFileInfo info(path);
        if (!info.isFile() || !supported.contains(info.suffix().toLower())) continue;
        const auto before = m_mediaRepository->items(MediaType::Image).size();
        const auto id = m_mediaRepository->add(MediaItem{
            .type = MediaType::Image,
            .title = info.completeBaseName(),
            .path = info.absoluteFilePath(),
        });
        if (!id.isEmpty() && m_mediaRepository->items(MediaType::Image).size() > before) ++imported;
    }
    refreshImageLibrary();
    refreshMediaPlaylist();
    setStatusMessage(imported > 0
                         ? QStringLiteral("%1 imagem(ns) importada(s).").arg(imported)
                         : QStringLiteral("Nenhuma imagem nova foi importada."));
    return imported;
}

void ApplicationController::removeImage(const QString &id)
{
    if (!m_mediaRepository) return;
    if (m_images.current().id == id) m_images.stop();
    if (m_mediaRepository->remove(id)) {
        if (m_currentMediaId == id) {
            stopMedia();
            m_currentMediaId.clear();
            emit currentMediaChanged();
        }
        refreshImageLibrary();
        refreshMediaPlaylist();
    }
}

void ApplicationController::moveImage(const QString &id, int newIndex)
{
    if (m_mediaRepository && m_mediaRepository->move(id, newIndex)) refreshImageLibrary();
}

void ApplicationController::showImage(const QString &id)
{
    if (!m_mediaRepository) return;
    const auto item = m_mediaRepository->item(id);
    if (item.id.isEmpty() || !QFileInfo::exists(item.path)) {
        setStatusMessage(QStringLiteral("O arquivo de imagem não existe mais."));
        return;
    }
    stopVideo();
    m_textPresentation.stop();
    if (m_images.show(id)) { setStatusMessage({}); recordHistory(QStringLiteral("image"),id,item.title); }
}

void ApplicationController::nextImage() { m_images.next(); }
void ApplicationController::previousImage() { m_images.previous(); }
void ApplicationController::stopImage() { m_images.stop(); }

QString ApplicationController::createTextPresentation(const QString &title)
{
    if (!m_presentationRepository) return {};
    Presentation item{.type = PresentationType::Text, .title = title.trimmed()};
    item.slides.append(Slide{.label = QStringLiteral("1"), .text = QStringLiteral("")});
    const auto id = m_presentationRepository->save(item);
    if (!id.isEmpty()) { refreshTextPresentations(); selectTextPresentation(id); }
    return id;
}

void ApplicationController::deleteTextPresentation(const QString &id)
{
    if (!m_presentationRepository || !m_presentationRepository->remove(id)) return;
    if (currentPresentationId() == id) { m_textPresentation.stop(); m_textPresentation.setPresentation({}); }
    refreshTextPresentations(); refreshSongs(); emit currentPresentationChanged(); emit currentSlideChanged();
}

void ApplicationController::selectTextPresentation(const QString &id)
{
    if (!m_presentationRepository) return;
    const auto item = m_presentationRepository->presentation(id);
    if (item.id.isEmpty()) return;
    m_textPresentation.stop(); m_textPresentation.setPresentation(item);
    loadActiveTheme();
    emit currentPresentationChanged(); emit textSlidesChanged(); emit currentSlideChanged();
}

void ApplicationController::addTextSlide(const QString &label, const QString &text)
{ if (m_textPresentation.addSlide(label, text)) saveCurrentPresentation(); }
void ApplicationController::updateTextSlide(const QString &id, const QString &label, const QString &text)
{ if (m_textPresentation.updateSlide(id, label, text)) saveCurrentPresentation(); }
void ApplicationController::duplicateTextSlide(const QString &id)
{ if (m_textPresentation.duplicateSlide(id)) saveCurrentPresentation(); }
void ApplicationController::splitTextSlide(const QString &id, int cursorPosition)
{ if (m_textPresentation.splitSlide(id, cursorPosition)) saveCurrentPresentation(); }
void ApplicationController::removeTextSlide(const QString &id)
{ if (m_textPresentation.removeSlide(id)) saveCurrentPresentation(); }
void ApplicationController::moveTextSlide(const QString &id, int newIndex)
{ if (m_textPresentation.moveSlide(id, newIndex)) saveCurrentPresentation(); }

void ApplicationController::showTextSlide(int index)
{
    stopVideo(); m_images.stop();
    if(m_textPresentation.show(index)&&index==0){const auto&p=m_textPresentation.presentation();const auto type=p.type==PresentationType::Song?QStringLiteral("song"):p.type==PresentationType::Bible?QStringLiteral("bible"):QStringLiteral("text");recordHistory(type,p.id,p.title);}
}
void ApplicationController::nextTextSlide() { m_textPresentation.next(); }
void ApplicationController::previousTextSlide() { m_textPresentation.previous(); }
void ApplicationController::firstTextSlide() { m_textPresentation.first(); }
void ApplicationController::lastTextSlide() { m_textPresentation.last(); }
void ApplicationController::stopTextPresentation() { m_textPresentation.stop(); }

QString ApplicationController::createTheme(const QString &name)
{
    if (!m_themeRepository) return {};
    Theme theme; theme.name = name.trimmed(); theme.fontFamily = m_clockFontFamily;
    const auto id = m_themeRepository->save(theme);
    if (!id.isEmpty()) { refreshThemes(); applyTheme(id); }
    return id;
}

void ApplicationController::updateTheme(const QVariantMap &v)
{
    if (!m_themeRepository || m_activeTheme.id.isEmpty()) return;
    auto &t=m_activeTheme;
    if(v.contains("name"))t.name=v.value("name").toString();
    if(v.contains("backgroundType"))t.backgroundType=static_cast<BackgroundType>(v.value("backgroundType").toInt());
    if(v.contains("backgroundColor"))t.backgroundColor=v.value("backgroundColor").toString();
    if(v.contains("backgroundImage"))t.backgroundImage=v.value("backgroundImage").toString();
    if(v.contains("fontFamily"))t.fontFamily=v.value("fontFamily").toString();
    if(v.contains("fontSize"))t.fontSize=std::clamp(v.value("fontSize").toInt(),28,240);
    if(v.contains("minimumFontSize"))t.minimumFontSize=std::clamp(v.value("minimumFontSize").toInt(),12,t.fontSize);
    if(v.contains("fontWeight"))t.fontWeight=v.value("fontWeight").toInt();
    if(v.contains("textColor"))t.textColor=v.value("textColor").toString();
    if(v.contains("horizontalAlignment"))t.horizontalAlignment=v.value("horizontalAlignment").toString();
    if(v.contains("verticalAlignment"))t.verticalAlignment=v.value("verticalAlignment").toString();
    if(v.contains("lineSpacing"))t.lineSpacing=v.value("lineSpacing").toInt();
    if(v.contains("margin"))t.margin=std::clamp(v.value("margin").toInt(),0,400);
    if(v.contains("outline"))t.outline=v.value("outline").toBool();
    if(v.contains("outlineColor"))t.outlineColor=v.value("outlineColor").toString();
    if(v.contains("shadow"))t.shadow=v.value("shadow").toBool();
    if(v.contains("shadowColor"))t.shadowColor=v.value("shadowColor").toString();
    if(v.contains("transition"))t.transition=v.value("transition").toString();
    if (!m_themeRepository->save(t).isEmpty()) { refreshThemes(); emit activeThemeChanged(); }
}

void ApplicationController::deleteTheme(const QString &id)
{
    if (!m_themeRepository || !m_themeRepository->remove(id)) return;
    if (m_activeTheme.id==id) m_activeTheme={};
    refreshThemes(); loadActiveTheme();
}

void ApplicationController::applyTheme(const QString &id)
{
    if (!m_themeRepository) return;
    const auto theme=m_themeRepository->theme(id); if(theme.id.isEmpty())return;
    m_activeTheme=theme;
    if(!currentPresentationId().isEmpty()){auto p=m_textPresentation.presentation();p.defaultTheme=id;m_textPresentation.setPresentation(p);saveCurrentPresentation();}
    emit activeThemeChanged();
}

QString ApplicationController::createSong(const QString &title, const QString &author,
                                          const QString &structuredLyrics, const QString &sequenceText)
{
    if(!m_presentationRepository)return{};
    Presentation song{.type=PresentationType::Song,.title=title.trimmed(),.author=author.trimmed(),.defaultTheme=m_activeTheme.id};
    const auto blocks=structuredLyrics.split(QRegularExpression(QStringLiteral("\\n\\s*\\n")),Qt::SkipEmptyParts);
    QHash<QString,QString> sectionIds;
    for(const auto&block:blocks){
        auto lines=block.split(QLatin1Char('\n'));if(lines.isEmpty())continue;
        const auto label=lines.takeFirst().trimmed().toUpper();if(label.isEmpty())continue;
        const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces);sectionIds.insert(label,id);
        song.slides.append(Slide{.id=id,.label=label,.text=lines.join(QLatin1Char('\n')).trimmed(),.order=static_cast<int>(song.slides.size())});
    }
    const auto requested=sequenceText.toUpper().split(QRegularExpression(QStringLiteral("[\\s,;]+")),Qt::SkipEmptyParts);
    for(const auto&label:requested)if(sectionIds.contains(label))song.sequence.append(sectionIds[label]);
    if(song.sequence.isEmpty())for(const auto&s:song.slides)song.sequence.append(s.id);
    if(song.slides.isEmpty())return{};
    const auto id=m_presentationRepository->save(song);if(!id.isEmpty()){refreshSongs();selectSong(id);}return id;
}

void ApplicationController::selectSong(const QString &id)
{
    if(!m_presentationRepository)return;const auto song=m_presentationRepository->presentation(id);
    if(song.id.isEmpty()||song.type!=PresentationType::Song)return;
    m_textPresentation.stop();m_textPresentation.setPresentation(song);loadActiveTheme();
    emit currentPresentationChanged();emit textSlidesChanged();emit currentSlideChanged();
}

void ApplicationController::updateSongSequence(const QString &sequenceText)
{
    auto song=m_textPresentation.presentation();if(song.type!=PresentationType::Song)return;
    QHash<QString,QString> ids;for(const auto&s:song.slides)ids.insert(s.label.toUpper(),s.id);
    QStringList sequence;for(const auto&label:sequenceText.toUpper().split(QRegularExpression("[\\s,;]+"),Qt::SkipEmptyParts))if(ids.contains(label))sequence.append(ids[label]);
    if(sequence.isEmpty())return;song.sequence=sequence;m_textPresentation.setPresentation(song);saveCurrentPresentation();emit currentPresentationChanged();
}

QString ApplicationController::createEvent(const QString &title,const QString &scheduledAt)
{
    if(!m_eventRepository)return{};const auto id=m_eventRepository->saveEvent(Event{.title=title.trimmed(),.scheduledAt=scheduledAt.trimmed()});
    if(!id.isEmpty()){refreshEvents();selectEvent(id);}return id;
}
void ApplicationController::selectEvent(const QString&id)
{if(!m_eventRepository||m_eventRepository->event(id).id.isEmpty())return;m_currentEventId=id;refreshEventItems();emit currentEventChanged();}
void ApplicationController::deleteEvent(const QString&id)
{if(!m_eventRepository||!m_eventRepository->removeEvent(id))return;if(m_currentEventId==id){m_currentEventId.clear();m_eventItems.clear();emit currentEventChanged();emit eventItemsChanged();}refreshEvents();}
void ApplicationController::addEventItem(const QString&type,const QString&referenceId,const QString&title,qint64 durationMs)
{
    if(!m_eventRepository||m_currentEventId.isEmpty()||referenceId.isEmpty())return;
    PlaylistItemType itemType=PlaylistItemType::Text;if(type=="song")itemType=PlaylistItemType::Song;else if(type=="image")itemType=PlaylistItemType::Image;else if(type=="video")itemType=PlaylistItemType::Video;else if(type=="audio")itemType=PlaylistItemType::Audio;
    if(m_eventRepository->addItem(m_currentEventId,PlaylistItem{.type=itemType,.referenceId=referenceId,.title=title,.durationMs=durationMs}))refreshEventItems();
}
void ApplicationController::removeEventItem(const QString&id){if(m_eventRepository&&m_eventRepository->removeItem(id))refreshEventItems();}
void ApplicationController::moveEventItem(const QString&id,int newIndex){if(m_eventRepository&&m_eventRepository->moveItem(id,newIndex))refreshEventItems();}
void ApplicationController::executeEventItem(const QString&id)
{
    if(!m_eventRepository)return;for(const auto&item:m_eventRepository->items(m_currentEventId))if(item.id==id){
        switch(item.type){case PlaylistItemType::Song:selectSong(item.referenceId);showTextSlide(0);break;case PlaylistItemType::Text:selectTextPresentation(item.referenceId);showTextSlide(0);break;case PlaylistItemType::Image:showImage(item.referenceId);break;case PlaylistItemType::Video:playVideo(item.referenceId);break;case PlaylistItemType::Audio:playAudio(item.referenceId);break;}return;
    }
}
void ApplicationController::clearHistory(){if(m_historyRepository&&m_historyRepository->clear())refreshHistory();}
QString ApplicationController::createBackup(){if(!m_recovery)return{};m_lastBackupPath=m_recovery->createBackup();setStatusMessage(m_lastBackupPath.isEmpty()?QStringLiteral("Não foi possível criar o backup."):QStringLiteral("Backup criado em %1").arg(m_lastBackupPath));emit maintenanceChanged();return m_lastBackupPath;}
bool ApplicationController::scheduleRestore(const QUrl&source){if(!m_recovery)return false;const auto path=source.isLocalFile()?source.toLocalFile():source.toString();const bool ok=m_recovery->scheduleRestore(path);setStatusMessage(ok?QStringLiteral("Restauração agendada. Reinicie o HolyScreen para aplicá-la."):QStringLiteral("O arquivo não é um backup SQLite válido."));emit maintenanceChanged();return ok;}
void ApplicationController::runBenchmark(){QElapsedTimer timer;timer.start();volatile quint64 checksum=0;for(int frame=0;frame<100000;++frame)checksum+=qHash(QString::number(frame));const auto elapsed=std::max<qint64>(1,timer.nsecsElapsed());m_diagnostics["benchmarkOperationsPerSecond"]=static_cast<qint64>(100000.0*1e9/elapsed);m_diagnostics["benchmarkChecksum"]=static_cast<qulonglong>(checksum);m_diagnostics["benchmarkAt"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);emit diagnosticsChanged();}
void ApplicationController::checkForUpdates(){m_updateStatus=QStringLiteral("Verificando...");emit updateChanged();m_updateChecker.check(QUrl(m_updateEndpoint),QCoreApplication::applicationVersion());}

int ApplicationController::importBibleTranslation(const QUrl &source)
{
    if (!m_bibleRepository) return 0;
    const auto path = source.isLocalFile() ? source.toLocalFile() : source.toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatusMessage(QStringLiteral("Não foi possível abrir o arquivo da Bíblia."));
        return 0;
    }
    auto imported = m_bibleJsonImporter.parse(file.readAll());
    if (!imported.isValid()) {
        setStatusMessage(imported.error);
        return 0;
    }
    const auto translationId = m_bibleRepository->saveTranslation(imported.translation);
    if (translationId.isEmpty()
        || !m_bibleRepository->importVerses(translationId, imported.verses)) {
        setStatusMessage(QStringLiteral("Não foi possível salvar a tradução importada."));
        return 0;
    }
    refreshBibleTranslations();
    if (m_biblePrimaryTranslationId.isEmpty()) setBiblePrimaryTranslationId(translationId);
    setStatusMessage(QStringLiteral("Tradução %1 importada com %2 versículo(s).")
                         .arg(imported.translation.abbreviation)
                         .arg(imported.verses.size()));
    return imported.verses.size();
}

bool ApplicationController::searchBibleReference()
{
    m_bibleResults.clear();
    if (!m_bibleRepository) return false;
    const auto reference = m_bibleReferenceParser.parse(m_bibleReferenceInput);
    if (!reference.has_value()) {
        emit bibleResultsChanged();
        setStatusMessage(QStringLiteral("Referência bíblica inválida. Exemplo: João 3:16-18."));
        return false;
    }

    QStringList selectedIds;
    for (const auto &id : {m_biblePrimaryTranslationId,
                           m_bibleSecondaryTranslationId,
                           m_bibleTertiaryTranslationId}) {
        if (!id.isEmpty() && !selectedIds.contains(id)) selectedIds.append(id);
    }
    if (selectedIds.isEmpty()) {
        emit bibleResultsChanged();
        setStatusMessage(QStringLiteral("Importe e selecione ao menos uma tradução bíblica."));
        return false;
    }

    QHash<QString, BibleTranslation> translationsById;
    for (const auto &translation : m_bibleRepository->translations()) {
        translationsById.insert(translation.id, translation);
    }
    QHash<QString, QHash<int, BibleVerse>> versesByTranslation;
    for (const auto &id : selectedIds) {
        for (const auto &verse : m_bibleRepository->verses(id, reference.value())) {
            versesByTranslation[id].insert(verse.verse, verse);
        }
    }

    const auto bookName = bibleBookName(reference->book);
    for (int verseNumber = reference->firstVerse; verseNumber <= reference->lastVerse; ++verseNumber) {
        QVariantList versions;
        QStringList combinedTexts;
        for (const auto &id : selectedIds) {
            const auto verse = versesByTranslation.value(id).value(verseNumber);
            if (verse.text.isEmpty()) continue;
            const auto translation = translationsById.value(id);
            versions.append(QVariantMap{
                {QStringLiteral("translationId"), id},
                {QStringLiteral("abbreviation"), translation.abbreviation},
                {QStringLiteral("text"), verse.text},
            });
            combinedTexts.append(QStringLiteral("[%1] %2").arg(translation.abbreviation, verse.text));
        }
        if (versions.isEmpty()) continue;
        const auto label = QStringLiteral("%1 %2:%3")
            .arg(bookName).arg(reference->chapter).arg(verseNumber);
        m_bibleResults.append(QVariantMap{
            {QStringLiteral("verse"), verseNumber},
            {QStringLiteral("label"), label},
            {QStringLiteral("text"), combinedTexts.join(QStringLiteral("\n\n"))},
            {QStringLiteral("versions"), versions},
        });
    }
    m_currentBibleReference = reference.value();
    emit bibleResultsChanged();
    if (m_bibleResults.isEmpty()) {
        setStatusMessage(QStringLiteral("Nenhum versículo foi encontrado nas traduções selecionadas."));
        return false;
    }
    setStatusMessage(QStringLiteral("%1 versículo(s) encontrado(s).").arg(m_bibleResults.size()));
    return true;
}

void ApplicationController::showBibleVerse(int index)
{
    if (index < 0 || index >= m_bibleResults.size()) return;
    Presentation presentation{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = PresentationType::Bible,
        .title = m_bibleReferenceInput.trimmed(),
    };
    for (int slideIndex = 0; slideIndex < m_bibleResults.size(); ++slideIndex) {
        const auto result = m_bibleResults.at(slideIndex).toMap();
        presentation.slides.append(Slide{
            .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
            .label = result.value(QStringLiteral("label")).toString(),
            .text = result.value(QStringLiteral("text")).toString(),
            .order = slideIndex,
        });
    }
    m_textPresentation.setPresentation(std::move(presentation));
    emit currentPresentationChanged();
    emit textSlidesChanged();
    showTextSlide(index);
}

QString ApplicationController::bibleTextForSlide(
    int slideIndex, const QString &translationId) const
{
    if (slideIndex < 0 || slideIndex >= m_bibleResults.size()) return {};
    const auto result = m_bibleResults.at(slideIndex).toMap();
    if (translationId.isEmpty()) return result.value(QStringLiteral("text")).toString();
    for (const auto &versionEntry : result.value(QStringLiteral("versions")).toList()) {
        const auto version = versionEntry.toMap();
        if (version.value(QStringLiteral("translationId")).toString() == translationId) {
            return QStringLiteral("[%1] %2")
                .arg(version.value(QStringLiteral("abbreviation")).toString(),
                     version.value(QStringLiteral("text")).toString());
        }
    }
    return result.value(QStringLiteral("text")).toString();
}

void ApplicationController::setWallpaperColor(const QString &color)
{
    if (m_wallpaperColor == color) return;
    m_wallpaperColor = color;
    saveSetting(QStringLiteral("wallpaperColor"), color);
    emit wallpaperColorChanged();
}

void ApplicationController::setWallpaperSource(const QUrl &source)
{
    QUrl normalized = source;
    if (!normalized.isEmpty()) {
        if (!normalized.isLocalFile() || !QFileInfo::exists(normalized.toLocalFile())) {
            setStatusMessage(QStringLiteral("Não foi possível abrir o wallpaper selecionado."));
            return;
        }
    }
    if (m_wallpaperSource == normalized) return;
    m_wallpaperSource = normalized;
    saveSetting(QStringLiteral("wallpaperSource"), normalized.toString());
    emit wallpaperSourceChanged();
    setStatusMessage({});
}

void ApplicationController::setWallpaperFit(const QString &fit)
{
    static const QStringList accepted{QStringLiteral("cover"), QStringLiteral("contain"),
                                      QStringLiteral("stretch"), QStringLiteral("center")};
    const auto normalized = accepted.contains(fit) ? fit : QStringLiteral("cover");
    if (m_wallpaperFit == normalized) return;
    m_wallpaperFit = normalized;
    saveSetting(QStringLiteral("wallpaperFit"), normalized);
    emit wallpaperFitChanged();
}

void ApplicationController::setClockVisible(bool visible)
{
    if (m_clockVisible == visible) return;
    m_clockVisible = visible;
    saveSetting(QStringLiteral("clockVisible"), visible);
    emit clockVisibleChanged();
}

void ApplicationController::setClockPosition(const QString &position)
{
    static const QStringList accepted{QStringLiteral("bottomRight"), QStringLiteral("topRight"),
                                      QStringLiteral("bottomLeft"), QStringLiteral("topLeft")};
    const auto normalized = accepted.contains(position) ? position : QStringLiteral("bottomRight");
    if (m_clockPosition == normalized) return;
    m_clockPosition = normalized;
    saveSetting(QStringLiteral("clockPosition"), normalized);
    emit clockPositionChanged();
}

void ApplicationController::setClockFormat(const QString &format)
{
    if (m_clock.format() == format) return;
    m_clock.setFormat(format);
    saveSetting(QStringLiteral("clockFormat"), m_clock.format());
}

void ApplicationController::setClockFontFamily(const QString &family)
{
    const auto normalized = family.trimmed();
    if (m_clockFontFamily == normalized) return;
    m_clockFontFamily = normalized;
    saveSetting(QStringLiteral("clockFontFamily"), normalized);
    emit clockFontFamilyChanged();
}

void ApplicationController::setClockFontSize(int size)
{
    const auto normalized = std::clamp(size, 16, 240);
    if (m_clockFontSize == normalized) return;
    m_clockFontSize = normalized;
    saveSetting(QStringLiteral("clockFontSize"), normalized);
    emit clockFontSizeChanged();
}

void ApplicationController::setClockColor(const QString &color)
{
    if (m_clockColor == color) return;
    m_clockColor = color;
    saveSetting(QStringLiteral("clockColor"), color);
    emit clockColorChanged();
}

void ApplicationController::setSimulatedOutputCount(int count)
{
    count = std::clamp(count, 1, 5);
    if (m_simulatedOutputCount == count) return;
    m_simulatedOutputCount = count;
    saveSetting(QStringLiteral("simulatedOutputCount"), count);
    emit simulatedOutputCountChanged();
}

void ApplicationController::setAudioFileSearch(const QString &search)
{
    if (m_audioFileSearch == search) return;
    m_audioFileSearch = search;
    refreshMediaCatalogViews();
}

void ApplicationController::setVideoFileSearch(const QString &search)
{
    if (m_videoFileSearch == search) return;
    m_videoFileSearch = search;
    refreshMediaCatalogViews();
}

void ApplicationController::setImageFileSearch(const QString &search)
{
    if (m_imageFileSearch == search) return;
    m_imageFileSearch = search;
    refreshMediaCatalogViews();
}

void ApplicationController::setStageMessage(const QString &message)
{
    const auto normalized = message.trimmed();
    if (m_stageMessage == normalized) return;
    m_stageMessage = normalized;
    saveSetting(QStringLiteral("stageMessage"), normalized);
    emit stageMessageChanged();
}

void ApplicationController::setAudienceMessage(const QString &message)
{ m_overlayCommands->requestAudienceMessage(message); }
void ApplicationController::setAlertMessage(const QString &message)
{ m_overlayCommands->requestAlert(message); }
void ApplicationController::setLowerThird(const QString &title, const QString &subtitle)
{ m_overlayCommands->requestLowerThird(title, subtitle); }
void ApplicationController::startCountdown(int seconds)
{ m_overlayCommands->requestCountdownStart(seconds); }
void ApplicationController::stopCountdown()
{ m_overlayCommands->requestCountdownStop(); }
void ApplicationController::startStopwatch()
{ m_overlayCommands->requestStopwatchStart(); }
void ApplicationController::pauseStopwatch()
{ m_overlayCommands->requestStopwatchPause(); }
void ApplicationController::resetStopwatch()
{ m_overlayCommands->requestStopwatchReset(); }

void ApplicationController::setDebugEnabled(bool enabled)
{
    if (m_debugEnabled == enabled) return;
    m_debugEnabled = enabled;
    if (m_settings) m_settings->setValue(QStringLiteral("developer/debugEnabled"), enabled);
    AppLogger::setDebugMessagesEnabled(m_debugEnabled && m_debugLogging);
    emit debugOptionsChanged();
}

void ApplicationController::setDebugSimulatedOutputs(bool enabled)
{
    if (m_debugSimulatedOutputs == enabled) return;
    m_debugSimulatedOutputs = enabled;
    if (m_settings) m_settings->setValue(QStringLiteral("developer/debugSimulatedOutputs"), enabled);
    emit debugOptionsChanged();
}

void ApplicationController::setDebugDiagnostics(bool enabled)
{
    if (m_debugDiagnostics == enabled) return;
    m_debugDiagnostics = enabled;
    if (m_settings) m_settings->setValue(QStringLiteral("developer/debugDiagnostics"), enabled);
    emit debugOptionsChanged();
}

void ApplicationController::setDebugLogging(bool enabled)
{
    if (m_debugLogging == enabled) return;
    m_debugLogging = enabled;
    if (m_settings) m_settings->setValue(QStringLiteral("developer/debugLogging"), enabled);
    AppLogger::setDebugMessagesEnabled(m_debugEnabled && m_debugLogging);
    emit debugOptionsChanged();
}

void ApplicationController::setAudioVolume(double volume)
{
    setMediaVolume(volume);
}

void ApplicationController::setVideoVolume(double volume)
{
    setMediaVolume(volume);
}

void ApplicationController::setMediaVolume(double volume)
{
    const auto normalized = std::clamp(volume, 0.0, 1.0);
    if (qFuzzyCompare(m_video.volume(), normalized)) return;
    m_video.setVolume(normalized);
    saveSetting(QStringLiteral("mediaVolume"), normalized);
    emit mediaVolumeChanged();
    emit audioVolumeChanged();
    emit videoVolumeChanged();
}

void ApplicationController::setMediaRepeatMode(const QString &mode)
{
    const auto normalized = mode == QStringLiteral("one") ? QStringLiteral("one")
                          : mode == QStringLiteral("all") ? QStringLiteral("all")
                          : QStringLiteral("off");
    if (m_mediaRepeatMode == normalized) return;
    m_mediaRepeatMode = normalized;
    m_video.setLoop(normalized == QStringLiteral("one"));
    saveSetting(QStringLiteral("mediaRepeatMode"), normalized);
    emit mediaRepeatModeChanged();
    emit videoLoopChanged();
}

void ApplicationController::setVideoLoop(bool loop)
{
    setMediaRepeatMode(loop ? QStringLiteral("one") : QStringLiteral("off"));
}

void ApplicationController::setImageFit(const QString &fit)
{
    ImageFit normalized = ImageFit::Contain;
    if (fit == QStringLiteral("cover")) normalized = ImageFit::Cover;
    else if (fit == QStringLiteral("stretch")) normalized = ImageFit::Stretch;
    else if (fit == QStringLiteral("center")) normalized = ImageFit::Center;
    if (m_images.fit() == normalized) return;
    m_images.setFit(normalized);
    saveSetting(QStringLiteral("imageFit"), imageFitName(normalized));
    emit imageFitChanged();
}

void ApplicationController::setImageTransition(const QString &transition)
{
    const auto normalized = transition == QStringLiteral("none")
        ? ImageTransition::None : ImageTransition::Fade;
    if (m_images.transition() == normalized) return;
    m_images.setTransition(normalized);
    saveSetting(QStringLiteral("imageTransition"), imageTransitionName(normalized));
    emit imageTransitionChanged();
}

void ApplicationController::setImageAutoplay(bool enabled)
{
    if (m_images.autoplay() == enabled) return;
    m_images.setAutoplay(enabled);
    saveSetting(QStringLiteral("imageAutoplay"), enabled);
    emit imageAutoplayChanged();
}

void ApplicationController::setImageIntervalMs(int intervalMs)
{
    const auto before = m_images.autoplayIntervalMs();
    m_images.setAutoplayIntervalMs(intervalMs);
    if (before == m_images.autoplayIntervalMs()) return;
    saveSetting(QStringLiteral("imageIntervalMs"), m_images.autoplayIntervalMs());
    emit imageIntervalChanged();
}

void ApplicationController::setSongSearch(const QString &search)
{
    const auto normalized=search.trimmed();if(m_songSearch==normalized)return;m_songSearch=normalized;emit songSearchChanged();refreshSongs();
}
void ApplicationController::setUpdateEndpoint(const QString&endpoint){const auto value=endpoint.trimmed();if(m_updateEndpoint==value)return;m_updateEndpoint=value;saveSetting(QStringLiteral("updateEndpoint"),value);emit updateChanged();}

void ApplicationController::setBiblePrimaryTranslationId(const QString &id)
{
    if (m_biblePrimaryTranslationId == id) return;
    m_biblePrimaryTranslationId = id;
    if (m_settings) m_settings->setValue(QStringLiteral("bible/primaryTranslationId"), id);
    emit bibleSelectionChanged();
}

void ApplicationController::setBibleSecondaryTranslationId(const QString &id)
{
    if (m_bibleSecondaryTranslationId == id) return;
    m_bibleSecondaryTranslationId = id;
    if (m_settings) m_settings->setValue(QStringLiteral("bible/secondaryTranslationId"), id);
    emit bibleSelectionChanged();
}

void ApplicationController::setBibleTertiaryTranslationId(const QString &id)
{
    if (m_bibleTertiaryTranslationId == id) return;
    m_bibleTertiaryTranslationId = id;
    if (m_settings) m_settings->setValue(QStringLiteral("bible/tertiaryTranslationId"), id);
    emit bibleSelectionChanged();
}

void ApplicationController::setBibleReferenceInput(const QString &reference)
{
    if (m_bibleReferenceInput == reference) return;
    m_bibleReferenceInput = reference;
    emit bibleSelectionChanged();
}

void ApplicationController::refreshScreens()
{
    const auto &descriptors = m_screenManager->screens();
    m_outputs.applyScreens(descriptors);

    m_screens.clear();
    for (const auto &screen : descriptors) {
        bool selected = false;
        QString bibleTranslationId;
        QString role = QStringLiteral("audience");
        for (const auto &output : m_outputs.activeOutputs()) {
            if (output.screenFingerprint == screen.fingerprint) {
                selected = true;
                bibleTranslationId = output.bibleTranslationId;
                role = outputRoleName(output.role);
                break;
            }
        }
        auto item = mapScreen(screen, selected);
        item.insert(QStringLiteral("bibleTranslationId"), bibleTranslationId);
        item.insert(QStringLiteral("role"), role);
        m_screens.append(item);
    }
    emit screensChanged();
    emit outputWindowsChanged();
    m_diagnostics[QStringLiteral("detectedScreens")]=descriptors.size();
    m_diagnostics[QStringLiteral("activeOutputs")]=m_outputs.activeOutputs().size();
    emit diagnosticsChanged();
}

void ApplicationController::loadSettings()
{
    const auto overridePath = qEnvironmentVariable("HOLYSCREEN_DATA_DIR");
    const auto appData = overridePath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        : overridePath;
    m_dataDirectory=appData;
    QDir().mkpath(appData);
    const auto databasePath = appData + QStringLiteral("/presenter.db");
    m_recovery=std::make_unique<DataRecoveryService>(appData);
    if(!m_recovery->applyPendingRestore())qWarning()<<"Could not apply pending database restore";
    const auto migration = ApplicationDatabase::migrate(databasePath);
    if (!migration.success) {
        qCritical() << "Database migration failed:" << migration.error;
        m_diagnostics = {
            {QStringLiteral("database"), databasePath},
            {QStringLiteral("schemaVersion"), migration.currentVersion},
            {QStringLiteral("migrationError"), migration.error},
        };
        setStatusMessage(QStringLiteral("Não foi possível atualizar o banco local: %1")
                             .arg(migration.error));
        return;
    }
    m_recovery->beginSession();
    m_settings = std::make_unique<SettingsRepository>(databasePath);
    if (!m_settings->open()) {
        qWarning() << "Failed to open settings database";
        return;
    }

    m_wallpaperColor = m_settings->value(QStringLiteral("presentation/wallpaperColor"), m_wallpaperColor).toString();
    const QUrl storedWallpaper(m_settings->value(QStringLiteral("presentation/wallpaperSource")).toString());
    if (storedWallpaper.isLocalFile() && QFileInfo::exists(storedWallpaper.toLocalFile())) {
        m_wallpaperSource = storedWallpaper;
    } else if (!storedWallpaper.isEmpty()) {
        qWarning() << "wallpaper_missing" << storedWallpaper.toLocalFile();
    }
    m_wallpaperFit = m_settings->value(QStringLiteral("presentation/wallpaperFit"), m_wallpaperFit).toString();
    m_clockVisible = m_settings->value(QStringLiteral("presentation/clockVisible"), m_clockVisible).toBool();
    m_clockPosition = m_settings->value(QStringLiteral("presentation/clockPosition"), m_clockPosition).toString();
    m_clock.setFormat(m_settings->value(QStringLiteral("presentation/clockFormat"), m_clock.format()).toString());
    m_clockFontFamily = m_settings->value(QStringLiteral("presentation/clockFontFamily"), m_clockFontFamily).toString();
    m_clockFontSize = m_settings->value(QStringLiteral("presentation/clockFontSize"), m_clockFontSize).toInt();
    m_clockColor = m_settings->value(QStringLiteral("presentation/clockColor"), m_clockColor).toString();
    m_stageMessage = m_settings->value(QStringLiteral("presentation/stageMessage")).toString();
    m_simulatedOutputCount = m_settings->value(QStringLiteral("developer/simulatedOutputCount"), m_simulatedOutputCount).toInt();
    m_debugEnabled = m_settings->value(QStringLiteral("developer/debugEnabled"), false).toBool();
    m_debugSimulatedOutputs = m_settings->value(QStringLiteral("developer/debugSimulatedOutputs"), true).toBool();
    m_debugDiagnostics = m_settings->value(QStringLiteral("developer/debugDiagnostics"), true).toBool();
    m_debugLogging = m_settings->value(QStringLiteral("developer/debugLogging"), false).toBool();
    m_mediaFolderPaths = m_settings->value(QStringLiteral("library/mediaFolders"), QStringList{}).toStringList();
    m_biblePrimaryTranslationId = m_settings->value(QStringLiteral("bible/primaryTranslationId")).toString();
    m_bibleSecondaryTranslationId = m_settings->value(QStringLiteral("bible/secondaryTranslationId")).toString();
    m_bibleTertiaryTranslationId = m_settings->value(QStringLiteral("bible/tertiaryTranslationId")).toString();
    AppLogger::setDebugMessagesEnabled(m_debugEnabled && m_debugLogging);
    const auto legacyVolume = m_settings->value(QStringLiteral("presentation/videoVolume"),
        m_settings->value(QStringLiteral("presentation/audioVolume"), 0.8)).toDouble();
    m_video.setVolume(m_settings->value(QStringLiteral("presentation/mediaVolume"), legacyVolume).toDouble());
    m_mediaRepeatMode = m_settings->value(QStringLiteral("presentation/mediaRepeatMode"),
        m_settings->value(QStringLiteral("presentation/videoLoop"), false).toBool()
            ? QStringLiteral("one") : QStringLiteral("off")).toString();
    m_video.setLoop(m_mediaRepeatMode == QStringLiteral("one"));
    m_updateEndpoint=m_settings->value(QStringLiteral("presentation/updateEndpoint"),QString{}).toString();
    const auto storedImageFit = m_settings->value(QStringLiteral("presentation/imageFit"), QStringLiteral("contain")).toString();
    m_images.setFit(storedImageFit == QStringLiteral("cover") ? ImageFit::Cover
                    : storedImageFit == QStringLiteral("stretch") ? ImageFit::Stretch
                    : storedImageFit == QStringLiteral("center") ? ImageFit::Center
                    : ImageFit::Contain);
    m_images.setTransition(m_settings->value(QStringLiteral("presentation/imageTransition"), QStringLiteral("fade")).toString()
                               == QStringLiteral("none") ? ImageTransition::None : ImageTransition::Fade);
    m_images.setAutoplayIntervalMs(m_settings->value(QStringLiteral("presentation/imageIntervalMs"), 5000).toInt());
    m_images.setAutoplay(m_settings->value(QStringLiteral("presentation/imageAutoplay"), false).toBool());

    m_mediaRepository = std::make_unique<MediaRepository>(databasePath);
    if (!m_mediaRepository->open()) {
        qWarning() << "Failed to open media database";
    }
    refreshAudioLibrary();
    refreshVideoLibrary();
    refreshMediaPlaylist();
    refreshImageLibrary();
    refreshMediaCatalog();
    m_bibleRepository = std::make_unique<BibleRepository>(databasePath);
    if (!m_bibleRepository->open()) qWarning() << "Failed to open Bible database";
    refreshBibleTranslations();
    m_presentationRepository = std::make_unique<PresentationRepository>(databasePath);
    if (!m_presentationRepository->open()) qWarning() << "Failed to open presentation database";
    refreshTextPresentations();
    m_themeRepository = std::make_unique<ThemeRepository>(databasePath);
    if (!m_themeRepository->open()) qWarning() << "Failed to open theme database";
    if (m_themeRepository->themes().isEmpty()) {
        Theme standard; standard.name=QStringLiteral("Padrão"); standard.fontFamily=m_clockFontFamily;
        m_themeRepository->save(standard);
    }
    refreshThemes(); loadActiveTheme();
    refreshSongs();
    m_eventRepository=std::make_unique<EventRepository>(databasePath);
    if(!m_eventRepository->open())qWarning()<<"Failed to open event database";
    refreshEvents();
    m_historyRepository=std::make_unique<HistoryRepository>(databasePath);
    if(!m_historyRepository->open())qWarning()<<"Failed to open history database";
    refreshHistory();
    m_diagnostics={{"version",QCoreApplication::applicationVersion()},{"qtVersion",QString::fromLatin1(qVersion())},{"platform",QSysInfo::prettyProductName()},{"cpu",QSysInfo::currentCpuArchitecture()},{"dataDirectory",m_dataDirectory},{"database",databasePath},{"schemaVersion",migration.currentVersion},{"migrationBackup",migration.backupPath},{"recoveredFromCrash",recoveredFromCrash()}};

    const auto serialized = m_settings->value(QStringLiteral("outputs/items")).toStringList();
    for (const auto &entry : serialized) {
        const auto values = entry.split(QLatin1Char('\u001F'));
        if (values.size() >= 2) {
            m_outputs.restore(OutputDescriptor{
                .screenId = values[0],
                .screenFingerprint = values[1],
                .displayName = values.size() > 2 ? values[2] : values[0],
                .enabled = true,
                .role = values.size() > 4 ? outputRoleFromName(values[4]) : OutputRole::Audience,
                .bibleTranslationId = values.size() > 3 ? values[3] : QString{},
            });
        }
    }

}

void ApplicationController::saveSetting(const QString &key, const QVariant &value)
{
    if (!m_settings) return;
    const auto fullKey = key == QStringLiteral("simulatedOutputCount")
        ? QStringLiteral("developer/%1").arg(key)
        : QStringLiteral("presentation/%1").arg(key);
    m_settings->setValue(fullKey, value);
}

void ApplicationController::saveOutputs()
{
    QStringList serialized;
    for (const auto &output : m_outputs.activeOutputs()) {
        serialized.append(QStringList{output.screenId, output.screenFingerprint,
                                      output.displayName, output.bibleTranslationId,
                                      outputRoleName(output.role)}
                              .join(QLatin1Char('\u001F')));
    }
    if (m_settings) {
        m_settings->setValue(QStringLiteral("outputs/items"), serialized);
    }
}

void ApplicationController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) return;
    m_statusMessage = message;
    emit statusMessageChanged();
}

void ApplicationController::saveMediaFolders()
{
    if (m_settings) {
        m_settings->setValue(QStringLiteral("library/mediaFolders"), m_mediaFolderPaths);
    }
}

void ApplicationController::rebuildMediaFolderWatcher()
{
    const auto watched = m_mediaFolderWatcher.directories();
    if (!watched.isEmpty()) m_mediaFolderWatcher.removePaths(watched);

    QSet<QString> directories;
    for (const auto &folder : m_mediaFolderPaths) {
        const QFileInfo info(folder);
        if (!info.isDir()) continue;
        directories.insert(info.canonicalFilePath());
        QDirIterator iterator(folder,
                              QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const auto path = QFileInfo(iterator.next()).canonicalFilePath();
            if (!path.isEmpty()) directories.insert(path);
        }
    }
    if (!directories.isEmpty()) m_mediaFolderWatcher.addPaths(directories.values());
}

void ApplicationController::refreshMediaCatalog()
{
    m_mediaCatalogEntries = m_mediaFolderScanner.scan(m_mediaFolderPaths);
    rebuildMediaFolderWatcher();
    refreshMediaCatalogViews();
}

void ApplicationController::refreshMediaCatalogViews()
{
    QSet<QString> playlistPaths;
    if (m_mediaRepository) {
        for (const auto &item : m_mediaRepository->playlistItems()) playlistPaths.insert(item.path);
    }

    const auto mapEntries = [&](MediaType type, const QString &search) {
        QVariantList result;
        for (const auto &entry : MediaFolderScanner::filter(m_mediaCatalogEntries, type, search)) {
            result.append(mapCatalogEntry(entry, playlistPaths.contains(entry.path)));
        }
        return result;
    };
    m_folderAudioFiles = mapEntries(MediaType::Audio, m_audioFileSearch);
    m_folderVideoFiles = mapEntries(MediaType::Video, m_videoFileSearch);
    m_folderImageFiles = mapEntries(MediaType::Image, m_imageFileSearch);
    emit mediaCatalogChanged();
}

void ApplicationController::refreshMediaPlaylist()
{
    QVariantList updated;
    if (m_mediaRepository) {
        for (const auto &item : m_mediaRepository->playlistItems()) updated.append(mapMedia(item));
    }
    m_mediaPlaylist = updated;
    emit mediaPlaylistChanged();
    emit currentMediaChanged();
    refreshMediaCatalogViews();
}

void ApplicationController::updateCurrentMediaMetadata(const MediaItem &metadata)
{
    if (!m_mediaRepository || m_currentMediaId.isEmpty()) return;
    auto stored = m_mediaRepository->item(m_currentMediaId);
    if (stored.id.isEmpty()) return;
    if (!metadata.title.isEmpty()) stored.title = metadata.title;
    if (!metadata.artist.isEmpty()) stored.artist = metadata.artist;
    if (!metadata.album.isEmpty()) stored.album = metadata.album;
    if (metadata.durationMs > 0) stored.durationMs = metadata.durationMs;
    if (!m_mediaRepository->update(stored)) return;
    refreshMediaPlaylist();
    if (stored.type == MediaType::Audio) refreshAudioLibrary();
    else if (stored.type == MediaType::Video) refreshVideoLibrary();
}

void ApplicationController::refreshAudioLibrary()
{
    QVariantList updated;
    if (m_mediaRepository) {
        for (const auto &item : m_mediaRepository->items(MediaType::Audio)) {
            updated.append(mapMedia(item));
        }
    }
    m_audioLibrary = updated;
    emit audioLibraryChanged();
    emit currentAudioChanged();
}

void ApplicationController::updateCurrentAudioMetadata(const MediaItem &metadata)
{
    if (!m_mediaRepository || m_currentAudioId.isEmpty()) return;
    auto stored = m_mediaRepository->item(m_currentAudioId);
    if (stored.id.isEmpty()) return;
    if (!metadata.title.isEmpty()) stored.title = metadata.title;
    if (!metadata.artist.isEmpty()) stored.artist = metadata.artist;
    if (!metadata.album.isEmpty()) stored.album = metadata.album;
    if (metadata.durationMs > 0) stored.durationMs = metadata.durationMs;
    if (m_mediaRepository->update(stored)) refreshAudioLibrary();
}

void ApplicationController::refreshVideoLibrary()
{
    QVariantList updated;
    if (m_mediaRepository) {
        for (const auto &item : m_mediaRepository->items(MediaType::Video)) updated.append(mapMedia(item));
    }
    m_videoLibrary = updated;
    emit videoLibraryChanged();
    emit currentVideoChanged();
}

void ApplicationController::updateCurrentVideoMetadata(const MediaItem &metadata)
{
    if (!m_mediaRepository || m_currentVideoId.isEmpty()) return;
    auto stored = m_mediaRepository->item(m_currentVideoId);
    if (stored.id.isEmpty()) return;
    if (!metadata.title.isEmpty()) stored.title = metadata.title;
    if (metadata.durationMs > 0) stored.durationMs = metadata.durationMs;
    if (m_mediaRepository->update(stored)) refreshVideoLibrary();
}

void ApplicationController::refreshImageLibrary()
{
    QVariantList updated;
    QVector<MediaItem> playlist;
    if (m_mediaRepository) {
        playlist = m_mediaRepository->items(MediaType::Image);
        for (const auto &item : playlist) updated.append(mapMedia(item));
    }
    m_imageLibrary = updated;
    m_images.setPlaylist(std::move(playlist));
    emit imageLibraryChanged();
    emit currentImageChanged();
    emit presentationImageSourceChanged();
}

void ApplicationController::refreshBibleTranslations()
{
    QVariantList updated;
    QSet<QString> availableIds;
    if (m_bibleRepository) {
        for (const auto &translation : m_bibleRepository->translations()) {
            updated.append(mapBibleTranslation(translation));
            availableIds.insert(translation.id);
        }
    }
    m_bibleTranslations = updated;
    if (!availableIds.contains(m_biblePrimaryTranslationId)) {
        m_biblePrimaryTranslationId = updated.isEmpty()
            ? QString{} : updated.front().toMap().value(QStringLiteral("id")).toString();
    }
    if (!availableIds.contains(m_bibleSecondaryTranslationId)) m_bibleSecondaryTranslationId.clear();
    if (!availableIds.contains(m_bibleTertiaryTranslationId)) m_bibleTertiaryTranslationId.clear();
    emit bibleTranslationsChanged();
    emit bibleSelectionChanged();
}

void ApplicationController::refreshTextPresentations()
{
    QVariantList updated;
    if (m_presentationRepository) {
        for (const auto &item : m_presentationRepository->presentations(PresentationType::Text)) {
            updated.append(QVariantMap{{"id",item.id},{"title",item.title},{"author",item.author},{"slideCount",item.slides.size()}});
        }
    }
    m_textPresentations = updated;
    emit textPresentationsChanged();
}

void ApplicationController::saveCurrentPresentation()
{
    if (!m_presentationRepository || currentPresentationId().isEmpty()) return;
    if (!m_presentationRepository->save(m_textPresentation.presentation()).isEmpty()) {
        refreshTextPresentations(); emit textSlidesChanged(); emit currentSlideChanged();
    }
}

void ApplicationController::refreshThemes()
{
    QVariantList updated;
    if(m_themeRepository)for(const auto&t:m_themeRepository->themes())updated.append(mapTheme(t));
    m_themes=updated;emit themesChanged();
}

void ApplicationController::loadActiveTheme()
{
    Theme selected;
    if(m_themeRepository){
        const auto id=m_textPresentation.presentation().defaultTheme;
        if(!id.isEmpty())selected=m_themeRepository->theme(id);
        if(selected.id.isEmpty()){const auto all=m_themeRepository->themes();if(!all.isEmpty())selected=all.front();}
    }
    m_activeTheme=selected;emit activeThemeChanged();
}

void ApplicationController::refreshSongs()
{
    QVariantList updated;
    if(m_presentationRepository)for(const auto&song:m_presentationRepository->presentations(PresentationType::Song)){
        bool matches=m_songSearch.isEmpty()||song.title.contains(m_songSearch,Qt::CaseInsensitive)||song.author.contains(m_songSearch,Qt::CaseInsensitive);
        if(!matches)for(const auto&slide:song.slides)if(slide.text.contains(m_songSearch,Qt::CaseInsensitive)){matches=true;break;}
        if(!matches)continue;
        updated.append(QVariantMap{{"id",song.id},{"title",song.title},{"author",song.author},{"slideCount",song.slides.size()},{"sequenceCount",song.sequence.size()}});
    }
    m_songs=updated;emit songsChanged();
}

void ApplicationController::refreshEvents()
{
    QVariantList updated;if(m_eventRepository)for(const auto&e:m_eventRepository->events())updated.append(QVariantMap{{"id",e.id},{"title",e.title},{"scheduledAt",e.scheduledAt},{"durationMs",m_eventRepository->totalDurationMs(e.id)}});
    m_events=updated;emit eventsChanged();
}
void ApplicationController::refreshEventItems()
{
    QVariantList updated;if(m_eventRepository)for(const auto&i:m_eventRepository->items(m_currentEventId)){
        QString type="text";if(i.type==PlaylistItemType::Song)type="song";else if(i.type==PlaylistItemType::Image)type="image";else if(i.type==PlaylistItemType::Video)type="video";else if(i.type==PlaylistItemType::Audio)type="audio";
        updated.append(QVariantMap{{"id",i.id},{"type",type},{"referenceId",i.referenceId},{"title",i.title},{"durationMs",i.durationMs},{"position",i.position}});
    }m_eventItems=updated;emit eventItemsChanged();refreshEvents();
}

void ApplicationController::refreshHistory()
{
    QVariantList updated;if(m_historyRepository)for(const auto&e:m_historyRepository->entries())updated.append(QVariantMap{{"id",e.id},{"type",e.itemType},{"referenceId",e.referenceId},{"title",e.title},{"eventId",e.eventId},{"executedAt",e.executedAt}});m_history=updated;emit historyChanged();
}
void ApplicationController::recordHistory(const QString&type,const QString&referenceId,const QString&title)
{
    if(m_historyRepository&&m_historyRepository->record(HistoryEntry{.itemType=type,.referenceId=referenceId,.title=title,.eventId=m_currentEventId}))refreshHistory();
}

} // namespace churchpresenter
