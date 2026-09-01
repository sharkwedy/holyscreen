#include "app/ApplicationController.h"
#include "app/contexts/AutomationContext.h"
#include "app/contexts/BibleContext.h"
#include "app/contexts/EventContext.h"
#include "app/contexts/IntegrationContext.h"
#include "app/contexts/MaintenanceContext.h"
#include "app/contexts/MediaContext.h"
#include "app/contexts/OutputContext.h"
#include "remote/RemoteQrCode.h"
#include "app/AppLogger.h"
#include "app/DiagnosticExporter.h"
#include "app/MediaThumbnailer.h"
#include "persistence/ApplicationDatabase.h"
#include "core/CommandCatalog.h"
#include "integrations/IntegrationSanitizer.h"
#include "integrations/secrets/SecretStoreFactory.h"
#include "screens/OutputRole.h"
#include "screens/OutputRouting.h"
#include "bible/BibleImportService.h"

#include <algorithm>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantMap>
#include <QRegularExpression>
#include <QUuid>
#include <QHash>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QWindow>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLocale>
#include <QMediaDevices>
#include <QSysInfo>
#include <QDateTime>
#include <QDirIterator>
#include <QSet>
#include <QRandomGenerator>
#include <QPointer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrentRun>

#include <utility>

namespace churchpresenter {

namespace {
QVariantMap mapMedia(const MediaItem &item, const QUrl &thumbnailSource = {})
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
        {QStringLiteral("thumbnailSource"), thumbnailSource},
        {QStringLiteral("typeLabel"), type == QStringLiteral("video")
             ? QCoreApplication::translate("ApplicationController", "VÍDEO")
             : type == QStringLiteral("audio")
             ? QCoreApplication::translate("ApplicationController", "ÁUDIO")
             : QCoreApplication::translate("ApplicationController", "IMAGEM")},
    };
}

QVariantMap mapCatalogEntry(const MediaCatalogEntry &entry, bool inPlaylist, bool favorite = false,
                            const QUrl &thumbnailSource = {})
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
        {QStringLiteral("thumbnailSource"), thumbnailSource},
        {QStringLiteral("inPlaylist"), inPlaylist},
        {QStringLiteral("favorite"), favorite},
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
    m_autosaveStatus = tr("Salvo");
    m_updateStatus = tr("Não verificado");
    m_automationContext = std::make_unique<AutomationContext>(*this);
    m_bibleContext = std::make_unique<BibleContext>(*this);
    m_eventContext = std::make_unique<EventContext>(*this);
    m_integrationContext = std::make_unique<IntegrationContext>(*this);
    m_maintenanceContext = std::make_unique<MaintenanceContext>(*this);
    m_mediaContext = std::make_unique<MediaContext>(*this);
    m_outputContext = std::make_unique<OutputContext>(*this);
    connect(&m_outputModule, &OutputModule::blackoutChanged,
            this, &ApplicationController::blackoutChanged);
    connect(&m_undoManager, &UndoManager::stateChanged,
            this, &ApplicationController::undoStateChanged);
    m_outputRoutingCommands = std::make_unique<OutputRoutingCommandModule>(
        m_commandBus, m_eventBus,
        OutputRoutingCommandModule::Actions{
            .output = [this](const QString &fingerprint) {
                return outputRoutingState(fingerprint);
            },
            .setEnabled = [this](const QString &fingerprint, bool enabled) {
                return applyToggleScreen(fingerprint, enabled);
            },
            .setRole = [this](const QString &fingerprint, const QString &role) {
                return applyOutputRole(fingerprint, role);
            },
            .setMediaEnabled = [this](const QString &fingerprint, bool enabled) {
                return applyOutputMediaEnabled(fingerprint, enabled);
            },
            .broadcastProfile = [this](const QString &fingerprint) {
                return outputRoutingState(fingerprint).isEmpty()
                    ? QVariantMap{} : outputBroadcastProfile(fingerprint);
            },
            .setBroadcastProfile = [this](const QString &fingerprint,
                                          const QVariantMap &changes) {
                return applyOutputBroadcastProfile(fingerprint, changes);
            },
        }, &m_undoManager, this);
    m_overlayCommands = std::make_unique<OverlayCommandModule>(
        m_commandBus, m_eventBus, m_overlays, &m_undoManager, this);
    m_mediaCommands = std::make_unique<MediaCommandModule>(
        m_commandBus, m_eventBus,
        MediaCommandModule::Actions{
            .play = [this](const QString &id) { return applyPlayMedia(id); },
            .togglePause = [this] { return applyToggleMediaPause(); },
            .stop = [this] { return applyStopMedia(); },
            .seek = [this](int positionMs) { return applySeekMedia(positionMs); },
            .previous = [this] { return applyPreviousMedia(); },
            .next = [this] { return applyNextMedia(); },
            .setRepeat = [this](const QString &mode) {
                applyMediaRepeatMode(mode);
                return true;
            },
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
        }, &m_undoManager, this);
    m_presentationCommands = std::make_unique<PresentationCommandModule>(
        m_commandBus, m_eventBus,
        PresentationCommandModule::Actions{
            .show = [this](int index) { return applyShowTextSlide(index); },
            .next = [this] { m_textPresentation.next(); return true; },
            .previous = [this] { m_textPresentation.previous(); return true; },
            .first = [this] { m_textPresentation.first(); return true; },
            .last = [this] { m_textPresentation.last(); return true; },
            .stop = [this] { m_textPresentation.stop(); return true; },
            .stateSnapshot = [this] {
                return QVariantMap{
                    {QStringLiteral("presentationId"), currentPresentationId()},
                    {QStringLiteral("presentationType"), currentPresentationType()},
                    {QStringLiteral("slideIndex"), currentSlideIndex()},
                    {QStringLiteral("slideId"), currentSlideId()},
                    {QStringLiteral("visible"), textVisible()},
                };
            },
        }, this);
    m_stageCommands = std::make_unique<StageCommandModule>(
        m_commandBus, m_eventBus,
        StageCommandModule::Actions{
            .message = [this] { return stageMessage(); },
            .setMessage = [this](const QString &message) {
                return applyStageMessage(message);
            },
        }, &m_undoManager, this);
    m_bibleCommands = std::make_unique<BibleCommandModule>(
        m_commandBus, m_eventBus,
        BibleCommandModule::Actions{
            .search = [this](const QString &reference) {
                return applyBibleSearch(reference);
            },
            .present = [this](int bookId, int chapter, int verse) {
                return applyBiblePresentation(bookId, chapter, verse);
            },
            .stateSnapshot = [this] {
                return QVariantMap{
                    {QStringLiteral("reference"), bibleReferenceInput()},
                    {QStringLiteral("resultCount"), bibleResults().size()},
                    {QStringLiteral("presentationId"), currentPresentationId()},
                    {QStringLiteral("slideIndex"), currentSlideIndex()},
                };
            },
        }, this);
    m_eventCommands = std::make_unique<EventCommandModule>(
        m_commandBus, m_eventBus,
        EventCommandModule::Actions{
            .select = [this](const QString &id) { return applyEventSelection(id); },
            .executeItem = [this](const QString &id) {
                return applyEventItemExecution(id);
            },
            .stateSnapshot = [this] {
                return QVariantMap{
                    {QStringLiteral("eventId"), currentEventId()},
                    {QStringLiteral("itemCount"), eventItems().size()},
                    {QStringLiteral("presentationId"), currentPresentationId()},
                    {QStringLiteral("mediaId"), currentMediaId()},
                };
            },
        }, this);
    m_themeCommands = std::make_unique<ThemeCommandModule>(
        m_commandBus, m_eventBus,
        ThemeCommandModule::Actions{
            .currentThemeId = [this] { return m_activeTheme.id; },
            .apply = [this](const QString &id) { return applyThemeSelection(id); },
            .stateSnapshot = [this] { return activeTheme(); },
        }, &m_undoManager, this);
    m_playlistCommands = std::make_unique<PlaylistCommandModule>(
        m_commandBus, m_eventBus,
        PlaylistCommandModule::Actions{
            .snapshot = [this] { return mediaPlaylist(); },
            .move = [this](const QString &id, int index) { return applyMoveMedia(id, index); },
            .remove = [this](const QString &id) { return applyRemoveMedia(id); },
            .clear = [this] { return applyClearMediaPlaylist(); },
            .restore = [this](const QVariantList &snapshot) {
                return restoreMediaPlaylist(snapshot);
            },
        }, &m_undoManager, this);
    m_autosave = std::make_unique<AutosaveCoordinator>(
        [this] { return persistCurrentPresentation(); }, this);
    connect(m_autosave.get(), &AutosaveCoordinator::dirtyChanged, this, [this](bool dirty) {
        m_autosaveStatus = dirty ? tr("Alterações pendentes…")
                                 : tr("Salvo");
        emit autosaveChanged();
    });
    connect(m_autosave.get(), &AutosaveCoordinator::saved, this, [this] {
        m_autosaveStatus = tr("Salvo automaticamente");
        emit autosaveChanged();
    });
    connect(m_autosave.get(), &AutosaveCoordinator::saveFailed,
            this, [this](const QString &message) {
        m_autosaveStatus = tr("Falha ao salvar");
        emit autosaveChanged();
        setStatusMessage(message);
    });
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->installEventFilter(this);
    }
    m_clockFontFamily = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    m_mediaCatalogDebounce.setSingleShot(true);
    m_mediaCatalogDebounce.setInterval(350);
    connect(&m_mediaCatalogDebounce, &QTimer::timeout,
            this, &ApplicationController::refreshMediaCatalog);
    connect(&m_mediaFolderWatcher, &QFileSystemWatcher::directoryChanged,
            this, [this] { m_mediaCatalogDebounce.start(); });
    connect(&m_mediaCatalogScanWatcher,
            &QFutureWatcher<MediaCatalogSnapshot>::finished, this, [this] {
        const auto scannedFolders = m_mediaCatalogScanFolders;
        const auto foldersChanged = scannedFolders != m_mediaFolderPaths;
        if (!foldersChanged) {
            auto snapshot = m_mediaCatalogScanWatcher.result();
            m_mediaCatalogEntries = std::move(snapshot.entries);
            m_mediaCatalogDirectories = std::move(snapshot.directories);
            m_thumbnailSourceCache.clear();
            saveMediaCatalogCache();
            rebuildMediaFolderWatcher();
            refreshMediaCatalogViews();
            refreshFavoriteMedia();
        }
        const auto shouldRescan = std::exchange(m_mediaCatalogRescanPending, false)
            || foldersChanged;
        if (shouldRescan) {
            QTimer::singleShot(0, this, &ApplicationController::refreshMediaCatalog);
        }
    });
    m_thumbnailRefreshDebounce.setSingleShot(true);
    m_thumbnailRefreshDebounce.setInterval(100);
    connect(&m_thumbnailRefreshDebounce, &QTimer::timeout, this, [this] {
        refreshAudioLibrary();
        refreshVideoLibrary();
        refreshImageLibrary();
        refreshMediaPlaylist();
    });
    m_mediaDevices = std::make_unique<QMediaDevices>();
    connect(m_mediaDevices.get(), &QMediaDevices::audioOutputsChanged, this, [this] {
        const bool selectedDeviceWasActive = !m_audioOutputId.isEmpty()
            && QString::fromLatin1(m_video.audioDevice().id().toHex()) == m_audioOutputId;
        applyAudioOutputSelection();
        emit audioOutputsChanged();
        if (selectedDeviceWasActive && !audioOutputConfigured()) {
            setStatusMessage(tr("A saída de áudio selecionada foi desconectada; usando a saída padrão."));
        }
    });
    m_remoteServer.setStateProvider([this] { return remoteState(); });
    m_remoteServer.setCommandDispatcher([this](const Command &command) {
        return m_commandBus.dispatch(command);
    });
    connect(&m_remoteServer, &LocalApiServer::statusChanged,
            this, &ApplicationController::remoteChanged);
    connect(&m_eventBus, &EventBus::eventPublished, this, [this](const DomainEvent &event) {
        m_remoteServer.broadcastEvent(event, m_commandBus.stateRevision() + 1);
    });
    connect(&m_commandBus, &CommandBus::commandDispatched, this,
            [this](const Command &, const CommandResult &result) {
        if (result.accepted) m_remoteServer.broadcastState();
    });
    loadSettings();
    refreshScreens();
    if (m_remoteEnabled && (!remotePasswordConfigured() || !restartRemoteServer())) {
        m_remoteEnabled = false;
        if (m_settings) m_settings->setValue(QStringLiteral("remote/enabled"), false);
    }

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
                             ? tr("O arquivo de mídia não foi encontrado.")
                             : tr("Não foi possível reproduzir a mídia selecionada."));
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
    connect(&m_updateChecker,&UpdateChecker::completed,this,[this](const UpdateRelease &release){
        m_updateRelease = release;
        const auto error = release.error;
        if(!error.isEmpty())m_updateStatus=tr("Falha ao verificar: %1").arg(error);
        else if(release.available){
            m_updateStatus = updateDownloadable()
                ? tr("Versão %1 disponível.").arg(release.latestVersion)
                : tr("Versão %1 disponível, mas sem pacote verificável para este sistema.")
                      .arg(release.latestVersion);
        }
        else m_updateStatus=tr("HolyScreen está atualizado (%1).").arg(QCoreApplication::applicationVersion());
        emit updateChanged();
    });

    connect(&m_updateDownloader, &UpdateDownloader::progress, this,
            [this](qint64 received, qint64 total) {
        m_updateDownloadProgress = total > 0
            ? static_cast<double>(received) / static_cast<double>(total) : 0.0;
        emit updateDownloadChanged();
    });
    connect(&m_updateDownloader, &UpdateDownloader::finished, this,
            [this](const QString &path, const QString &error) {
        m_updateDownloading = false;
        m_updateDownloadedPath = error.isEmpty() ? path : QString{};
        if (!error.isEmpty()) {
            m_updateDownloadedSha256.clear();
            m_updateDownloadedSize = 0;
        }
        m_updateDownloadProgress = error.isEmpty() ? 1.0 : 0.0;
        emit updateDownloadChanged();
        m_updateStatus = error.isEmpty()
            ? tr("Pacote verificado e salvo em %1.").arg(path)
            : tr("Falha no download: %1").arg(error);
        emit updateChanged();
    });

    // O HolyScreen só verifica atualizações quando o operador pede ou quando
    // autoriza a verificação automática. Uma vez por dia é suficiente e não
    // interfere numa operação ao vivo, porque nada é baixado sem um clique.
    m_updateCheckTimer.setInterval(24 * 60 * 60 * 1000);
    connect(&m_updateCheckTimer, &QTimer::timeout, this, &ApplicationController::checkForUpdates);
    if (m_automaticUpdateChecks) {
        m_updateCheckTimer.start();
        // Uma pequena espera evita competir com a abertura das saídas.
        QTimer::singleShot(5000, this, &ApplicationController::checkForUpdates);
    }
    connect(&m_bibleImportWatcher, &QFutureWatcher<BibleImportResult>::finished,
            this, [this] { finishBibleImport(m_bibleImportWatcher.result()); });
}

ApplicationController::~ApplicationController()
{
    m_remoteServer.stop();
    // Nenhuma chamada externa pode sobreviver ao encerramento do culto.
    m_integrations.cancelAll();
    if (m_bibleImportCancelled) m_bibleImportCancelled->store(true);
    if (m_bibleImportWatcher.isRunning()) m_bibleImportWatcher.waitForFinished();
    if (m_autosave) m_autosave->flush();
    if (m_recovery) m_recovery->endSession();
}

bool ApplicationController::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::KeyPress) {
        return QObject::eventFilter(watched, event);
    }
    const auto *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->modifiers().testAnyFlags(
            Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        return QObject::eventFilter(watched, event);
    }
    const auto text = keyEvent->text();
    if (text.size() != 1 || !text.front().isLetter()) {
        return QObject::eventFilter(watched, event);
    }

    const auto *focusObject = QGuiApplication::focusObject();
    if (focusObject) {
        const auto className = QString::fromLatin1(focusObject->metaObject()->className());
        const bool textEditor = className.contains(QStringLiteral("TextInput"))
            || className.contains(QStringLiteral("TextEdit"))
            || className.contains(QStringLiteral("TextField"))
            || className.contains(QStringLiteral("TextArea"));
        const bool editableControl = focusObject->property("editable").toBool();
        const bool textControl = focusObject->property("text").isValid()
            && focusObject->property("readOnly").isValid();
        if (textEditor || editableControl || textControl) {
            return QObject::eventFilter(watched, event);
        }
    }

    emit quickBibleSearchRequested(text);
    return true;
}

CommandBus &ApplicationController::commandBus() { return m_commandBus; }
EventBus &ApplicationController::eventBus() { return m_eventBus; }
AutomationContext *ApplicationController::automationContext() const { return m_automationContext.get(); }
BibleContext *ApplicationController::bibleContext() const { return m_bibleContext.get(); }
EventContext *ApplicationController::eventContext() const { return m_eventContext.get(); }
IntegrationContext *ApplicationController::integrationContext() const { return m_integrationContext.get(); }
MaintenanceContext *ApplicationController::maintenanceContext() const { return m_maintenanceContext.get(); }
MediaContext *ApplicationController::mediaContext() const { return m_mediaContext.get(); }
OutputContext *ApplicationController::outputContext() const { return m_outputContext.get(); }
QString ApplicationController::locale() const { return m_locale; }
bool ApplicationController::localeRestartRequired() const
{
    const auto selected = QLocale(m_locale == QStringLiteral("en-US")
                                      ? QStringLiteral("en_US")
                                      : QStringLiteral("pt_BR"));
    return selected.name() != QLocale().name();
}
bool ApplicationController::demoMode() const { return m_demoMode; }
double ApplicationController::interfaceScale() const { return m_interfaceScale; }
bool ApplicationController::onboardingCompleted() const { return m_onboardingCompleted; }
QStringList ApplicationController::onboardingSkippedSteps() const
{
    return m_onboardingSkippedSteps;
}
QVariantMap ApplicationController::shortcuts() const { return m_shortcuts; }

void ApplicationController::setLocale(const QString &locale)
{
    if (locale != QStringLiteral("pt-BR") && locale != QStringLiteral("en-US")) return;
    if (m_locale == locale) return;
    m_locale = locale;
    if (m_settings) m_settings->setValue(QStringLiteral("operator/locale"), locale);
    emit preferencesChanged();
}

void ApplicationController::setDemoMode(bool enabled)
{
    if (m_demoMode == enabled) return;
    m_demoMode = enabled;
    if (m_settings) m_settings->setValue(QStringLiteral("operator/demoMode"), enabled);
    emit preferencesChanged();
}

void ApplicationController::setInterfaceScale(double scale)
{
    if (scale != 1.0 && scale != 1.5 && scale != 2.0) return;
    if (m_interfaceScale == scale) return;
    m_interfaceScale = scale;
    if (m_settings) m_settings->setValue(QStringLiteral("operator/interfaceScale"), scale);
    emit preferencesChanged();
}

void ApplicationController::completeOnboarding()
{
    if (m_onboardingCompleted) return;
    m_onboardingCompleted = true;
    if (m_settings) m_settings->setValue(QStringLiteral("operator/onboardingCompleted"), true);
    emit onboardingChanged();
}

void ApplicationController::reopenOnboarding()
{
    if (!m_onboardingCompleted) return;
    m_onboardingCompleted = false;
    if (m_settings) m_settings->setValue(QStringLiteral("operator/onboardingCompleted"), false);
    emit onboardingChanged();
}

bool ApplicationController::skipOnboardingStep(const QString &stepId)
{
    static const QStringList allowedSteps{
        QStringLiteral("screens"), QStringLiteral("audio"), QStringLiteral("library"),
        QStringLiteral("bible"), QStringLiteral("remote"), QStringLiteral("broadcast"),
    };
    if (!allowedSteps.contains(stepId) || m_onboardingSkippedSteps.contains(stepId)) return false;
    m_onboardingSkippedSteps.append(stepId);
    if (m_settings) {
        m_settings->setValue(QStringLiteral("operator/onboardingSkippedSteps"),
                             m_onboardingSkippedSteps);
    }
    emit onboardingChanged();
    return true;
}

bool ApplicationController::resumeOnboardingStep(const QString &stepId)
{
    if (!m_onboardingSkippedSteps.removeOne(stepId)) return false;
    if (m_settings) {
        m_settings->setValue(QStringLiteral("operator/onboardingSkippedSteps"),
                             m_onboardingSkippedSteps);
    }
    emit onboardingChanged();
    return true;
}

bool ApplicationController::setShortcut(const QString &action, const QString &sequence)
{
    if (!m_shortcuts.contains(action)) return false;
    const auto trimmed = sequence.trimmed();
    if (!trimmed.isEmpty()
        && QKeySequence::fromString(trimmed, QKeySequence::PortableText).isEmpty()) return false;
    for (auto it = m_shortcuts.cbegin(); it != m_shortcuts.cend(); ++it) {
        if (it.key() != action && !trimmed.isEmpty()
            && it.value().toString().compare(trimmed, Qt::CaseInsensitive) == 0) return false;
    }
    if (m_shortcuts.value(action).toString() == trimmed) return true;
    m_shortcuts.insert(action, trimmed);
    if (m_settings) m_settings->setValue(QStringLiteral("operator/shortcuts"), m_shortcuts);
    emit preferencesChanged();
    return true;
}

QVariantList ApplicationController::screens() const { return m_screens; }

QVariantList ApplicationController::outputWindows() const
{
    return m_outputs.describeOutputWindows(m_screenManager->screens());
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
bool ApplicationController::clockFontBold() const { return m_clockFontBold; }
bool ApplicationController::clockFontItalic() const { return m_clockFontItalic; }
QString ApplicationController::clockBackgroundColor() const { return m_clockBackgroundColor; }
double ApplicationController::clockLineHeight() const { return m_clockLineHeight; }
int ApplicationController::clockCornerRadius() const { return m_clockCornerRadius; }
double ApplicationController::clockTextOpacity() const { return m_clockTextOpacity; }
double ApplicationController::clockBackgroundOpacity() const { return m_clockBackgroundOpacity; }
int ApplicationController::clockMarginHorizontal() const { return m_clockMarginHorizontal; }
int ApplicationController::clockMarginVertical() const { return m_clockMarginVertical; }
QString ApplicationController::clockEffect() const { return m_clockEffect; }
int ApplicationController::simulatedOutputCount() const { return m_simulatedOutputCount; }
bool ApplicationController::debugEnabled() const { return m_debugEnabled; }
bool ApplicationController::debugSimulatedOutputs() const { return m_debugSimulatedOutputs; }
bool ApplicationController::debugDiagnostics() const { return m_debugDiagnostics; }
bool ApplicationController::debugLogging() const { return m_debugLogging; }
bool ApplicationController::remoteEnabled() const { return m_remoteEnabled; }
int ApplicationController::remotePort() const { return m_remotePort; }
QString ApplicationController::remoteInterface() const { return m_remoteInterface; }
bool ApplicationController::remotePasswordConfigured() const { return m_remoteServer.hasPassword(); }
QString ApplicationController::remoteUrl() const { return m_remoteServer.remoteUrl(); }
QString ApplicationController::remoteQrCode() const
{
    return RemoteQrCode::svgDataUrl(remoteUrl());
}
int ApplicationController::remoteClients() const { return m_remoteServer.clientCount(); }
int ApplicationController::remoteSessions() const { return m_remoteServer.sessionCount(); }
QString ApplicationController::remoteError() const { return m_remoteServer.lastError(); }
bool ApplicationController::blackout() const { return m_outputModule.blackout(); }
bool ApplicationController::canUndo() const { return m_undoManager.canUndo(); }
bool ApplicationController::canRedo() const { return m_undoManager.canRedo(); }
QString ApplicationController::undoLabel() const { return m_undoManager.undoLabel(); }
QString ApplicationController::redoLabel() const { return m_undoManager.redoLabel(); }
bool ApplicationController::autosavePending() const { return m_autosave && m_autosave->dirty(); }
QString ApplicationController::autosaveStatus() const { return m_autosaveStatus; }
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
QVariantList ApplicationController::favoriteMedia() const { return m_favoriteMedia; }
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
QVariantList ApplicationController::audioOutputs() const
{
    QVariantList outputs;
    const auto defaultDevice = QMediaDevices::defaultAudioOutput();
    for (const auto &device : QMediaDevices::audioOutputs()) {
        outputs.append(QVariantMap{
            {QStringLiteral("id"), QString::fromLatin1(device.id().toHex())},
            {QStringLiteral("displayName"), device.description()},
            {QStringLiteral("isDefault"), device == defaultDevice},
        });
    }
    return outputs;
}
QString ApplicationController::audioOutputId() const { return m_audioOutputId; }
bool ApplicationController::audioOutputConfigured() const
{
    if (m_audioOutputId.isEmpty()) return false;
    return std::ranges::any_of(QMediaDevices::audioOutputs(), [this](const QAudioDevice &device) {
        return QString::fromLatin1(device.id().toHex()) == m_audioOutputId;
    });
}
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
QString ApplicationController::bibleThemeId() const { return m_bibleThemeId; }
QString ApplicationController::lyricsThemeId() const { return m_lyricsThemeId; }
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
bool ApplicationController::automaticUpdateChecks()const{return m_automaticUpdateChecks;}
bool ApplicationController::updateAvailable()const{return m_updateRelease.available;}
QString ApplicationController::updateLatestVersion()const{return m_updateRelease.latestVersion;}
QString ApplicationController::updateReleaseUrl()const{return m_updateRelease.releaseUrl.toString();}
QString ApplicationController::updateAssetName()const{return m_updateRelease.assetName;}
bool ApplicationController::updateDownloadable()const
{
    return m_updateRelease.available && !m_updateRelease.assetName.isEmpty()
        && m_updateRelease.sha256.size() == 64 && m_updateRelease.assetSize > 0
        && UpdateChecker::isTrustedDownloadUrl(m_updateRelease.downloadUrl);
}
bool ApplicationController::updateDownloading()const{return m_updateDownloading;}
double ApplicationController::updateDownloadProgress()const{return m_updateDownloadProgress;}
QString ApplicationController::updateDownloadedPath()const{return m_updateDownloadedPath;}
bool ApplicationController::updateInstallable()const
{
    return m_updateInstaller.canInstall(m_updateDownloadedPath);
}
QVariantList ApplicationController::bibleTranslations() const{return m_bibleTranslations;}
bool ApplicationController::bibleImportRunning() const{return m_bibleImportActive;}
int ApplicationController::bibleImportProgress() const
{
    if (m_bibleImportProgressTotal <= 0) return 0;
    return std::clamp((m_bibleImportProgressCurrent * 100) / m_bibleImportProgressTotal, 0, 100);
}
QString ApplicationController::bibleImportMessage() const{return m_bibleImportMessage;}
bool ApplicationController::bibleImportRequiresLicenseConfirmation() const{return m_bibleImportRequiresLicenseConfirmation;}
QString ApplicationController::bibleImportLicenseWarning() const
{
    return m_bibleImportRestrictedTranslations.join(QStringLiteral("\n"));
}
QVariantList ApplicationController::bibleBooks() const
{
    QVariantList result;
    const auto lastBook = static_cast<int>(BibleBook::Revelation);
    const auto lastOldTestamentBook = static_cast<int>(BibleBook::Malachi);
    for (int bookId = static_cast<int>(BibleBook::Genesis); bookId <= lastBook; ++bookId) {
        result.append(QVariantMap{
            {QStringLiteral("id"), bookId},
            {QStringLiteral("name"), bibleBookName(static_cast<BibleBook>(bookId))},
            {QStringLiteral("testament"), bookId <= lastOldTestamentBook
                 ? QStringLiteral("old") : QStringLiteral("new")},
        });
    }
    return result;
}
QString ApplicationController::biblePrimaryTranslationId() const{return m_biblePrimaryTranslationId;}
QString ApplicationController::bibleSecondaryTranslationId() const{return m_bibleSecondaryTranslationId;}
QString ApplicationController::bibleTertiaryTranslationId() const{return m_bibleTertiaryTranslationId;}
QString ApplicationController::bibleReferenceInput() const{return m_bibleReferenceInput;}
QVariantList ApplicationController::bibleResults() const{return m_bibleResults;}
QVariantList ApplicationController::favoriteBibleVerses() const
{
    return m_favoriteBibleVerses;
}

bool ApplicationController::toggleScreen(const QString &screenFingerprint, bool enabled)
{
    return m_outputRoutingCommands->requestEnabled(screenFingerprint, enabled).accepted;
}

bool ApplicationController::applyToggleScreen(const QString &screenFingerprint, bool enabled)
{
    const auto found = std::find_if(m_screens.cbegin(), m_screens.cend(), [&](const QVariant &entry) {
        return entry.toMap().value(QStringLiteral("id")).toString() == screenFingerprint;
    });
    if (found == m_screens.cend()) {
        return false;
    }

    const auto item = found->toMap();
    if (item.value(QStringLiteral("primary")).toBool()) {
        setStatusMessage(tr("A tela principal é reservada para o operador."));
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
                                 ? tr("O limite de cinco saídas foi atingido.")
                                 : tr("A tela não está mais conectada."));
            return false;
        }
    } else {
        m_outputs.disable(screenFingerprint);
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
                         ? tr("Todas as telas externas conectadas estão ativas.")
                         : tr("Nenhuma tela externa conectada foi encontrada."));
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
    return m_outputRoutingCommands->requestRole(screenFingerprint, role).accepted;
}

bool ApplicationController::applyOutputRole(const QString &screenFingerprint, const QString &role)
{
    if (!m_outputs.setRole(screenFingerprint, role)) return false;
    saveOutputs();
    refreshScreens();
    return true;
}

bool ApplicationController::setOutputMediaEnabled(const QString &screenFingerprint, bool enabled)
{
    return m_outputRoutingCommands->requestMediaEnabled(screenFingerprint, enabled).accepted;
}

bool ApplicationController::applyOutputMediaEnabled(const QString &screenFingerprint, bool enabled)
{
    if (!m_outputs.setMediaEnabled(screenFingerprint, enabled)) return false;
    saveOutputs();
    refreshScreens();
    return true;
}

QVariantMap ApplicationController::outputBroadcastProfile(const QString &screenFingerprint) const
{
    return broadcastProfileToMap(m_outputs.broadcastProfile(screenFingerprint));
}

bool ApplicationController::setOutputBroadcastProfile(const QString &screenFingerprint,
                                                      const QVariantMap &changes)
{
    return m_outputRoutingCommands->requestBroadcastProfile(screenFingerprint, changes).accepted;
}

bool ApplicationController::applyOutputBroadcastProfile(const QString &screenFingerprint,
                                                        const QVariantMap &changes)
{
    const auto merged = m_outputs.mergeBroadcastProfile(screenFingerprint, changes);
    if (m_broadcastProfiles && !m_broadcastProfiles->save(merged)) {
        setStatusMessage(tr("O perfil de transmissão não pôde ser salvo."));
        return false;
    }
    refreshScreens();
    return true;
}

void ApplicationController::setupIntegrations(const QString &databasePath)
{
    m_secretStore = SecretStoreFactory::create();
    m_httpAdapter = std::make_unique<HttpIntegrationAdapter>(m_httpTransport, m_secretStore.get());
    m_webSocketAdapter = std::make_unique<WebSocketIntegrationAdapter>(m_webSocketTransport);
    m_obsAdapter = std::make_unique<ObsIntegrationAdapter>(m_obsClient, m_secretStore.get());
    m_midiAdapter = std::make_unique<MidiIntegrationAdapter>(m_midiTransport);
    m_oscAdapter = std::make_unique<OscIntegrationAdapter>(m_oscTransport);
    m_integrations.registerAdapter(IntegrationType::Http, m_httpAdapter.get());
    m_integrations.registerAdapter(IntegrationType::WebSocket, m_webSocketAdapter.get());
    m_integrations.registerAdapter(IntegrationType::Obs, m_obsAdapter.get());
    m_integrations.registerAdapter(IntegrationType::Midi, m_midiAdapter.get());
    m_integrations.registerAdapter(IntegrationType::Osc, m_oscAdapter.get());
    m_integrations.setSecretStore(m_secretStore.get());

    m_integrationRepository = std::make_unique<IntegrationRepository>(databasePath);
    if (!m_integrationRepository->open()) {
        qWarning() << "Failed to open the integration database";
    } else {
        m_integrations.setRepository(m_integrationRepository.get());
    }

    m_integrationCommands = std::make_unique<IntegrationCommandModule>(
        m_commandBus, m_eventBus,
        IntegrationCommandModule::Actions{
            .execute = [this](const QString &integrationId, const QString &operation,
                              const QVariantMap &payload, const QString &correlationId) {
                return runIntegration(integrationId, operation, payload, correlationId, false);
            },
            .test = [this](const QString &integrationId, const QString &correlationId) {
                return runIntegration(integrationId, QStringLiteral("connection.test"), {},
                                      correlationId, true);
            },
            .definition = [this](const QString &integrationId) {
                return integrationDefinition(integrationId);
            },
        }, this);
    refreshIntegrationDiagnostics();
    emit integrationsChanged();
}

QVariantMap ApplicationController::runIntegration(const QString &integrationId,
                                                  const QString &operation,
                                                  const QVariantMap &payload,
                                                  const QString &correlationId, bool isTest)
{
    // As chamadas são assíncronas; o operador acompanha pelo estado e pelo
    // histórico, sem diálogo modal travando o culto.
    QVariantMap acknowledgement{{QStringLiteral("accepted"), true},
                                {QStringLiteral("message"), tr("Chamada enviada.")}};
    const auto completion = [this, integrationId](const IntegrationResult &result) {
        const auto name = m_integrations.definition(integrationId)
                              .value_or(IntegrationDefinition{}).name;
        setIntegrationStatus(result.accepted
            ? tr("%1: %2 (%3 ms)").arg(name, result.message).arg(result.durationMs)
            : tr("%1 falhou: %2").arg(name,
                  result.message.isEmpty() ? result.errorCode : result.message));
        emit integrationHistoryChanged();
    };

    if (isTest) {
        m_integrations.test(integrationId, completion);
        return acknowledgement;
    }
    m_integrations.execute(IntegrationRequest{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .integrationId = integrationId,
        .operation = operation,
        .payload = payload,
        .correlationId = correlationId,
        .issuedAt = QDateTime::currentDateTimeUtc(),
    }, completion);
    return acknowledgement;
}

void ApplicationController::setupAutomations(const QString &databasePath)
{
    m_automationRepository = std::make_unique<AutomationRepository>(databasePath);
    if (!m_automationRepository->open()) {
        qWarning() << "Failed to open the automation database";
    } else {
        m_automationEngine.setAutomations(m_automationRepository->automations());
        m_authorizedExecutables.restore(m_automationRepository->authorizedExecutables());
    }

    if (m_settings) {
        m_automationEngine.setEnabled(
            m_settings->value(QStringLiteral("automation/enabled"), true).toBool());
        m_authorizedExecutables.setEnabled(
            m_settings->value(QStringLiteral("automation/processActions"), false).toBool());
    }

    m_automationEngine.setPorts(AutomationEngine::Ports{
        .dispatchCommand = [this](const QString &type, const QVariantMap &payload,
                                  const QString &correlationId) {
            if (!CommandCatalog::contains(type)) {
                return ActionOutcome{.actionType = QStringLiteral("command"),
                                     .accepted = false,
                                     .errorCode = QStringLiteral("unknown_command"),
                                     .message = tr("Comando fora do catálogo: %1.")
                                                    .arg(type)};
            }
            const auto result = m_commandBus.dispatch(Command{
                .id = correlationId,
                .type = type,
                .payload = payload,
                .source = QStringLiteral("automation"),
                .issuedAt = QDateTime::currentDateTimeUtc(),
            });
            return ActionOutcome{.actionType = QStringLiteral("command"),
                                 .accepted = result.accepted,
                                 .errorCode = result.errorCode,
                                 .message = result.message};
        },
        .runIntegration = [this](const QString &integrationId, const QString &operation,
                                 const QVariantMap &payload, const QString &correlationId) {
            const auto acknowledgement = runIntegration(integrationId, operation, payload,
                                                        correlationId, false);
            return ActionOutcome{
                .actionType = QStringLiteral("integration"),
                .accepted = acknowledgement.value(QStringLiteral("accepted")).toBool(),
                .errorCode = acknowledgement.value(QStringLiteral("errorCode")).toString(),
                .message = acknowledgement.value(QStringLiteral("message")).toString(),
            };
        },
        .runProcess = [this](const QVariantMap &parameters, const QString &) {
            QString errorCode;
            QString message;
            const auto request = m_authorizedExecutables.validate(parameters, &errorCode, &message);
            if (!request.has_value()) {
                return ActionOutcome{.actionType = QStringLiteral("process"),
                                     .accepted = false,
                                     .errorCode = errorCode,
                                     .message = message};
            }
            m_processRunner.run(*request, [this, executable = request->executable](
                                              const ProcessResult &result) {
                setAutomationStatus(result.finished && result.exitCode == 0
                    ? tr("Processo %1 concluído.").arg(QFileInfo(executable).fileName())
                    : tr("Processo %1 falhou: %2")
                          .arg(QFileInfo(executable).fileName(), result.message));
            });
            return ActionOutcome{.actionType = QStringLiteral("process"),
                                 .accepted = true,
                                 .message = tr("Processo iniciado.")};
        },
        .state = [this] {
            return QVariantMap{
                {QStringLiteral("blackout"), blackout()},
                {QStringLiteral("activeOutputs"), m_outputs.activeOutputs().size()},
                {QStringLiteral("mediaState"), mediaState()},
                {QStringLiteral("currentMediaTitle"), currentMediaTitle()},
                {QStringLiteral("eventId"), currentEventId()},
                {QStringLiteral("presentationTitle"), currentPresentationTitle()},
            };
        },
    });

    // Fatos do domínio viram gatilhos; resultados de integração e de automação
    // nunca voltam como gatilho, para não realimentar a cadeia.
    connect(&m_eventBus, &EventBus::eventPublished, this, [this](const DomainEvent &event) {
        const auto match = TriggerTranslator::translate(event);
        if (!match.has_value()) return;
        m_automationEngine.handleTrigger(match->triggerType, match->payload,
                                         match->correlationId);
        if (match->triggerType == QLatin1StringView(AutomationTrigger::PresentationStarted)
            && event.payload.value(QStringLiteral("presentationType")).toString()
                   == QStringLiteral("song")) {
            m_automationEngine.handleTrigger(
                QString::fromLatin1(AutomationTrigger::SongStarted), match->payload,
                match->correlationId);
        }
    });

    connect(&m_commandBus, &CommandBus::commandDispatched, this,
            [this](const Command &command, const CommandResult &result) {
        if (!result.accepted) return;
        if (command.source.startsWith(QStringLiteral("remote:"))) {
            m_eventBus.publish(DomainEvent{
                .type = QStringLiteral("automation.remote.command"),
                .payload = {{QStringLiteral("type"), command.type},
                            {QStringLiteral("payload"), command.payload},
                            {QStringLiteral("source"), command.source}},
                .occurredAt = QDateTime::currentDateTimeUtc(),
                .correlationId = command.id,
            });
        }

        QString timerEvent;
        QString timerType;
        if (command.type == QStringLiteral("timer.countdown.start")) {
            timerEvent = QStringLiteral("automation.timer.started");
            timerType = QStringLiteral("countdown");
        } else if (command.type == QStringLiteral("timer.stopwatch.start")) {
            timerEvent = QStringLiteral("automation.timer.started");
            timerType = QStringLiteral("stopwatch");
        } else if (command.type == QStringLiteral("timer.countdown.stop")) {
            timerEvent = QStringLiteral("automation.timer.finished");
            timerType = QStringLiteral("countdown");
        } else if (command.type == QStringLiteral("timer.stopwatch.pause")
                   || command.type == QStringLiteral("timer.stopwatch.reset")) {
            timerEvent = QStringLiteral("automation.timer.finished");
            timerType = QStringLiteral("stopwatch");
        }
        if (!timerEvent.isEmpty()) {
            m_eventBus.publish(DomainEvent{
                .type = timerEvent,
                .payload = {{QStringLiteral("timerType"), timerType},
                            {QStringLiteral("commandType"), command.type}},
                .occurredAt = QDateTime::currentDateTimeUtc(),
                .correlationId = command.id,
            });
        }
    });

    connect(&m_overlays, &OverlayController::countdownExpired, this, [this] {
        m_eventBus.publish(DomainEvent{
            .type = QStringLiteral("automation.timer.finished"),
            .payload = {{QStringLiteral("timerType"), QStringLiteral("countdown")},
                        {QStringLiteral("reason"), QStringLiteral("elapsed")}},
            .occurredAt = QDateTime::currentDateTimeUtc(),
            .correlationId = QUuid::createUuid().toString(QUuid::WithoutBraces),
        });
    });

    connect(&m_localTimeTriggerScheduler,
            &LocalTimeTriggerScheduler::localTimeOccurred, this,
            [this](const QVariantMap &payload, const QString &correlationId) {
        m_automationEngine.handleTrigger(
            QString::fromLatin1(AutomationTrigger::LocalTime), payload, correlationId);
    });
    m_localTimeTriggerScheduler.start();

    connect(&m_automationEngine, &AutomationEngine::runFinished, this,
            [this](const AutomationRun &run) {
        if (m_automationRepository) {
            m_automationRepository->recordRun(run);
            m_automationRepository->pruneRuns(AutomationRepository::DefaultRunRetention);
        }
        const auto automation = m_automationEngine.automation(run.automationId);
        const auto name = automation ? automation->name : run.automationId;
        if (run.status == QStringLiteral("failed")) {
            setAutomationStatus(tr("%1 falhou: %2")
                                    .arg(name, run.outcomes.isEmpty()
                                                   ? run.reason
                                                   : run.outcomes.last().message));
        } else if (run.status == QStringLiteral("blocked")) {
            setAutomationStatus(tr("%1 bloqueada: %2").arg(name, run.reason));
        } else if (run.status == QStringLiteral("completed")) {
            setAutomationStatus(tr("%1 executada.").arg(name));
        }
        emit automationRunsChanged();
    });

    connect(&m_automationEngine, &AutomationEngine::automationDisabled, this,
            [this](const QString &automationId, const QString &reason) {
        const auto automation = m_automationEngine.automation(automationId);
        if (m_automationRepository && automation) {
            m_automationRepository->updateRuntimeState(automationId, false,
                                                       automation->consecutiveFailures);
        }
        setAutomationStatus(tr("%1 %2")
                                .arg(automation ? automation->name : automationId, reason));
    });

    connect(&m_automationEngine, &AutomationEngine::automationsChanged, this,
            &ApplicationController::automationsChanged);
    emit automationsChanged();
    emit authorizedExecutablesChanged();
}

QVariantList ApplicationController::automations() const
{
    QVariantList result;
    for (const auto &automation : m_automationEngine.automations()) {
        auto item = automationToMap(automation);
        item.insert(QStringLiteral("actionCount"), static_cast<int>(automation.actions.size()));
        result.append(item);
    }
    return result;
}

QVariantList ApplicationController::automationRuns() const
{
    QVariantList result;
    if (!m_automationRepository) return result;
    for (const auto &run : m_automationRepository->runs({}, 100)) {
        result.append(automationRunToMap(run));
    }
    return result;
}

QString ApplicationController::automationStatus() const { return m_automationStatus; }

void ApplicationController::setAutomationStatus(const QString &message)
{
    if (m_automationStatus == message) return;
    m_automationStatus = message;
    emit automationStatusChanged();
}

bool ApplicationController::automationsEnabled() const { return m_automationEngine.isEnabled(); }

void ApplicationController::setAutomationsEnabled(bool enabled)
{
    if (m_automationEngine.isEnabled() == enabled) return;
    m_automationEngine.setEnabled(enabled);
    if (m_settings) m_settings->setValue(QStringLiteral("automation/enabled"), enabled);
    setAutomationStatus(enabled ? tr("Automações ativas.")
                                : tr("Automações pausadas."));
    emit automationsChanged();
}

bool ApplicationController::processActionsEnabled() const
{
    return m_authorizedExecutables.isEnabled();
}

void ApplicationController::setProcessActionsEnabled(bool enabled)
{
    if (m_authorizedExecutables.isEnabled() == enabled) return;
    m_authorizedExecutables.setEnabled(enabled);
    if (m_settings) m_settings->setValue(QStringLiteral("automation/processActions"), enabled);
    setAutomationStatus(enabled
        ? tr("Processos externos liberados para a lista autorizada.")
        : tr("Processos externos desativados."));
    emit authorizedExecutablesChanged();
}

QVariantList ApplicationController::authorizedExecutables() const
{
    QVariantList result;
    for (const auto &entry : m_authorizedExecutables.entries()) {
        result.append(QVariantMap{
            {QStringLiteral("canonicalPath"), entry.canonicalPath},
            {QStringLiteral("label"), entry.label},
            {QStringLiteral("authorizedAt"), QLocale().toString(
                 entry.authorizedAt.toLocalTime(), QLocale::ShortFormat)},
        });
    }
    return result;
}

QStringList ApplicationController::automationTriggerTypeList() const
{
    return automationTriggerTypes();
}

QStringList ApplicationController::automationActionTypeList() const
{
    return automationActionTypes();
}

QStringList ApplicationController::automationConditionOperationList() const
{
    return automationConditionOperations();
}

QStringList ApplicationController::validateAutomation(const Automation &automation) const
{
    QStringList errors;
    if (automation.name.trimmed().isEmpty()) {
        errors.append(tr("A automação precisa de um nome."));
    }
    if (!automationTriggerTypes().contains(automation.trigger.type)) {
        errors.append(tr("Escolha um gatilho válido."));
    }
    if (automation.trigger.type == QLatin1StringView(AutomationTrigger::LocalTime)) {
        const auto time = QTime::fromString(
            automation.trigger.parameters.value(QStringLiteral("time")).toString(),
            QStringLiteral("HH:mm"));
        const auto days = automation.trigger.parameters.value(QStringLiteral("daysOfWeek")).toList();
        const auto invalidDay = std::any_of(days.cbegin(), days.cend(), [](const QVariant &day) {
            return day.toInt() < 1 || day.toInt() > 7;
        });
        if (!time.isValid() || days.isEmpty() || invalidDay) {
            errors.append(tr("Informe horário e dias válidos para o gatilho local."));
        }
    }
    if (automation.actions.isEmpty()) {
        errors.append(tr("Adicione pelo menos uma ação."));
    }
    if (automation.actions.size() > m_automationEngine.limits().maximumActionsPerRun) {
        errors.append(tr("No máximo %1 ações por automação.")
                          .arg(m_automationEngine.limits().maximumActionsPerRun));
    }
    for (const auto &condition : automation.conditions) {
        if (condition.field.trimmed().isEmpty()
            || !automationConditionOperations().contains(condition.operation)) {
            errors.append(tr("Condição incompleta ou com operação inválida."));
            break;
        }
    }
    for (const auto &action : automation.actions) {
        if (!automationActionTypes().contains(action.type)) {
            errors.append(tr("Ação não suportada: %1.").arg(action.type));
            continue;
        }
        if (action.type == QLatin1StringView(AutomationAction::Command)) {
            const auto type = action.parameters.value(QStringLiteral("type")).toString();
            if (!CommandCatalog::contains(type)) {
                errors.append(tr("Comando fora do catálogo: %1.").arg(type));
            }
        } else if (action.type == QLatin1StringView(AutomationAction::Integration)) {
            const auto id = action.parameters.value(QStringLiteral("integrationId")).toString();
            if (!m_integrations.definition(id).has_value()) {
                errors.append(tr("Integração não encontrada: %1.").arg(id));
            }
        } else if (action.type == QLatin1StringView(AutomationAction::Process)) {
            const auto executable = action.parameters.value(QStringLiteral("executable"))
                                        .toString();
            if (!m_authorizedExecutables.isAuthorized(executable)) {
                errors.append(tr("O executável %1 não está autorizado.")
                                  .arg(executable));
            }
        }
    }
    return errors;
}

QVariantMap ApplicationController::saveAutomation(const QVariantMap &automation)
{
    auto parsed = automationFromMap(automation);
    if (parsed.id.isEmpty()) parsed.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto errors = validateAutomation(parsed);
    if (!errors.isEmpty()) {
        return {{QStringLiteral("accepted"), false}, {QStringLiteral("errors"), errors}};
    }
    if (!m_automationRepository || !m_automationRepository->save(parsed)) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"),
                 QStringList{tr("A automação não pôde ser salva.")}}};
    }
    m_automationEngine.setAutomations(m_automationRepository->automations());
    setAutomationStatus(tr("Automação %1 salva.").arg(parsed.name));
    emit automationsChanged();
    return {{QStringLiteral("accepted"), true},
            {QStringLiteral("errors"), QStringList{}},
            {QStringLiteral("id"), parsed.id}};
}

QVariantMap ApplicationController::exportAutomations(const QUrl &destination)
{
    auto path = destination.isLocalFile() ? destination.toLocalFile() : destination.toString();
    if (path.trimmed().isEmpty()) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("error"), tr("Escolha o arquivo de destino.")}};
    }
    if (!path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        path.append(QStringLiteral(".json"));
    }

    QJsonArray definitions;
    for (const auto &automation : m_automationEngine.automations()) {
        auto exported = automationToMap(automation);
        exported.remove(QStringLiteral("consecutiveFailures"));
        definitions.append(QJsonObject::fromVariantMap(exported));
    }
    const QJsonObject document{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("exportedAt"),
         QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("automations"), definitions},
    };

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(QJsonDocument(document).toJson(QJsonDocument::Indented)) < 0
        || !file.commit()) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("error"), tr("Não foi possível exportar o arquivo.")}};
    }
    setAutomationStatus(tr("%1 automações exportadas.").arg(definitions.size()));
    return {{QStringLiteral("accepted"), true},
            {QStringLiteral("count"), definitions.size()},
            {QStringLiteral("path"), path}};
}

QVariantMap ApplicationController::importAutomations(const QUrl &source)
{
    const auto path = source.isLocalFile() ? source.toLocalFile() : source.toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"),
                 QStringList{tr("O arquivo não pôde ser lido ou excede 1 MiB.")}}};
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    const auto root = document.object();
    if (parseError.error != QJsonParseError::NoError || !document.isObject()
        || root.value(QStringLiteral("schemaVersion")).toInt() != 1
        || !root.value(QStringLiteral("automations")).isArray()) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"),
                 QStringList{tr("Documento de automações inválido ou incompatível.")}}};
    }

    const auto array = root.value(QStringLiteral("automations")).toArray();
    if (array.isEmpty() || array.size() > 500) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"),
                 QStringList{tr("O documento deve conter entre 1 e 500 automações.")}}};
    }

    QSet<QString> ids;
    for (const auto &existing : m_automationEngine.automations()) ids.insert(existing.id);
    QList<Automation> parsed;
    QStringList warnings;
    QStringList structuralErrors;
    int position = 0;
    for (const auto &value : array) {
        ++position;
        if (!value.isObject()) {
            structuralErrors.append(tr("Item %1 não é uma definição.").arg(position));
            continue;
        }
        auto automation = automationFromMap(value.toObject().toVariantMap());
        if (automation.id.isEmpty() || ids.contains(automation.id)) {
            automation.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        ids.insert(automation.id);
        automation.consecutiveFailures = 0;

        const auto errors = validateAutomation(automation);
        bool hasMissingReference = false;
        for (const auto &error : errors) {
            if (error.startsWith(tr("Integração não encontrada:"))
                || error.startsWith(tr("O executável "))) {
                hasMissingReference = true;
                warnings.append(QStringLiteral("%1: %2").arg(automation.name, error));
            } else {
                structuralErrors.append(QStringLiteral("%1: %2").arg(automation.name, error));
            }
        }
        if (hasMissingReference) automation.enabled = false;
        parsed.append(automation);
    }
    if (!structuralErrors.isEmpty()) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"), structuralErrors}};
    }

    QStringList insertedIds;
    for (const auto &automation : parsed) {
        if (!m_automationRepository || !m_automationRepository->save(automation)) {
            if (m_automationRepository) {
                for (const auto &id : insertedIds) m_automationRepository->remove(id);
            }
            return {{QStringLiteral("accepted"), false},
                    {QStringLiteral("errors"),
                     QStringList{tr("A importação foi revertida por falha no banco.")}}};
        }
        insertedIds.append(automation.id);
    }

    m_automationEngine.setAutomations(m_automationRepository->automations());
    setAutomationStatus(warnings.isEmpty()
        ? tr("%1 automações importadas.").arg(parsed.size())
        : tr("%1 automações importadas; %2 desativadas por referências ausentes.")
              .arg(parsed.size()).arg(warnings.size()));
    emit automationsChanged();
    return {{QStringLiteral("accepted"), true},
            {QStringLiteral("count"), parsed.size()},
            {QStringLiteral("warnings"), warnings}};
}

bool ApplicationController::removeAutomation(const QString &automationId)
{
    if (!m_automationRepository || !m_automationRepository->remove(automationId)) return false;
    m_automationEngine.setAutomations(m_automationRepository->automations());
    setAutomationStatus(tr("Automação removida."));
    emit automationsChanged();
    emit automationRunsChanged();
    return true;
}

bool ApplicationController::setAutomationEnabled(const QString &automationId, bool enabled)
{
    auto found = m_automationEngine.automation(automationId);
    if (!found.has_value() || !m_automationRepository) return false;
    if (!m_automationRepository->updateRuntimeState(automationId, enabled,
                                                    enabled ? 0 : found->consecutiveFailures)) {
        return false;
    }
    m_automationEngine.setAutomations(m_automationRepository->automations());
    emit automationsChanged();
    return true;
}

bool ApplicationController::resumeAutomation(const QString &automationId)
{
    if (!m_automationEngine.resume(automationId)) return false;
    if (m_automationRepository) {
        m_automationRepository->updateRuntimeState(automationId, true, 0);
    }
    setAutomationStatus(tr("Automação retomada."));
    emit automationsChanged();
    return true;
}

QVariantMap ApplicationController::dryRunAutomation(const QString &automationId,
                                                    const QVariantMap &payload)
{
    const auto run = m_automationEngine.dryRun(automationId, payload);
    if (m_automationRepository) {
        m_automationRepository->recordRun(run);
        emit automationRunsChanged();
    }
    setAutomationStatus(run.reason.isEmpty()
        ? tr("Ensaio concluído com %1 ações previstas.").arg(run.outcomes.size())
        : tr("Ensaio: %1").arg(run.reason));
    return automationRunToMap(run);
}

QVariantMap ApplicationController::authorizeExecutable(const QString &path, const QString &label)
{
    QString error;
    if (!m_authorizedExecutables.authorize(path, label, &error)) {
        return {{QStringLiteral("accepted"), false}, {QStringLiteral("message"), error}};
    }
    const auto entries = m_authorizedExecutables.entries();
    if (m_automationRepository && !entries.isEmpty()) {
        for (const auto &entry : entries) m_automationRepository->authorizeExecutable(entry);
    }
    setAutomationStatus(tr("Executável autorizado."));
    emit authorizedExecutablesChanged();
    return {{QStringLiteral("accepted"), true},
            {QStringLiteral("canonicalPath"), entries.last().canonicalPath}};
}

bool ApplicationController::revokeExecutable(const QString &canonicalPath)
{
    if (!m_authorizedExecutables.revoke(canonicalPath)) return false;
    if (m_automationRepository) m_automationRepository->revokeExecutable(canonicalPath);
    setAutomationStatus(tr("Autorização removida."));
    emit authorizedExecutablesChanged();
    return true;
}

QVariantList ApplicationController::integrations() const
{
    QVariantList result;
    for (const auto &definition : m_integrations.exportableDefinitions()) {
        auto item = integrationDefinitionToMap(definition);
        item.insert(QStringLiteral("typeLabel"), integrationTypeName(definition.type));
        item.insert(QStringLiteral("operations"),
                    integrationOperations(integrationTypeName(definition.type)));
        result.append(item);
    }
    return result;
}

QVariantList ApplicationController::integrationHistory() const
{
    QVariantList result;
    for (const auto &call : m_integrations.history({}, 100)) {
        result.append(integrationCallToMap(call));
    }
    return result;
}

QString ApplicationController::integrationStatus() const { return m_integrationStatus; }

QStringList ApplicationController::integrationTypes() const { return integrationTypeNames(); }

QString ApplicationController::integrationSecretBackend() const
{
    return m_secretStore ? m_secretStore->backendName()
                         : SecretStoreFactory::platformBackendName();
}

bool ApplicationController::integrationSecretsPersistent() const
{
    return m_secretStore && m_secretStore->isPersistent();
}

QStringList ApplicationController::integrationOperations(const QString &type) const
{
    const auto parsed = integrationTypeFromName(type);
    if (!parsed.has_value()) return {};
    switch (*parsed) {
    case IntegrationType::Http: return {QStringLiteral("request.send")};
    case IntegrationType::WebSocket: return WebSocketIntegrationAdapter::supportedOperations();
    case IntegrationType::Obs: return ObsIntegrationAdapter::supportedOperations();
    case IntegrationType::Midi: return MidiIntegrationAdapter::supportedOperations();
    case IntegrationType::Osc: return {QStringLiteral("message.send")};
    }
    return {};
}

QStringList ApplicationController::midiOutputPorts() const
{
    return m_midiTransport.outputPorts();
}

QVariantMap ApplicationController::exportConfiguration(const QUrl &destination) const
{
    QVariantMap profile{
        {QStringLiteral("locale"), m_locale},
        {QStringLiteral("demoMode"), m_demoMode},
        {QStringLiteral("interfaceScale"), m_interfaceScale},
        {QStringLiteral("presentation"), QVariantMap{
             {QStringLiteral("wallpaperColor"), wallpaperColor()},
             {QStringLiteral("wallpaperSource"), wallpaperSource().toString()},
             {QStringLiteral("wallpaperFit"), wallpaperFit()},
             {QStringLiteral("clockVisible"), clockVisible()},
             {QStringLiteral("clockPosition"), clockPosition()},
             {QStringLiteral("clockFormat"), clockFormat()},
             {QStringLiteral("clockFontFamily"), clockFontFamily()},
             {QStringLiteral("clockFontSize"), clockFontSize()},
             {QStringLiteral("clockColor"), clockColor()},
         }},
        {QStringLiteral("media"), QVariantMap{
             {QStringLiteral("volume"), mediaVolume()},
             {QStringLiteral("audioOutputId"), audioOutputId()},
             {QStringLiteral("repeatMode"), mediaRepeatMode()},
             {QStringLiteral("imageFit"), imageFit()},
             {QStringLiteral("imageTransition"), imageTransition()},
             {QStringLiteral("imageAutoplay"), imageAutoplay()},
             {QStringLiteral("imageIntervalMs"), imageIntervalMs()},
         }},
        {QStringLiteral("bible"), QVariantMap{
             {QStringLiteral("primaryTranslationId"), biblePrimaryTranslationId()},
             {QStringLiteral("secondaryTranslationId"), bibleSecondaryTranslationId()},
             {QStringLiteral("tertiaryTranslationId"), bibleTertiaryTranslationId()},
         }},
        {QStringLiteral("remote"), QVariantMap{
             {QStringLiteral("interface"), remoteInterface()},
             {QStringLiteral("port"), remotePort()},
         }},
        {QStringLiteral("library"), QVariantMap{
             {QStringLiteral("mediaFolders"), m_mediaFolderPaths},
             {QStringLiteral("favorites"), m_favoriteMediaPaths},
         }},
        {QStringLiteral("outputs"), m_outputs.serialize()},
        {QStringLiteral("onboarding"), QVariantMap{
             {QStringLiteral("completed"), m_onboardingCompleted},
             {QStringLiteral("skippedSteps"), m_onboardingSkippedSteps},
         }},
        {QStringLiteral("shortcuts"), m_shortcuts},
    };
    QStringList errors;
    const auto document = ConfigurationProfileService::serialize(profile, &errors);
    if (document.isEmpty())
        return {{QStringLiteral("accepted"), false}, {QStringLiteral("errors"), errors}};
    const auto path = destination.toLocalFile();
    if (path.isEmpty()) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"), QStringList{tr("Selecione um arquivo local.")}}};
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(document) != document.size()
        || !file.commit()) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"), QStringList{tr("Não foi possível salvar o perfil.")}}};
    }
    return {{QStringLiteral("accepted"), true}, {QStringLiteral("path"), path}};
}

QVariantMap ApplicationController::importConfiguration(const QUrl &source)
{
    const auto path = source.toLocalFile();
    QFile file(path);
    if (path.isEmpty() || !file.open(QIODevice::ReadOnly)) {
        return {{QStringLiteral("accepted"), false},
                {QStringLiteral("errors"), QStringList{tr("Não foi possível abrir o perfil.")}}};
    }
    const auto parsed = ConfigurationProfileService::parse(
        file.read(ConfigurationProfileService::MaximumDocumentSize + 1));
    if (!parsed.accepted)
        return {{QStringLiteral("accepted"), false}, {QStringLiteral("errors"), parsed.errors}};

    // A partir daqui o documento inteiro já foi validado. Segredos, estado do
    // servidor remoto e mídia nunca são importados.
    const auto &profile = parsed.profile;
    const auto importedShortcuts = profile.value(QStringLiteral("shortcuts")).toMap();
    auto candidateShortcuts = m_shortcuts;
    for (auto it = importedShortcuts.cbegin(); it != importedShortcuts.cend(); ++it)
        candidateShortcuts.insert(it.key(), it.value().toString().trimmed());
    QSet<QString> shortcutSequences;
    for (auto it = candidateShortcuts.cbegin(); it != candidateShortcuts.cend(); ++it) {
        const auto sequence = it.value().toString().trimmed().toCaseFolded();
        if (sequence.isEmpty()) continue;
        if (shortcutSequences.contains(sequence)) {
            return {{QStringLiteral("accepted"), false},
                    {QStringLiteral("errors"), QStringList{
                         tr("O atalho %1 entra em conflito com a configuração atual.")
                             .arg(it.value().toString())}}};
        }
        shortcutSequences.insert(sequence);
    }
    if (profile.contains(QStringLiteral("locale"))) setLocale(profile.value(QStringLiteral("locale")).toString());
    if (profile.contains(QStringLiteral("demoMode"))) setDemoMode(profile.value(QStringLiteral("demoMode")).toBool());
    if (profile.contains(QStringLiteral("interfaceScale"))) setInterfaceScale(profile.value(QStringLiteral("interfaceScale")).toDouble());
    const auto presentation = profile.value(QStringLiteral("presentation")).toMap();
    if (presentation.contains(QStringLiteral("wallpaperColor"))) setWallpaperColor(presentation.value(QStringLiteral("wallpaperColor")).toString());
    if (presentation.contains(QStringLiteral("wallpaperSource"))) setWallpaperSource(QUrl(presentation.value(QStringLiteral("wallpaperSource")).toString()));
    if (presentation.contains(QStringLiteral("wallpaperFit"))) setWallpaperFit(presentation.value(QStringLiteral("wallpaperFit")).toString());
    if (presentation.contains(QStringLiteral("clockVisible"))) setClockVisible(presentation.value(QStringLiteral("clockVisible")).toBool());
    if (presentation.contains(QStringLiteral("clockPosition"))) setClockPosition(presentation.value(QStringLiteral("clockPosition")).toString());
    if (presentation.contains(QStringLiteral("clockFormat"))) setClockFormat(presentation.value(QStringLiteral("clockFormat")).toString());
    if (presentation.contains(QStringLiteral("clockFontFamily"))) setClockFontFamily(presentation.value(QStringLiteral("clockFontFamily")).toString());
    if (presentation.contains(QStringLiteral("clockFontSize"))) setClockFontSize(presentation.value(QStringLiteral("clockFontSize")).toInt());
    if (presentation.contains(QStringLiteral("clockColor"))) setClockColor(presentation.value(QStringLiteral("clockColor")).toString());
    const auto media = profile.value(QStringLiteral("media")).toMap();
    if (media.contains(QStringLiteral("volume"))) setMediaVolume(media.value(QStringLiteral("volume")).toDouble());
    if (media.contains(QStringLiteral("audioOutputId"))) setAudioOutputId(media.value(QStringLiteral("audioOutputId")).toString());
    if (media.contains(QStringLiteral("repeatMode"))) setMediaRepeatMode(media.value(QStringLiteral("repeatMode")).toString());
    if (media.contains(QStringLiteral("imageFit"))) setImageFit(media.value(QStringLiteral("imageFit")).toString());
    if (media.contains(QStringLiteral("imageTransition"))) setImageTransition(media.value(QStringLiteral("imageTransition")).toString());
    if (media.contains(QStringLiteral("imageAutoplay"))) setImageAutoplay(media.value(QStringLiteral("imageAutoplay")).toBool());
    if (media.contains(QStringLiteral("imageIntervalMs"))) setImageIntervalMs(media.value(QStringLiteral("imageIntervalMs")).toInt());
    const auto bible = profile.value(QStringLiteral("bible")).toMap();
    if (bible.contains(QStringLiteral("primaryTranslationId"))) setBiblePrimaryTranslationId(bible.value(QStringLiteral("primaryTranslationId")).toString());
    if (bible.contains(QStringLiteral("secondaryTranslationId"))) setBibleSecondaryTranslationId(bible.value(QStringLiteral("secondaryTranslationId")).toString());
    if (bible.contains(QStringLiteral("tertiaryTranslationId"))) setBibleTertiaryTranslationId(bible.value(QStringLiteral("tertiaryTranslationId")).toString());
    const auto remote = profile.value(QStringLiteral("remote")).toMap();
    if (remote.contains(QStringLiteral("interface"))) setRemoteInterface(remote.value(QStringLiteral("interface")).toString());
    if (remote.contains(QStringLiteral("port"))) setRemotePort(remote.value(QStringLiteral("port")).toInt());
    const auto library = profile.value(QStringLiteral("library")).toMap();
    if (library.contains(QStringLiteral("mediaFolders"))) m_mediaFolderPaths = library.value(QStringLiteral("mediaFolders")).toStringList();
    if (library.contains(QStringLiteral("favorites"))) m_favoriteMediaPaths = library.value(QStringLiteral("favorites")).toStringList();
    if (!library.isEmpty()) {
        saveMediaFolders();
        saveFavoriteMedia();
        refreshMediaCatalog();
    }
    if (profile.contains(QStringLiteral("outputs"))) {
        m_outputs.restore(profile.value(QStringLiteral("outputs")).toStringList());
        saveOutputs();
        refreshScreens();
    }
    const auto onboarding = profile.value(QStringLiteral("onboarding")).toMap();
    if (onboarding.contains(QStringLiteral("completed"))) m_onboardingCompleted = onboarding.value(QStringLiteral("completed")).toBool();
    if (onboarding.contains(QStringLiteral("skippedSteps"))) m_onboardingSkippedSteps = onboarding.value(QStringLiteral("skippedSteps")).toStringList();
    if (!onboarding.isEmpty() && m_settings) {
        m_settings->setValue(QStringLiteral("operator/onboardingCompleted"), m_onboardingCompleted);
        m_settings->setValue(QStringLiteral("operator/onboardingSkippedSteps"), m_onboardingSkippedSteps);
        emit onboardingChanged();
    }
    for (auto it = importedShortcuts.cbegin(); it != importedShortcuts.cend(); ++it)
        setShortcut(it.key(), it.value().toString());
    return {{QStringLiteral("accepted"), true}, {QStringLiteral("path"), path}};
}

QVariantMap ApplicationController::integrationDefinition(const QString &integrationId) const
{
    const auto found = m_integrations.definition(integrationId);
    if (!found.has_value()) return {};
    // A cópia devolvida ao QML nunca leva segredo.
    return integrationDefinitionToMap(IntegrationSanitizer::sanitizedDefinition(*found));
}

QVariantMap ApplicationController::saveIntegration(const QVariantMap &definition)
{
    auto parsed = integrationDefinitionFromMap(definition);
    if (parsed.id.isEmpty()) {
        parsed.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    const auto validation = m_integrations.save(parsed);
    if (validation.valid) {
        setIntegrationStatus(tr("Integração %1 salva.").arg(parsed.name));
        refreshIntegrationDiagnostics();
        emit integrationsChanged();
    }
    return {{QStringLiteral("accepted"), validation.valid},
            {QStringLiteral("errors"), validation.errors},
            {QStringLiteral("id"), parsed.id}};
}

bool ApplicationController::removeIntegration(const QString &integrationId)
{
    const auto found = m_integrations.definition(integrationId);
    if (!m_integrations.remove(integrationId)) return false;
    for (const auto &reference : found ? found->secretReferences : QStringList{}) {
        if (m_secretStore) m_secretStore->remove(reference);
    }
    setIntegrationStatus(tr("Integração removida."));
    refreshIntegrationDiagnostics();
    emit integrationsChanged();
    emit integrationHistoryChanged();
    return true;
}

QString ApplicationController::duplicateIntegration(const QString &integrationId)
{
    const auto found = m_integrations.definition(integrationId);
    if (!found.has_value()) return {};
    auto copy = *found;
    copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    copy.name = tr("%1 (cópia)").arg(found->name);
    copy.enabled = false;
    // Segredos não são copiados: a cópia precisa receber os seus.
    copy.secretReferences.clear();
    for (auto it = copy.configuration.begin(); it != copy.configuration.end(); ++it) {
        if (found->secretReferences.contains(it.value().toString())) it.value() = QString{};
    }
    if (!m_integrations.save(copy).valid) return {};
    setIntegrationStatus(tr("Integração duplicada. Configure os segredos da cópia."));
    emit integrationsChanged();
    return copy.id;
}

bool ApplicationController::setIntegrationEnabled(const QString &integrationId, bool enabled)
{
    auto found = m_integrations.definition(integrationId);
    if (!found.has_value()) return false;
    found->enabled = enabled;
    if (!m_integrations.save(*found).valid) return false;
    emit integrationsChanged();
    return true;
}

QString ApplicationController::setIntegrationSecret(const QString &integrationId,
                                                    const QString &field, const QString &secret)
{
    auto found = m_integrations.definition(integrationId);
    if (!found.has_value() || field.trimmed().isEmpty() || !m_secretStore) return {};
    const auto reference = QStringLiteral("%1/%2").arg(integrationId, field.trimmed());
    if (!m_secretStore->store(reference, secret)) return {};
    if (!found->secretReferences.contains(reference)) found->secretReferences.append(reference);
    // Um campo com ponto aponta para uma chave dentro de um mapa da
    // configuração, como `headers.Authorization`.
    const auto path = field.trimmed().split(QLatin1Char('.'));
    if (path.size() == 2) {
        auto nested = found->configuration.value(path.first()).toMap();
        nested.insert(path.last(), reference);
        found->configuration.insert(path.first(), nested);
    } else {
        found->configuration.insert(field.trimmed(), reference);
    }
    if (!m_integrations.save(*found).valid) return {};
    setIntegrationStatus(m_secretStore->isPersistent()
                             ? tr("Segredo guardado no %1.")
                                   .arg(m_secretStore->backendName())
                             : tr("Sem cofre do sistema: o segredo vale só nesta sessão."));
    emit integrationsChanged();
    return reference;
}

bool ApplicationController::testIntegration(const QString &integrationId)
{
    if (!m_integrationCommands) return false;
    return m_integrationCommands->requestTest(integrationId).accepted;
}

bool ApplicationController::executeIntegration(const QString &integrationId,
                                               const QString &operation,
                                               const QVariantMap &payload)
{
    if (!m_integrationCommands) return false;
    return m_integrationCommands->requestExecute(integrationId, operation, payload).accepted;
}

void ApplicationController::refreshIntegrationDiagnostics()
{
    // O diagnóstico lista nome, tipo e estado; nunca a configuração, que pode
    // conter URLs internas, e nunca as referências de segredo.
    QStringList summary;
    for (const auto &definition : m_integrations.definitions()) {
        summary.append(QStringLiteral("%1 [%2] %3")
                           .arg(definition.name,
                                integrationTypeName(definition.type),
                                definition.enabled ? tr("ativa")
                                                   : tr("inativa")));
    }
    m_diagnostics[QStringLiteral("integrations")] = summary;
    m_diagnostics[QStringLiteral("integrationSecretBackend")] = integrationSecretBackend();
    m_diagnostics[QStringLiteral("integrationSecretsPersistent")] = integrationSecretsPersistent();
    emit diagnosticsChanged();
}

void ApplicationController::setIntegrationStatus(const QString &message)
{
    if (m_integrationStatus == message) return;
    m_integrationStatus = message;
    emit integrationStatusChanged();
}

bool ApplicationController::broadcastTransparencySupported() const
{
    // Wayland, macOS e Windows compõem janelas translúcidas por padrão. No X11
    // a transparência depende de um compositor ativo, que não pode ser
    // detectado com confiança sem APIs privadas.
    const auto platform = QGuiApplication::platformName();
    return platform != QStringLiteral("xcb") && platform != QStringLiteral("offscreen")
        && platform != QStringLiteral("minimal");
}

QString ApplicationController::broadcastTransparencyWarning() const
{
    if (broadcastTransparencySupported()) return {};
    return QStringLiteral(
        "Nesta plataforma a transparência depende de um compositor ativo. "
        "Se a captura mostrar fundo preto, use o modo chroma.");
}

QVariantMap ApplicationController::outputRoutingState(const QString &screenFingerprint) const
{
    const auto found = std::find_if(m_screens.cbegin(), m_screens.cend(),
                                    [&screenFingerprint](const QVariant &entry) {
        return entry.toMap().value(QStringLiteral("fingerprint")).toString()
            == screenFingerprint;
    });
    if (found == m_screens.cend()) return {};
    auto state = found->toMap();
    state.insert(QStringLiteral("enabled"), state.value(QStringLiteral("selected")).toBool());
    return state;
}

bool ApplicationController::setOutputDisplayName(
    const QString &screenFingerprint, const QString &displayName)
{
    if (!m_outputs.setDisplayName(screenFingerprint, displayName)) return false;
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
        setStatusMessage(tr("A pasta selecionada não está disponível."));
        return false;
    }
    if (m_mediaFolderPaths.contains(canonicalPath)) {
        setStatusMessage(tr("Essa pasta já faz parte da biblioteca."));
        return true;
    }
    m_mediaFolderPaths.append(canonicalPath);
    saveMediaFolders();
    emit mediaFoldersChanged();
    refreshMediaCatalog();
    setStatusMessage(tr("Pasta adicionada à biblioteca."));
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
    setStatusMessage(tr("Pasta removida da biblioteca. Os itens já adicionados à playlist foram mantidos."));
}

void ApplicationController::rescanMediaFolders()
{
    refreshMediaCatalog();
    setStatusMessage(tr("Biblioteca de pastas atualizada."));
}

QString ApplicationController::addCatalogFileToPlaylist(const QString &path)
{
    if (!m_mediaRepository) return {};
    const QFileInfo fileInfo(path);
    const auto canonicalPath = fileInfo.canonicalFilePath();
    if (canonicalPath.isEmpty()) {
        setStatusMessage(tr("O arquivo não está mais disponível."));
        return {};
    }

    const auto catalogEntryIt = std::find_if(
        m_mediaCatalogEntries.cbegin(), m_mediaCatalogEntries.cend(), [&](const auto &entry) {
            return entry.path == canonicalPath;
        });
    MediaCatalogEntry catalogEntry;
    if (catalogEntryIt != m_mediaCatalogEntries.cend()) {
        catalogEntry = *catalogEntryIt;
    } else {
        const auto type = MediaFolderScanner::mediaTypeForFile(canonicalPath);
        if (!type.has_value()) {
            setStatusMessage(tr("Esse formato de arquivo não é compatível."));
            return {};
        }
        catalogEntry = MediaCatalogEntry{
            .type = type.value(),
            .fileName = fileInfo.fileName(),
            .title = fileInfo.completeBaseName(),
            .path = canonicalPath,
            .folderPath = fileInfo.absolutePath(),
        };
    }

    if (!QFileInfo::exists(catalogEntry.path)) {
        setStatusMessage(tr("O arquivo não está mais disponível na biblioteca."));
        return {};
    }

    const auto id = m_mediaRepository->add(MediaItem{
        .type = catalogEntry.type,
        .title = catalogEntry.title,
        .path = catalogEntry.path,
        .durationMs = catalogEntry.type == MediaType::Image ? m_images.autoplayIntervalMs() : 0,
    });
    if (id.isEmpty()) {
        setStatusMessage(tr("Não foi possível adicionar o arquivo à playlist."));
        return {};
    }

    if (catalogEntry.type == MediaType::Audio) refreshAudioLibrary();
    else if (catalogEntry.type == MediaType::Video) refreshVideoLibrary();
    else refreshImageLibrary();
    refreshMediaPlaylist();
    setStatusMessage(tr("%1 adicionado à playlist.").arg(catalogEntry.fileName));
    return id;
}

bool ApplicationController::isFavoriteMedia(const QString &path) const
{
    const auto canonicalPath = QFileInfo(path).canonicalFilePath();
    return !canonicalPath.isEmpty() && m_favoriteMediaPaths.contains(canonicalPath);
}

void ApplicationController::toggleFavoriteMedia(const QString &path)
{
    const QFileInfo info(path);
    const auto canonicalPath = info.canonicalFilePath();
    if (canonicalPath.isEmpty() || !info.isFile()) {
        setStatusMessage(tr("Não foi possível localizar esse arquivo."));
        return;
    }

    if (m_favoriteMediaPaths.removeAll(canonicalPath) > 0) {
        setStatusMessage(tr("%1 removido dos favoritos.").arg(info.fileName()));
    } else {
        m_favoriteMediaPaths.append(canonicalPath);
        setStatusMessage(tr("%1 adicionado aos favoritos.").arg(info.fileName()));
    }
    saveFavoriteMedia();
    refreshFavoriteMedia();
    refreshMediaCatalogViews();
}

bool ApplicationController::openFileLocation(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        setStatusMessage(tr("Não foi possível localizar esse arquivo."));
        return false;
    }
    const auto opened = QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
    if (!opened) setStatusMessage(tr("Não foi possível abrir a pasta do arquivo."));
    return opened;
}

void ApplicationController::removeMedia(const QString &id)
{
    m_playlistCommands->requestRemove(id);
}

void ApplicationController::requestMediaThumbnail(const QString &path, const QString &type)
{
    if (!m_mediaThumbnailer) return;
    const auto mediaType = type == QStringLiteral("video") ? MediaType::Video
        : type == QStringLiteral("image") ? MediaType::Image : MediaType::Audio;
    m_mediaThumbnailer->request(path, mediaType);
}

bool ApplicationController::applyRemoveMedia(const QString &id)
{
    if (!m_mediaRepository) return false;
    const auto item = m_mediaRepository->item(id);
    if (item.id.isEmpty()) return false;
    if (item.type == MediaType::Video) removeVideo(id);
    else if (item.type == MediaType::Image) removeImage(id);
    else removeAudio(id);
    return m_mediaRepository->item(id).id.isEmpty();
}

void ApplicationController::moveMedia(const QString &id, int newIndex)
{
    m_playlistCommands->requestMove(id, newIndex);
}

bool ApplicationController::applyMoveMedia(const QString &id, int newIndex)
{
    if (m_mediaRepository && m_mediaRepository->moveInPlaylist(id, newIndex)) {
        refreshMediaPlaylist();
        return true;
    }
    return false;
}

void ApplicationController::shuffleMediaPlaylist()
{
    if (!m_mediaRepository) return;
    auto items = m_mediaRepository->playlistItems();
    if (items.size() < 2) {
        setStatusMessage(tr("Adicione pelo menos dois itens para embaralhar a playlist."));
        return;
    }
    for (int index = items.size() - 1; index > 0; --index) {
        items.swapItemsAt(index, QRandomGenerator::global()->bounded(index + 1));
    }
    for (int index = 0; index < items.size(); ++index) {
        if (!m_mediaRepository->moveInPlaylist(items[index].id, index)) {
            setStatusMessage(tr("Não foi possível embaralhar a playlist."));
            return;
        }
    }
    refreshMediaPlaylist();
    setStatusMessage(tr("Playlist embaralhada."));
}

void ApplicationController::clearMediaPlaylist()
{
    m_playlistCommands->requestClear();
}

bool ApplicationController::applyClearMediaPlaylist()
{
    if (!m_mediaRepository) return false;
    applyStopMedia();
    if (!m_mediaRepository->clearPlaylist()) {
        setStatusMessage(tr("Não foi possível limpar a playlist."));
        return false;
    }
    refreshAudioLibrary();
    refreshVideoLibrary();
    refreshImageLibrary();
    refreshMediaPlaylist();
    setStatusMessage(tr("Playlist limpa."));
    return true;
}

bool ApplicationController::restoreMediaPlaylist(const QVariantList &snapshot)
{
    if (!m_mediaRepository || !m_mediaRepository->clearPlaylist()) return false;
    for (const auto &entry : snapshot) {
        const auto value = entry.toMap();
        const auto typeName = value.value(QStringLiteral("type")).toString();
        const auto type = typeName == QStringLiteral("video") ? MediaType::Video
            : typeName == QStringLiteral("image") ? MediaType::Image : MediaType::Audio;
        const auto restoredId = m_mediaRepository->add(MediaItem{
            .id = value.value(QStringLiteral("id")).toString(),
            .type = type,
            .title = value.value(QStringLiteral("title")).toString(),
            .path = value.value(QStringLiteral("path")).toString(),
            .durationMs = value.value(QStringLiteral("durationMs")).toLongLong(),
            .artist = value.value(QStringLiteral("artist")).toString(),
            .album = value.value(QStringLiteral("album")).toString(),
        });
        if (restoredId.isEmpty()) return false;
    }
    refreshAudioLibrary();
    refreshVideoLibrary();
    refreshImageLibrary();
    refreshMediaPlaylist();
    return true;
}

bool ApplicationController::saveMediaPlaylist(const QUrl &destination)
{
    if (!m_mediaRepository || destination.isEmpty()) return false;
    const auto items = m_mediaRepository->playlistItems();
    if (items.isEmpty()) {
        setStatusMessage(tr("A playlist está vazia."));
        return false;
    }
    auto path = destination.isLocalFile() ? destination.toLocalFile() : destination.toString();
    if (!path.endsWith(QStringLiteral(".m3u8"), Qt::CaseInsensitive)) path += QStringLiteral(".m3u8");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatusMessage(tr("Não foi possível salvar a playlist."));
        return false;
    }
    QByteArray contents("#EXTM3U\n");
    for (const auto &item : items) {
        auto title = item.title;
        title.replace(QLatin1Char('\n'), QLatin1Char(' '));
        title.replace(QLatin1Char('\r'), QLatin1Char(' '));
        contents += QStringLiteral("#EXTINF:%1,%2\n%3\n")
                        .arg(item.durationMs > 0 ? item.durationMs / 1000 : -1)
                        .arg(title, QDir::toNativeSeparators(item.path))
                        .toUtf8();
    }
    if (file.write(contents) != contents.size() || !file.commit()) {
        setStatusMessage(tr("Não foi possível concluir o salvamento da playlist."));
        return false;
    }
    setStatusMessage(tr("Playlist salva em %1.").arg(QDir::toNativeSeparators(path)));
    return true;
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
        setStatusMessage(tr("O arquivo não existe mais: %1").arg(item.title));
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
            setStatusMessage(tr("Não foi possível exibir a imagem selecionada."));
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
    // Depois de Stop, Play representa uma nova execução da playlist. Isso
    // também cobre o estado inicial, quando ainda não há item selecionado.
    if (mediaState() == QStringLiteral("stopped")) {
        if (m_mediaPlaylist.isEmpty()) return false;
        const auto firstId = m_mediaPlaylist.front().toMap()
                                 .value(QStringLiteral("id")).toString();
        return !firstId.isEmpty() && applyPlayMedia(firstId);
    }
    if (currentMediaType() == QStringLiteral("image")) {
        if (m_stillMedia.playing()) m_stillMedia.pause();
        else m_stillMedia.resume();
        return true;
    }
    if (m_video.state() == VideoState::Playing || m_video.state() == VideoState::Buffering) {
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
                         ? tr("%1 áudio(s) importado(s).").arg(imported)
                         : tr("Nenhum áudio novo foi importado."));
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
                         ? tr("%1 vídeo(s) importado(s).").arg(imported)
                         : tr("Nenhum vídeo novo foi importado."));
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
                         ? tr("%1 imagem(ns) importada(s).").arg(imported)
                         : tr("Nenhuma imagem nova foi importada."));
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
        setStatusMessage(tr("O arquivo de imagem não existe mais."));
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
    if (m_autosave && !m_autosave->flush()) return {};
    if (!m_presentationRepository) return {};
    Presentation item{.type = PresentationType::Text, .title = title.trimmed()};
    item.slides.append(Slide{.label = QStringLiteral("1"), .text = QStringLiteral("")});
    const auto id = m_presentationRepository->save(item);
    if (!id.isEmpty()) { refreshTextPresentations(); selectTextPresentation(id); }
    return id;
}

void ApplicationController::deleteTextPresentation(const QString &id)
{
    if (m_autosave && !m_autosave->flush()) return;
    if (!m_presentationRepository || !m_presentationRepository->remove(id)) return;
    if (currentPresentationId() == id) { m_textPresentation.stop(); m_textPresentation.setPresentation({}); }
    refreshTextPresentations(); refreshSongs(); emit currentPresentationChanged(); emit currentSlideChanged();
}

void ApplicationController::selectTextPresentation(const QString &id)
{
    if (m_autosave && !m_autosave->flush()) return;
    if (!m_presentationRepository) return;
    const auto item = m_presentationRepository->presentation(id);
    if (item.id.isEmpty()) return;
    m_textPresentation.stop(); m_textPresentation.setPresentation(item);
    loadActiveTheme();
    emit currentPresentationChanged(); emit textSlidesChanged(); emit currentSlideChanged();
}

void ApplicationController::addTextSlide(const QString &label, const QString &text)
{
    const auto before = m_textPresentation.presentation();
    if (m_textPresentation.addSlide(label, text)) {
        const auto after = m_textPresentation.presentation(); saveCurrentPresentation();
        recordPresentationEdit(tr("Adicionar slide"), before, after);
    }
}
void ApplicationController::updateTextSlide(const QString &id, const QString &label, const QString &text)
{
    const auto before = m_textPresentation.presentation();
    if (m_textPresentation.updateSlide(id, label, text)) {
        const auto after = m_textPresentation.presentation(); saveCurrentPresentation();
        recordPresentationEdit(tr("Editar slide"), before, after);
    }
}
void ApplicationController::duplicateTextSlide(const QString &id)
{
    const auto before = m_textPresentation.presentation();
    if (m_textPresentation.duplicateSlide(id)) {
        const auto after = m_textPresentation.presentation(); saveCurrentPresentation();
        recordPresentationEdit(tr("Duplicar slide"), before, after);
    }
}
void ApplicationController::splitTextSlide(const QString &id, int cursorPosition)
{
    const auto before = m_textPresentation.presentation();
    if (m_textPresentation.splitSlide(id, cursorPosition)) {
        const auto after = m_textPresentation.presentation(); saveCurrentPresentation();
        recordPresentationEdit(tr("Dividir slide"), before, after);
    }
}
void ApplicationController::removeTextSlide(const QString &id)
{
    const auto before = m_textPresentation.presentation();
    if (m_textPresentation.removeSlide(id)) {
        const auto after = m_textPresentation.presentation(); saveCurrentPresentation();
        recordPresentationEdit(tr("Remover slide"), before, after);
    }
}
void ApplicationController::moveTextSlide(const QString &id, int newIndex)
{
    const auto before = m_textPresentation.presentation();
    if (m_textPresentation.moveSlide(id, newIndex)) {
        const auto after = m_textPresentation.presentation(); saveCurrentPresentation();
        recordPresentationEdit(tr("Mover slide"), before, after);
    }
}

bool ApplicationController::applyPresentationSnapshot(const Presentation &presentation)
{
    m_textPresentation.setPresentation(presentation);
    saveCurrentPresentation();
    return true;
}

void ApplicationController::recordPresentationEdit(const QString &label,
                                                   const Presentation &before,
                                                   const Presentation &after)
{
    m_undoManager.record(
        label,
        [this, before] { return applyPresentationSnapshot(before); },
        [this, after] { return applyPresentationSnapshot(after); });
}

void ApplicationController::showTextSlide(int index)
{
    m_presentationCommands->requestShow(index);
}
void ApplicationController::nextTextSlide() { m_presentationCommands->requestNext(); }
void ApplicationController::previousTextSlide() { m_presentationCommands->requestPrevious(); }
void ApplicationController::firstTextSlide() { m_presentationCommands->requestFirst(); }
void ApplicationController::lastTextSlide() { m_presentationCommands->requestLast(); }
void ApplicationController::stopTextPresentation() { m_presentationCommands->requestStop(); }

bool ApplicationController::applyShowTextSlide(int index)
{
    applyStopMedia();
    m_images.stop();
    if (!m_textPresentation.show(index)) return false;
    if (index == 0) {
        const auto &presentation = m_textPresentation.presentation();
        const auto type = presentation.type == PresentationType::Song
            ? QStringLiteral("song")
            : presentation.type == PresentationType::Bible
                ? QStringLiteral("bible") : QStringLiteral("text");
        recordHistory(type, presentation.id, presentation.title);
    }
    return true;
}

QString ApplicationController::createTheme(const QString &name)
{
    if (!m_themeRepository) return {};
    Theme theme; theme.name = name.trimmed(); theme.fontFamily = m_clockFontFamily;
    const auto id = m_themeRepository->save(theme);
    if (!id.isEmpty()) refreshThemes();
    return id;
}

void ApplicationController::updateTheme(const QVariantMap &v)
{
    updateThemeById(m_activeTheme.id, v);
}

bool ApplicationController::updateThemeById(const QString &id, const QVariantMap &v)
{
    if (!m_themeRepository || id.isEmpty()) return false;
    auto t = m_themeRepository->theme(id);
    if (t.id.isEmpty()) return false;
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
    if (m_themeRepository->save(t).isEmpty()) return false;
    if (m_activeTheme.id == t.id) {
        m_activeTheme = t;
        emit activeThemeChanged();
    }
    refreshThemes();
    return true;
}

void ApplicationController::deleteTheme(const QString &id)
{
    if (!m_themeRepository || !m_themeRepository->remove(id)) return;
    if (m_bibleThemeId == id) m_bibleThemeId.clear();
    if (m_lyricsThemeId == id) m_lyricsThemeId.clear();
    const auto remainingThemes = m_themeRepository->themes();
    const auto fallbackThemeId = remainingThemes.isEmpty() ? QString{} : remainingThemes.front().id;
    if (m_bibleThemeId.isEmpty()) m_bibleThemeId = fallbackThemeId;
    if (m_lyricsThemeId.isEmpty()) m_lyricsThemeId = fallbackThemeId;
    if (m_settings) {
        m_settings->setValue(QStringLiteral("presentation/bibleThemeId"), m_bibleThemeId);
        m_settings->setValue(QStringLiteral("presentation/lyricsThemeId"), m_lyricsThemeId);
    }
    emit contentThemeDefaultsChanged();
    if (m_activeTheme.id==id) m_activeTheme={};
    refreshThemes(); loadActiveTheme();
}

void ApplicationController::applyTheme(const QString &id)
{
    m_themeCommands->requestApply(id);
}

bool ApplicationController::applyThemeSelection(const QString &id)
{
    if (!m_themeRepository) return false;
    const auto theme=m_themeRepository->theme(id); if(theme.id.isEmpty())return false;
    m_activeTheme=theme;
    if(!currentPresentationId().isEmpty()){auto p=m_textPresentation.presentation();p.defaultTheme=id;m_textPresentation.setPresentation(p);saveCurrentPresentation();}
    emit activeThemeChanged();
    return true;
}

bool ApplicationController::applyThemeForContent(const QString &scope, const QString &id)
{
    if (!m_themeRepository) return false;
    const auto theme = m_themeRepository->theme(id);
    if (theme.id.isEmpty()) return false;

    const auto normalizedScope = scope.trimmed().toLower();
    if (normalizedScope == QStringLiteral("bible")) {
        m_bibleThemeId = id;
        if (m_settings)
            m_settings->setValue(QStringLiteral("presentation/bibleThemeId"), id);
        if (currentPresentationType() == QStringLiteral("bible")) {
            auto presentation = m_textPresentation.presentation();
            presentation.defaultTheme = id;
            m_textPresentation.setPresentation(presentation);
            m_activeTheme = theme;
            emit currentPresentationChanged();
            emit activeThemeChanged();
        }
    } else if (normalizedScope == QStringLiteral("lyrics")) {
        m_lyricsThemeId = id;
        if (m_settings)
            m_settings->setValue(QStringLiteral("presentation/lyricsThemeId"), id);
        if (currentPresentationType() == QStringLiteral("song")) {
            auto presentation = m_textPresentation.presentation();
            presentation.defaultTheme = id;
            m_textPresentation.setPresentation(presentation);
            saveCurrentPresentation();
            m_activeTheme = theme;
            emit currentPresentationChanged();
            emit activeThemeChanged();
        }
    } else {
        return false;
    }
    emit contentThemeDefaultsChanged();
    return true;
}

QString ApplicationController::createSong(const QString &title, const QString &author,
                                           const QString &structuredLyrics, const QString &sequenceText)
{
    const auto id = saveSongToLibrary(title, author, structuredLyrics, sequenceText);
    if (!id.isEmpty()) selectSong(id);
    return id;
}

QString ApplicationController::saveSongToLibrary(const QString &title, const QString &author,
                                                  const QString &structuredLyrics,
                                                  const QString &sequenceText)
{
    if (m_autosave && !m_autosave->flush()) return {};
    if(!m_presentationRepository)return{};
    Presentation song{.type=PresentationType::Song,.title=title.trimmed(),.author=author.trimmed(),
                      .defaultTheme=m_lyricsThemeId.isEmpty() ? m_activeTheme.id
                                                             : m_lyricsThemeId};
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
    const auto id=m_presentationRepository->save(song);if(!id.isEmpty())refreshSongs();return id;
}

bool ApplicationController::storeVagalumeApiKey(const QString &apiKey)
{
    return m_secretStore && !apiKey.trimmed().isEmpty()
        && m_secretStore->store(QStringLiteral("online-lyrics/vagalume-api-key"), apiKey.trimmed());
}

bool ApplicationController::clearVagalumeApiKey()
{
    return m_secretStore && m_secretStore->remove(
        QStringLiteral("online-lyrics/vagalume-api-key"));
}

QString ApplicationController::vagalumeApiKey() const
{
    if (!m_secretStore) return {};
    const auto stored = m_secretStore->retrieve(
        QStringLiteral("online-lyrics/vagalume-api-key"));
    return stored.has_value() ? *stored : QString{};
}

bool ApplicationController::secretStoragePersistent() const
{
    return m_secretStore && m_secretStore->isPersistent();
}

QString ApplicationController::secretStorageName() const
{
    return m_secretStore ? m_secretStore->backendName()
                         : SecretStoreFactory::platformBackendName();
}

void ApplicationController::selectSong(const QString &id)
{
    if (m_autosave && !m_autosave->flush()) return;
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
{ m_eventCommands->requestSelect(id); }
bool ApplicationController::applyEventSelection(const QString&id)
{if(!m_eventRepository||m_eventRepository->event(id).id.isEmpty())return false;m_currentEventId=id;refreshEventItems();emit currentEventChanged();return true;}
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
{ m_eventCommands->requestExecuteItem(id); }
bool ApplicationController::applyEventItemExecution(const QString&id)
{
    if(!m_eventRepository)return false;for(const auto&item:m_eventRepository->items(m_currentEventId))if(item.id==id){
        switch(item.type){case PlaylistItemType::Song:selectSong(item.referenceId);showTextSlide(0);break;case PlaylistItemType::Text:selectTextPresentation(item.referenceId);showTextSlide(0);break;case PlaylistItemType::Image:showImage(item.referenceId);break;case PlaylistItemType::Video:playVideo(item.referenceId);break;case PlaylistItemType::Audio:playAudio(item.referenceId);break;}return true;
    }return false;
}
void ApplicationController::clearHistory(){if(m_historyRepository&&m_historyRepository->clear())refreshHistory();}
QString ApplicationController::createBackup(){if(!m_recovery||(m_autosave&&!m_autosave->flush()))return{};m_lastBackupPath=m_recovery->createBackup();setStatusMessage(m_lastBackupPath.isEmpty()?tr("Não foi possível criar o backup."):tr("Backup criado em %1").arg(m_lastBackupPath));emit maintenanceChanged();return m_lastBackupPath;}
bool ApplicationController::scheduleRestore(const QUrl&source){if(!m_recovery)return false;const auto path=source.isLocalFile()?source.toLocalFile():source.toString();const bool ok=m_recovery->scheduleRestore(path);setStatusMessage(ok?tr("Restauração agendada. Reinicie o HolyScreen para aplicá-la."):tr("O arquivo não é um backup SQLite válido."));emit maintenanceChanged();return ok;}
bool ApplicationController::exportDiagnostics(const QUrl &destination)
{
    if (destination.isEmpty()) return false;
    auto path = destination.isLocalFile() ? destination.toLocalFile() : destination.toString();
    if (!path.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) path += QStringLiteral(".zip");
    const QVariantMap application{
        {QStringLiteral("version"), QCoreApplication::applicationVersion()},
        {QStringLiteral("qtVersion"), QString::fromLatin1(qVersion())},
        {QStringLiteral("platform"), QSysInfo::prettyProductName()},
        {QStringLiteral("cpu"), QSysInfo::currentCpuArchitecture()},
        {QStringLiteral("schemaVersion"), m_diagnostics.value(QStringLiteral("schemaVersion"))},
        {QStringLiteral("recoveredFromCrash"), recoveredFromCrash()},
    };
    const QVariantMap configuration{
        {QStringLiteral("wallpaperFit"), wallpaperFit()},
        {QStringLiteral("clockVisible"), clockVisible()},
        {QStringLiteral("clockPosition"), clockPosition()},
        {QStringLiteral("mediaRepeatMode"), mediaRepeatMode()},
        {QStringLiteral("simulatedOutputCount"), simulatedOutputCount()},
        {QStringLiteral("debugEnabled"), debugEnabled()},
        {QStringLiteral("debugSimulatedOutputs"), debugSimulatedOutputs()},
        {QStringLiteral("debugDiagnostics"), debugDiagnostics()},
        {QStringLiteral("debugLogging"), debugLogging()},
    };
    QString error;
    const auto exported = DiagnosticExporter::exportZip({
        .destinationPath = path,
        .application = application,
        .screens = screens(),
        .configuration = configuration,
        .logPath = AppLogger::logPath(),
    }, &error);
    setStatusMessage(exported
        ? tr("Diagnóstico exportado para %1.").arg(QDir::toNativeSeparators(path))
        : error);
    return exported;
}
void ApplicationController::runBenchmark(){QElapsedTimer timer;timer.start();volatile quint64 checksum=0;for(int frame=0;frame<100000;++frame)checksum+=qHash(QString::number(frame));const auto elapsed=std::max<qint64>(1,timer.nsecsElapsed());m_diagnostics["benchmarkOperationsPerSecond"]=static_cast<qint64>(100000.0*1e9/elapsed);m_diagnostics["benchmarkChecksum"]=static_cast<qulonglong>(checksum);m_diagnostics["benchmarkAt"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);emit diagnosticsChanged();}
void ApplicationController::checkForUpdates()
{
    m_updateStatus = tr("Verificando...");
    emit updateChanged();
    m_updateChecker.check(UpdateChecker::defaultEndpoint(), QCoreApplication::applicationVersion());
}

void ApplicationController::setAutomaticUpdateChecks(bool enabled)
{
    if (m_automaticUpdateChecks == enabled) return;
    m_automaticUpdateChecks = enabled;
    saveSetting(QStringLiteral("automaticUpdateChecks"), enabled);
    // A verificação periódica só existe enquanto o operador a mantém ligada; o
    // HolyScreen é offline por padrão e não fala com a rede sem autorização.
    if (enabled) {
        m_updateCheckTimer.start();
        checkForUpdates();
    } else {
        m_updateCheckTimer.stop();
    }
    emit updateChanged();
}

void ApplicationController::downloadUpdate()
{
    if (m_updateDownloading) return;
    if (!updateDownloadable()) {
        setStatusMessage(tr("Não há pacote verificável para baixar."));
        return;
    }
    const auto directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const auto destination = directory.isEmpty()
        ? m_dataDirectory + QStringLiteral("/updates") : directory;

    m_updateDownloading = true;
    m_updateDownloadProgress = 0.0;
    m_updateDownloadedPath.clear();
    m_updateDownloadedSha256 = m_updateRelease.sha256;
    m_updateDownloadedSize = m_updateRelease.assetSize;
    emit updateDownloadChanged();
    m_updateStatus = tr("Baixando %1...").arg(m_updateRelease.assetName);
    emit updateChanged();

    m_updateDownloader.start(m_updateRelease.downloadUrl, m_updateRelease.sha256,
                             m_updateRelease.assetSize, destination,
                             m_updateRelease.assetName);
}

void ApplicationController::cancelUpdateDownload()
{
    if (m_updateDownloading) m_updateDownloader.cancel();
}

bool ApplicationController::installDownloadedUpdate()
{
    if (m_updateDownloading) return false;

    QString error;
    if (!m_updateInstaller.install(m_updateDownloadedPath, m_updateDownloadedSha256,
                                   m_updateDownloadedSize, &error)) {
        m_updateStatus = tr("Não foi possível instalar a atualização: %1").arg(error);
        emit updateChanged();
        emit updateDownloadChanged();
        return false;
    }

    m_updateStatus = tr("Instalador iniciado. Encerrando o HolyScreen...");
    emit updateChanged();
    QTimer::singleShot(0, QCoreApplication::instance(), [] {
        // Fechar as janelas dispara o mesmo caminho usado pelo operador, que
        // salva o layout. quit() é a garantia final para janelas de saída.
        for (auto *window : QGuiApplication::allWindows()) {
            if (window) window->close();
        }
        QCoreApplication::quit();
    });
    return true;
}

bool ApplicationController::revealUpdateDownload()
{
    if (m_updateDownloadedPath.isEmpty()) return false;
    return openFileLocation(m_updateDownloadedPath);
}

int ApplicationController::importBibleTranslation(const QUrl &source)
{
    if (!m_bibleRepository) return 0;
    const auto path = source.isLocalFile() ? source.toLocalFile() : source.toString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatusMessage(tr("Não foi possível abrir o arquivo da Bíblia."));
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
        setStatusMessage(tr("Não foi possível salvar a tradução importada."));
        return 0;
    }
    refreshBibleTranslations();
    if (m_biblePrimaryTranslationId.isEmpty()) setBiblePrimaryTranslationId(translationId);
    setStatusMessage(tr("Tradução %1 importada com %2 versículo(s).")
                         .arg(imported.translation.abbreviation)
                         .arg(imported.verses.size()));
    return imported.verses.size();
}

bool ApplicationController::importBibleFolder(const QUrl &folder)
{
    const auto path = folder.isLocalFile() ? folder.toLocalFile() : folder.toString();
    return startBibleImport({.kind = BibleSourceKind::LocalFolder, .location = path}, false);
}

bool ApplicationController::importBibleGit(const QString &url)
{
    return startBibleImport(
        {.kind = BibleSourceKind::GitHttps, .location = url.trimmed()}, false);
}

bool ApplicationController::importBibleZip(const QString &url)
{
    return startBibleImport(
        {.kind = BibleSourceKind::ZipUrl, .location = url.trimmed()}, false);
}

bool ApplicationController::confirmBibleImportLicenses()
{
    if (!m_bibleImportRequiresLicenseConfirmation
        || m_pendingBibleImportSource.location.isEmpty()) return false;
    return startBibleImport(m_pendingBibleImportSource, true);
}

void ApplicationController::cancelBibleImport()
{
    if (!m_bibleImportActive || !m_bibleImportCancelled) return;
    m_bibleImportCancelled->store(true);
    m_bibleImportMessage = tr("Cancelando importação...");
    emit bibleImportStateChanged();
}

bool ApplicationController::updateBibleTranslationFromSource(const QString &translationId)
{
    if (!m_bibleRepository || m_bibleImportActive) return false;
    const auto metadata = m_bibleRepository->translationSource(translationId);
    if (!metadata.has_value() || metadata->location.trimmed().isEmpty()) {
        setStatusMessage(tr("Essa tradução não possui uma origem atualizável registrada."));
        return false;
    }
    return startBibleImport({.kind = metadata->kind,
                             .location = metadata->location,
                             .revision = metadata->revision},
                            false);
}

bool ApplicationController::startBibleImport(
    const BibleSource &source, bool confirmRestrictedLicenses)
{
    if (m_bibleImportActive) {
        setStatusMessage(tr("Já existe uma importação bíblica em andamento."));
        return false;
    }
    if (source.location.trimmed().isEmpty()) {
        setStatusMessage(tr("Informe a pasta ou URL da origem bíblica."));
        return false;
    }
    m_pendingBibleImportSource = source;
    m_bibleImportRequiresLicenseConfirmation = false;
    m_bibleImportRestrictedTranslations.clear();
    m_bibleImportProgressCurrent = 0;
    m_bibleImportProgressTotal = 0;
    m_bibleImportMessage = tr("Preparando importação...");
    m_bibleImportCancelled = std::make_shared<std::atomic_bool>(false);
    m_bibleImportActive = true;
    emit bibleImportStateChanged();

    const auto databasePath = QDir(m_dataDirectory).filePath(QStringLiteral("presenter.db"));
    const auto cancellation = m_bibleImportCancelled;
    const QPointer<ApplicationController> guard(this);
    auto future = QtConcurrent::run(
        [databasePath, source, confirmRestrictedLicenses, cancellation, guard] {
            const auto progressCallback = [guard](const BibleImportProgress &progress) {
                if (!guard) return;
                QMetaObject::invokeMethod(
                    guard.data(),
                    [guard, progress] {
                        if (guard) guard->updateBibleImportProgress(progress);
                    },
                    Qt::QueuedConnection);
            };
            return BibleImportService(databasePath).run(
                source, {.confirmRestrictedLicenses = confirmRestrictedLicenses},
                progressCallback,
                [cancellation] { return cancellation->load(); });
        });
    m_bibleImportWatcher.setFuture(future);
    emit bibleImportStateChanged();
    return true;
}

void ApplicationController::updateBibleImportProgress(const BibleImportProgress &progress)
{
    m_bibleImportProgressCurrent = progress.current;
    m_bibleImportProgressTotal = progress.total;
    m_bibleImportMessage = progress.message;
    emit bibleImportStateChanged();
}

void ApplicationController::finishBibleImport(const BibleImportResult &result)
{
    m_bibleImportActive = false;
    m_bibleImportProgressCurrent = result.success ? 1 : 0;
    m_bibleImportProgressTotal = result.success ? 1 : 0;
    m_bibleImportRequiresLicenseConfirmation = result.requiresLicenseConfirmation;
    m_bibleImportRestrictedTranslations = result.restrictedTranslations;
    if (result.requiresLicenseConfirmation) {
        m_bibleImportMessage = tr("Confirmação de licença necessária.");
        setStatusMessage(QStringLiteral(
            "A origem contém traduções não marcadas como domínio público. Confirme a licença antes de importar."));
    } else if (result.cancelled) {
        m_bibleImportMessage = tr("Importação cancelada.");
        setStatusMessage(m_bibleImportMessage);
        m_pendingBibleImportSource = {};
    } else if (!result.success) {
        m_bibleImportMessage = result.errorSummary().isEmpty()
            ? tr("A importação falhou.") : result.errorSummary();
        setStatusMessage(m_bibleImportMessage);
        m_pendingBibleImportSource = {};
    } else {
        refreshBibleTranslations();
        m_bibleImportMessage = tr("%1 tradução(ões) e %2 versículo(s) importados.")
                                   .arg(result.importedTranslations)
                                   .arg(result.importedVerses);
        if (result.failedTranslations > 0) {
            m_bibleImportMessage += tr(" %1 tradução(ões) ignoradas por erro: %2")
                                        .arg(result.failedTranslations)
                                        .arg(result.errorSummary());
        }
        setStatusMessage(m_bibleImportMessage);
        m_pendingBibleImportSource = {};
    }
    m_bibleImportCancelled.reset();
    emit bibleImportStateChanged();
}

bool ApplicationController::searchBibleReference()
{
    return m_bibleCommands->requestSearch(m_bibleReferenceInput).accepted;
}

bool ApplicationController::applyBibleSearch(const QString &referenceInput)
{
    setBibleReferenceInput(referenceInput);
    m_bibleResults.clear();
    if (!m_bibleRepository) return false;
    const auto reference = m_bibleReferenceParser.parse(m_bibleReferenceInput);
    if (!reference.has_value()) {
        emit bibleResultsChanged();
        setStatusMessage(tr("Referência bíblica inválida. Exemplo: João 3:16-18."));
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
        setStatusMessage(tr("Importe e selecione ao menos uma tradução bíblica."));
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
        const auto referenceKey = QStringLiteral("%1:%2:%3")
            .arg(static_cast<int>(reference->book)).arg(reference->chapter).arg(verseNumber);
        m_bibleResults.append(QVariantMap{
            {QStringLiteral("referenceKey"), referenceKey},
            {QStringLiteral("bookId"), static_cast<int>(reference->book)},
            {QStringLiteral("chapter"), reference->chapter},
            {QStringLiteral("verse"), verseNumber},
            {QStringLiteral("label"), label},
            {QStringLiteral("text"), combinedTexts.join(QStringLiteral("\n\n"))},
            {QStringLiteral("versions"), versions},
        });
    }
    m_currentBibleReference = reference.value();
    emit bibleResultsChanged();
    if (m_bibleResults.isEmpty()) {
        setStatusMessage(tr("Nenhum versículo foi encontrado nas traduções selecionadas."));
        return false;
    }
    setStatusMessage(tr("%1 versículo(s) encontrado(s).").arg(m_bibleResults.size()));
    return true;
}

void ApplicationController::showBibleVerse(int index)
{
    if (index < 0 || index >= m_bibleResults.size()) return;
    Presentation presentation{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = PresentationType::Bible,
        .title = m_bibleReferenceInput.trimmed(),
        .defaultTheme = m_bibleThemeId,
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
    loadActiveTheme();
    emit currentPresentationChanged();
    emit textSlidesChanged();
    showTextSlide(index);
}

void ApplicationController::toggleFavoriteBibleVerse(int index)
{
    if (index < 0 || index >= m_bibleResults.size()) return;
    const auto result = m_bibleResults.at(index).toMap();
    const auto referenceKey = result.value(QStringLiteral("referenceKey")).toString();
    if (referenceKey.isEmpty()) return;

    for (int favoriteIndex = 0; favoriteIndex < m_favoriteBibleVerses.size();
         ++favoriteIndex) {
        const auto favorite = m_favoriteBibleVerses.at(favoriteIndex).toMap();
        if (favorite.value(QStringLiteral("referenceKey")).toString() == referenceKey) {
            m_favoriteBibleVerses.removeAt(favoriteIndex);
            saveFavoriteBibleVerses();
            emit favoriteBibleVersesChanged();
            setStatusMessage(tr("%1 removido dos favoritos.")
                                 .arg(result.value(QStringLiteral("label")).toString()));
            return;
        }
    }

    auto favorite = result;
    favorite.remove(QStringLiteral("versions"));
    m_favoriteBibleVerses.append(favorite);
    saveFavoriteBibleVerses();
    emit favoriteBibleVersesChanged();
    setStatusMessage(tr("%1 adicionado aos favoritos.")
                         .arg(result.value(QStringLiteral("label")).toString()));
}

QVariantList ApplicationController::bibleChapterNumbers(int bookId) const
{
    QVariantList result;
    if (!m_bibleRepository || bookId < static_cast<int>(BibleBook::Genesis)
        || bookId > static_cast<int>(BibleBook::Revelation)) return result;
    for (const auto chapter : m_bibleRepository->chapters(
             m_biblePrimaryTranslationId, static_cast<BibleBook>(bookId))) {
        result.append(chapter);
    }
    return result;
}

QVariantList ApplicationController::bibleVerseNumbers(int bookId, int chapter) const
{
    QVariantList result;
    if (!m_bibleRepository || bookId < static_cast<int>(BibleBook::Genesis)
        || bookId > static_cast<int>(BibleBook::Revelation)) return result;
    for (const auto verse : m_bibleRepository->verseNumbers(
             m_biblePrimaryTranslationId, static_cast<BibleBook>(bookId), chapter)) {
        result.append(verse);
    }
    return result;
}

bool ApplicationController::presentBibleReference(int bookId, int chapter, int verse)
{
    return m_bibleCommands->requestPresent(bookId, chapter, verse).accepted;
}

bool ApplicationController::applyBiblePresentation(int bookId, int chapter, int verse)
{
    if (bookId < static_cast<int>(BibleBook::Genesis)
        || bookId > static_cast<int>(BibleBook::Revelation)
        || chapter <= 0 || verse <= 0) return false;
    const auto reference = QStringLiteral("%1 %2:%3")
                               .arg(bibleBookName(static_cast<BibleBook>(bookId)))
                               .arg(chapter)
                               .arg(verse);
    if (!applyBibleSearch(reference)) return false;
    showBibleVerse(0);
    return true;
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

QVariantList ApplicationController::compareBibleReference(const QString &referenceInput) const
{
    QVariantList comparison;
    if (!m_bibleRepository) return comparison;
    const auto reference = m_bibleReferenceParser.parse(referenceInput.trimmed());
    if (!reference.has_value()) return comparison;

    const auto bookName = bibleBookName(reference->book);
    for (const auto &translation : m_bibleRepository->translations()) {
        QVariantList verses;
        for (const auto &verse : m_bibleRepository->verses(translation.id,
                                                            reference.value())) {
            verses.append(QVariantMap{
                {QStringLiteral("label"), QStringLiteral("%1 %2:%3")
                     .arg(bookName).arg(verse.chapter).arg(verse.verse)},
                {QStringLiteral("bookId"), static_cast<int>(verse.book)},
                {QStringLiteral("chapter"), verse.chapter},
                {QStringLiteral("verse"), verse.verse},
                {QStringLiteral("text"), verse.text},
            });
        }
        if (verses.isEmpty()) continue;
        comparison.append(QVariantMap{
            {QStringLiteral("translationId"), translation.id},
            {QStringLiteral("name"), translation.name},
            {QStringLiteral("abbreviation"), translation.abbreviation},
            {QStringLiteral("language"), translation.language},
            {QStringLiteral("verses"), verses},
        });
    }
    return comparison;
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
            setStatusMessage(tr("Não foi possível abrir o wallpaper selecionado."));
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
    static const QStringList accepted{
        QStringLiteral("topLeft"), QStringLiteral("topCenter"), QStringLiteral("topRight"),
        QStringLiteral("centerLeft"), QStringLiteral("center"), QStringLiteral("centerRight"),
        QStringLiteral("bottomLeft"), QStringLiteral("bottomCenter"), QStringLiteral("bottomRight")};
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

void ApplicationController::setClockFontBold(bool bold)
{
    if (m_clockFontBold == bold) return;
    m_clockFontBold = bold; saveSetting(QStringLiteral("clockFontBold"), bold); emit clockStyleChanged();
}

void ApplicationController::setClockFontItalic(bool italic)
{
    if (m_clockFontItalic == italic) return;
    m_clockFontItalic = italic; saveSetting(QStringLiteral("clockFontItalic"), italic); emit clockStyleChanged();
}

void ApplicationController::setClockBackgroundColor(const QString &color)
{
    if (m_clockBackgroundColor == color) return;
    m_clockBackgroundColor = color; saveSetting(QStringLiteral("clockBackgroundColor"), color); emit clockStyleChanged();
}

void ApplicationController::setClockLineHeight(double height)
{
    height = std::clamp(height, 0.5, 3.0);
    if (qFuzzyCompare(m_clockLineHeight, height)) return;
    m_clockLineHeight = height; saveSetting(QStringLiteral("clockLineHeight"), height); emit clockStyleChanged();
}

void ApplicationController::setClockCornerRadius(int radius)
{
    radius = std::clamp(radius, 0, 200);
    if (m_clockCornerRadius == radius) return;
    m_clockCornerRadius = radius; saveSetting(QStringLiteral("clockCornerRadius"), radius); emit clockStyleChanged();
}

void ApplicationController::setClockTextOpacity(double opacity)
{
    opacity = std::clamp(opacity, 0.0, 1.0);
    if (qFuzzyCompare(m_clockTextOpacity, opacity)) return;
    m_clockTextOpacity = opacity; saveSetting(QStringLiteral("clockTextOpacity"), opacity); emit clockStyleChanged();
}

void ApplicationController::setClockBackgroundOpacity(double opacity)
{
    opacity = std::clamp(opacity, 0.0, 1.0);
    if (qFuzzyCompare(m_clockBackgroundOpacity, opacity)) return;
    m_clockBackgroundOpacity = opacity; saveSetting(QStringLiteral("clockBackgroundOpacity"), opacity); emit clockStyleChanged();
}

void ApplicationController::setClockMarginHorizontal(int margin)
{
    margin = std::clamp(margin, -100, 100);
    if (m_clockMarginHorizontal == margin) return;
    m_clockMarginHorizontal = margin; saveSetting(QStringLiteral("clockMarginHorizontal"), margin); emit clockStyleChanged();
}

void ApplicationController::setClockMarginVertical(int margin)
{
    margin = std::clamp(margin, -100, 100);
    if (m_clockMarginVertical == margin) return;
    m_clockMarginVertical = margin; saveSetting(QStringLiteral("clockMarginVertical"), margin); emit clockStyleChanged();
}

void ApplicationController::setClockEffect(const QString &effect)
{
    static const QStringList accepted{QStringLiteral("none"), QStringLiteral("outline"),
                                      QStringLiteral("raised"), QStringLiteral("sunken")};
    const auto normalized = accepted.contains(effect) ? effect : QStringLiteral("outline");
    if (m_clockEffect == normalized) return;
    m_clockEffect = normalized; saveSetting(QStringLiteral("clockEffect"), normalized); emit clockStyleChanged();
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
    refreshMediaCatalogView(MediaType::Audio);
}

void ApplicationController::setVideoFileSearch(const QString &search)
{
    if (m_videoFileSearch == search) return;
    m_videoFileSearch = search;
    refreshMediaCatalogView(MediaType::Video);
}

void ApplicationController::setImageFileSearch(const QString &search)
{
    if (m_imageFileSearch == search) return;
    m_imageFileSearch = search;
    refreshMediaCatalogView(MediaType::Image);
}

void ApplicationController::setStageMessage(const QString &message)
{
    m_stageCommands->requestMessage(message);
}

bool ApplicationController::applyStageMessage(const QString &message)
{
    const auto normalized = message.trimmed();
    if (m_stageMessage == normalized) return true;
    m_stageMessage = normalized;
    saveSetting(QStringLiteral("stageMessage"), normalized);
    emit stageMessageChanged();
    return true;
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

void ApplicationController::setRemoteEnabled(bool enabled)
{
    if (enabled == m_remoteEnabled && enabled == m_remoteServer.running()) return;
    if (enabled && !remotePasswordConfigured()) {
        setStatusMessage(tr("Defina uma senha antes de habilitar o controle remoto."));
        emit remoteChanged();
        return;
    }
    if (enabled) {
        m_remoteEnabled = restartRemoteServer();
        setStatusMessage(m_remoteEnabled ? tr("Controle remoto habilitado.")
                                         : m_remoteServer.lastError());
    } else {
        m_remoteServer.stop();
        m_remoteEnabled = false;
        setStatusMessage(tr("Controle remoto desabilitado."));
    }
    if (m_settings) m_settings->setValue(QStringLiteral("remote/enabled"), m_remoteEnabled);
    emit remoteChanged();
}

void ApplicationController::setRemotePort(int port)
{
    const auto normalized = std::clamp(port, 1024, 65535);
    if (m_remotePort == normalized) return;
    m_remotePort = normalized;
    if (m_settings) m_settings->setValue(QStringLiteral("remote/port"), normalized);
    if (m_remoteEnabled && !restartRemoteServer()) {
        m_remoteEnabled = false;
        if (m_settings) m_settings->setValue(QStringLiteral("remote/enabled"), false);
    }
    emit remoteChanged();
}

void ApplicationController::setRemoteInterface(const QString &interfaceAddress)
{
    const QHostAddress address(interfaceAddress.trimmed());
    if (address.isNull() || address.protocol() != QAbstractSocket::IPv4Protocol) {
        setStatusMessage(tr("Informe um endereço IPv4 válido para o remoto."));
        return;
    }
    const auto normalized = address.toString();
    if (m_remoteInterface == normalized) return;
    m_remoteInterface = normalized;
    if (m_settings) m_settings->setValue(QStringLiteral("remote/interface"), normalized);
    if (m_remoteEnabled && !restartRemoteServer()) {
        m_remoteEnabled = false;
        if (m_settings) m_settings->setValue(QStringLiteral("remote/enabled"), false);
    }
    emit remoteChanged();
}

bool ApplicationController::setRemotePassword(const QString &password)
{
    if (password.size() < 8) {
        setStatusMessage(tr("A senha do remoto deve ter pelo menos 8 caracteres."));
        return false;
    }
    const auto credentials = m_remoteServer.setPassword(password);
    if (credentials.isEmpty() || !m_settings
        || !m_settings->setValue(QStringLiteral("remote/credentials"), credentials)) {
        setStatusMessage(tr("Não foi possível salvar a senha do remoto."));
        return false;
    }
    setStatusMessage(tr("Senha do controle remoto atualizada. Sessões antigas foram revogadas."));
    emit remoteChanged();
    return true;
}

void ApplicationController::revokeRemoteSessions()
{
    m_remoteServer.revokeAllSessions();
    setStatusMessage(tr("Todas as sessões remotas foram revogadas."));
    emit remoteChanged();
}

bool ApplicationController::restartRemoteServer()
{
    m_remoteServer.stop();
    return m_remoteServer.start(static_cast<quint16>(m_remotePort),
                                QHostAddress(m_remoteInterface));
}

QJsonObject ApplicationController::remoteState() const
{
    return {
        {QStringLiteral("stateRevision"), static_cast<qint64>(m_commandBus.stateRevision())},
        {QStringLiteral("presentation"), QJsonObject{
            {QStringLiteral("id"), currentPresentationId()},
            {QStringLiteral("title"), currentPresentationTitle()},
            {QStringLiteral("slideIndex"), currentSlideIndex()},
            {QStringLiteral("slideId"), currentSlideId()},
            {QStringLiteral("slideText"), currentSlideText()},
            {QStringLiteral("nextSlideText"), nextSlideText()},
            {QStringLiteral("visible"), textVisible()},
        }},
        {QStringLiteral("media"), QJsonObject{
            {QStringLiteral("id"), currentMediaId()},
            {QStringLiteral("title"), currentMediaTitle()},
            {QStringLiteral("type"), currentMediaType()},
            {QStringLiteral("state"), mediaState()},
            {QStringLiteral("positionMs"), mediaPositionMs()},
            {QStringLiteral("durationMs"), mediaDurationMs()},
            {QStringLiteral("repeatMode"), mediaRepeatMode()},
            {QStringLiteral("playlist"), QJsonArray::fromVariantList(mediaPlaylist())},
        }},
        {QStringLiteral("bible"), QJsonObject{
            {QStringLiteral("reference"), bibleReferenceInput()},
            {QStringLiteral("results"), QJsonArray::fromVariantList(bibleResults())},
            {QStringLiteral("translations"), QJsonArray::fromVariantList(bibleTranslations())},
        }},
        {QStringLiteral("event"), QJsonObject{
            {QStringLiteral("id"), currentEventId()},
            {QStringLiteral("events"), QJsonArray::fromVariantList(events())},
            {QStringLiteral("items"), QJsonArray::fromVariantList(eventItems())},
        }},
        {QStringLiteral("stage"), QJsonObject{{QStringLiteral("message"), stageMessage()}}},
        {QStringLiteral("overlays"), QJsonObject{
            {QStringLiteral("audienceMessage"), audienceMessage()},
            {QStringLiteral("alertMessage"), alertMessage()},
            {QStringLiteral("lowerThirdTitle"), lowerThirdTitle()},
            {QStringLiteral("lowerThirdSubtitle"), lowerThirdSubtitle()},
        }},
        {QStringLiteral("timers"), QJsonObject{
            {QStringLiteral("countdownText"), countdownText()},
            {QStringLiteral("countdownRunning"), countdownRunning()},
            {QStringLiteral("stopwatchText"), stopwatchText()},
            {QStringLiteral("stopwatchRunning"), stopwatchRunning()},
        }},
        {QStringLiteral("blackout"), blackout()},
        {QStringLiteral("outputs"), QJsonArray::fromVariantList(screens())},
    };
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
    m_mediaCommands->requestRepeat(mode);
}

void ApplicationController::setAudioOutputId(const QString &id)
{
    const auto normalized = id.trimmed();
    if (normalized.isEmpty()) {
        if (m_audioOutputId.isEmpty()) return;
        m_audioOutputId.clear();
        m_video.setAudioDevice(QMediaDevices::defaultAudioOutput());
        saveSetting(QStringLiteral("audioOutputId"), m_audioOutputId);
        emit audioOutputsChanged();
        return;
    }
    const auto devices = QMediaDevices::audioOutputs();
    const auto found = std::ranges::find_if(devices, [&normalized](const QAudioDevice &device) {
        return QString::fromLatin1(device.id().toHex()) == normalized;
    });
    if (found == devices.cend()) {
        setStatusMessage(tr("A saída de áudio selecionada não está disponível."));
        return;
    }
    if (m_audioOutputId == normalized && m_video.audioDevice() == *found) return;
    m_audioOutputId = normalized;
    m_video.setAudioDevice(*found);
    saveSetting(QStringLiteral("audioOutputId"), m_audioOutputId);
    emit audioOutputsChanged();
    setStatusMessage(tr("Saída de áudio alterada para %1.").arg(found->description()));
}

void ApplicationController::applyAudioOutputSelection()
{
    const auto devices = QMediaDevices::audioOutputs();
    const auto found = std::ranges::find_if(devices, [this](const QAudioDevice &device) {
        return QString::fromLatin1(device.id().toHex()) == m_audioOutputId;
    });
    m_video.setAudioDevice(found != devices.cend() ? *found
                                                   : QMediaDevices::defaultAudioOutput());
}

void ApplicationController::applyMediaRepeatMode(const QString &mode)
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
void ApplicationController::setUpdateEndpoint(const QString&endpoint){Q_UNUSED(endpoint);const auto value=UpdateChecker::defaultEndpoint().toString();if(m_updateEndpoint==value)return;m_updateEndpoint=value;saveSetting(QStringLiteral("updateEndpoint"),value);emit updateChanged();}

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
    m_screens = m_outputs.describeScreens(descriptors);
    emit screensChanged();
    emit outputWindowsChanged();
    m_diagnostics[QStringLiteral("detectedScreens")]=descriptors.size();
    m_diagnostics[QStringLiteral("activeOutputs")]=m_outputs.activeOutputs().size();
    refreshIntegrationDiagnostics();
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
    m_mediaThumbnailer = std::make_unique<MediaThumbnailer>(
        QDir(appData).filePath(QStringLiteral("thumbnails")), this);
    connect(m_mediaThumbnailer.get(), &MediaThumbnailer::thumbnailReady,
            this, [this](const QString &path, const QUrl &source) {
        m_thumbnailSourceCache.insert(path, source);
        m_thumbnailRefreshDebounce.start();
    });
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
        setStatusMessage(tr("Não foi possível atualizar o banco local: %1")
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
    m_clockFontBold = m_settings->value(QStringLiteral("presentation/clockFontBold"), m_clockFontBold).toBool();
    m_clockFontItalic = m_settings->value(QStringLiteral("presentation/clockFontItalic"), m_clockFontItalic).toBool();
    m_clockBackgroundColor = m_settings->value(QStringLiteral("presentation/clockBackgroundColor"), m_clockBackgroundColor).toString();
    m_clockLineHeight = m_settings->value(QStringLiteral("presentation/clockLineHeight"), m_clockLineHeight).toDouble();
    m_clockCornerRadius = m_settings->value(QStringLiteral("presentation/clockCornerRadius"), m_clockCornerRadius).toInt();
    m_clockTextOpacity = m_settings->value(QStringLiteral("presentation/clockTextOpacity"), m_clockTextOpacity).toDouble();
    m_clockBackgroundOpacity = m_settings->value(QStringLiteral("presentation/clockBackgroundOpacity"), m_clockBackgroundOpacity).toDouble();
    m_clockMarginHorizontal = m_settings->value(QStringLiteral("presentation/clockMarginHorizontal"), m_clockMarginHorizontal).toInt();
    m_clockMarginVertical = m_settings->value(QStringLiteral("presentation/clockMarginVertical"), m_clockMarginVertical).toInt();
    m_clockEffect = m_settings->value(QStringLiteral("presentation/clockEffect"), m_clockEffect).toString();
    m_stageMessage = m_settings->value(QStringLiteral("presentation/stageMessage")).toString();
    m_simulatedOutputCount = m_settings->value(QStringLiteral("developer/simulatedOutputCount"), m_simulatedOutputCount).toInt();
    m_debugEnabled = m_settings->value(QStringLiteral("developer/debugEnabled"), false).toBool();
    m_debugSimulatedOutputs = m_settings->value(QStringLiteral("developer/debugSimulatedOutputs"), true).toBool();
    m_debugDiagnostics = m_settings->value(QStringLiteral("developer/debugDiagnostics"), true).toBool();
    m_debugLogging = m_settings->value(QStringLiteral("developer/debugLogging"), false).toBool();
    m_locale = m_settings->value(QStringLiteral("operator/locale"),
                                 QStringLiteral("pt-BR")).toString();
    if (m_locale != QStringLiteral("pt-BR") && m_locale != QStringLiteral("en-US"))
        m_locale = QStringLiteral("pt-BR");
    m_demoMode = m_settings->value(QStringLiteral("operator/demoMode"), false).toBool();
    m_interfaceScale = m_settings->value(QStringLiteral("operator/interfaceScale"), 1.0).toDouble();
    if (m_interfaceScale != 1.0 && m_interfaceScale != 1.5 && m_interfaceScale != 2.0)
        m_interfaceScale = 1.0;
    m_onboardingCompleted = m_settings->value(
        QStringLiteral("operator/onboardingCompleted"), false).toBool();
    m_onboardingSkippedSteps = m_settings->value(
        QStringLiteral("operator/onboardingSkippedSteps"), QStringList{}).toStringList();
    const auto storedShortcuts = m_settings->value(
        QStringLiteral("operator/shortcuts"), QVariantMap{}).toMap();
    for (auto it = storedShortcuts.cbegin(); it != storedShortcuts.cend(); ++it) {
        if (m_shortcuts.contains(it.key())) m_shortcuts.insert(it.key(), it.value().toString());
    }
    m_remoteEnabled = m_settings->value(QStringLiteral("remote/enabled"), false).toBool();
    m_remotePort = std::clamp(m_settings->value(QStringLiteral("remote/port"), 43120).toInt(),
                              1024, 65535);
    m_remoteInterface = m_settings->value(QStringLiteral("remote/interface"),
                                          QStringLiteral("0.0.0.0")).toString();
    const auto credentials = m_settings->value(QStringLiteral("remote/credentials")).toMap();
    if (!credentials.isEmpty() && !m_remoteServer.loadCredentials(credentials)) {
        qWarning() << "remote_credentials_invalid";
        m_remoteEnabled = false;
    }
    m_mediaFolderPaths = m_settings->value(QStringLiteral("library/mediaFolders"), QStringList{}).toStringList();
    m_favoriteMediaPaths = m_settings->value(QStringLiteral("library/favoriteMedia"), QStringList{}).toStringList();
    const auto favoriteBibleDocument = QJsonDocument::fromJson(
        m_settings->value(QStringLiteral("bible/favoriteVerses")).toByteArray());
    if (favoriteBibleDocument.isArray())
        m_favoriteBibleVerses = favoriteBibleDocument.array().toVariantList();
    m_biblePrimaryTranslationId = m_settings->value(QStringLiteral("bible/primaryTranslationId")).toString();
    m_bibleSecondaryTranslationId = m_settings->value(QStringLiteral("bible/secondaryTranslationId")).toString();
    m_bibleTertiaryTranslationId = m_settings->value(QStringLiteral("bible/tertiaryTranslationId")).toString();
    m_bibleThemeId = m_settings->value(QStringLiteral("presentation/bibleThemeId")).toString();
    m_lyricsThemeId = m_settings->value(QStringLiteral("presentation/lyricsThemeId")).toString();
    AppLogger::setDebugMessagesEnabled(m_debugEnabled && m_debugLogging);
    const auto legacyVolume = m_settings->value(QStringLiteral("presentation/videoVolume"),
        m_settings->value(QStringLiteral("presentation/audioVolume"), 0.8)).toDouble();
    m_video.setVolume(m_settings->value(QStringLiteral("presentation/mediaVolume"), legacyVolume).toDouble());
    m_audioOutputId = m_settings->value(
        QStringLiteral("presentation/audioOutputId"), QString{}).toString();
    applyAudioOutputSelection();
    m_mediaRepeatMode = m_settings->value(QStringLiteral("presentation/mediaRepeatMode"),
        m_settings->value(QStringLiteral("presentation/videoLoop"), false).toBool()
            ? QStringLiteral("one") : QStringLiteral("off")).toString();
    m_video.setLoop(m_mediaRepeatMode == QStringLiteral("one"));
    m_updateEndpoint = UpdateChecker::defaultEndpoint().toString();
    m_automaticUpdateChecks = m_settings->value(
        QStringLiteral("presentation/automaticUpdateChecks"), false).toBool();
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
    restoreMediaCatalogCache();
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
        Theme standard; standard.name=tr("Padrão"); standard.fontFamily=m_clockFontFamily;
        m_themeRepository->save(standard);
    }
    const auto availableThemes = m_themeRepository->themes();
    const auto themeExists = [&availableThemes](const QString &id) {
        return std::ranges::any_of(availableThemes, [&id](const Theme &theme) {
            return theme.id == id;
        });
    };
    const auto defaultThemeId = availableThemes.isEmpty() ? QString{} : availableThemes.front().id;
    if (!themeExists(m_bibleThemeId)) m_bibleThemeId = defaultThemeId;
    if (!themeExists(m_lyricsThemeId)) m_lyricsThemeId = defaultThemeId;
    if (m_settings) {
        m_settings->setValue(QStringLiteral("presentation/bibleThemeId"), m_bibleThemeId);
        m_settings->setValue(QStringLiteral("presentation/lyricsThemeId"), m_lyricsThemeId);
    }
    refreshThemes(); loadActiveTheme();
    refreshSongs();
    m_eventRepository=std::make_unique<EventRepository>(databasePath);
    if(!m_eventRepository->open())qWarning()<<"Failed to open event database";
    refreshEvents();
    m_historyRepository=std::make_unique<HistoryRepository>(databasePath);
    if(!m_historyRepository->open())qWarning()<<"Failed to open history database";
    refreshHistory();
    setupIntegrations(databasePath);
    setupAutomations(databasePath);
    m_broadcastProfiles=std::make_unique<BroadcastProfileRepository>(databasePath);
    if(!m_broadcastProfiles->open()) {
        qWarning()<<"Failed to open broadcast profile database";
    } else {
        m_outputs.setBroadcastProfiles(m_broadcastProfiles->all());
    }
    m_diagnostics={{"version",QCoreApplication::applicationVersion()},{"qtVersion",QString::fromLatin1(qVersion())},{"platform",QSysInfo::prettyProductName()},{"cpu",QSysInfo::currentCpuArchitecture()},{"dataDirectory",m_dataDirectory},{"database",databasePath},{"schemaVersion",migration.currentVersion},{"migrationBackup",migration.backupPath},{"recoveredFromCrash",recoveredFromCrash()}};

    m_outputs.restore(m_settings->value(QStringLiteral("outputs/items")).toStringList());

}

QUrl ApplicationController::thumbnailSourceFor(const QString &path, MediaType type)
{
    if (type == MediaType::Image) return QUrl::fromLocalFile(path);
    return m_thumbnailSourceCache.value(path);
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
    if (!m_settings) return;
    m_settings->setValue(QStringLiteral("outputs/items"), m_outputs.serialize());
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

void ApplicationController::saveMediaCatalogCache()
{
    if (!m_settings) return;
    const MediaCatalogSnapshot snapshot{
        .entries = m_mediaCatalogEntries,
        .directories = m_mediaCatalogDirectories,
    };
    m_settings->setValue(QStringLiteral("library/mediaCatalogCache"),
                         MediaFolderScanner::encodeCache(snapshot, m_mediaFolderPaths));
}

bool ApplicationController::restoreMediaCatalogCache()
{
    if (!m_settings) return false;
    auto snapshot = MediaFolderScanner::decodeCache(
        m_settings->value(QStringLiteral("library/mediaCatalogCache")).toByteArray(),
        m_mediaFolderPaths);
    if (!snapshot.has_value()) return false;
    m_mediaCatalogEntries = std::move(snapshot->entries);
    m_mediaCatalogDirectories = std::move(snapshot->directories);
    return true;
}

void ApplicationController::saveFavoriteMedia()
{
    if (m_settings) {
        m_settings->setValue(QStringLiteral("library/favoriteMedia"), m_favoriteMediaPaths);
    }
}

void ApplicationController::saveFavoriteBibleVerses()
{
    if (!m_settings) return;
    m_settings->setValue(
        QStringLiteral("bible/favoriteVerses"),
        QJsonDocument::fromVariant(m_favoriteBibleVerses).toJson(QJsonDocument::Compact));
}

void ApplicationController::rebuildMediaFolderWatcher()
{
    const auto watched = m_mediaFolderWatcher.directories();
    if (!watched.isEmpty()) m_mediaFolderWatcher.removePaths(watched);
    if (!m_mediaCatalogDirectories.isEmpty())
        m_mediaFolderWatcher.addPaths(m_mediaCatalogDirectories);
}

void ApplicationController::refreshMediaCatalog()
{
    if (m_mediaCatalogScanWatcher.isRunning()) {
        m_mediaCatalogRescanPending = true;
        return;
    }
    m_mediaCatalogScanFolders = m_mediaFolderPaths;
    const auto folders = m_mediaCatalogScanFolders;
    m_mediaCatalogScanWatcher.setFuture(QtConcurrent::run([folders] {
        return MediaFolderScanner{}.scanSnapshot(folders);
    }));
}

QVariantList ApplicationController::mediaCatalogView(MediaType type, const QString &search)
{
    QSet<QString> playlistPaths;
    if (m_mediaRepository) {
        for (const auto &item : m_mediaRepository->playlistItems()) playlistPaths.insert(item.path);
    }

    QVariantList result;
    for (const auto &entry : MediaFolderScanner::filter(m_mediaCatalogEntries, type, search)) {
        result.append(mapCatalogEntry(entry, playlistPaths.contains(entry.path),
                                      m_favoriteMediaPaths.contains(entry.path),
                                      thumbnailSourceFor(entry.path, entry.type)));
    }
    return result;
}

void ApplicationController::refreshMediaCatalogViews()
{
    m_folderAudioFiles = mediaCatalogView(MediaType::Audio, m_audioFileSearch);
    m_folderVideoFiles = mediaCatalogView(MediaType::Video, m_videoFileSearch);
    m_folderImageFiles = mediaCatalogView(MediaType::Image, m_imageFileSearch);
    emit mediaCatalogChanged();
}

void ApplicationController::refreshMediaCatalogView(MediaType type)
{
    if (type == MediaType::Audio)
        m_folderAudioFiles = mediaCatalogView(type, m_audioFileSearch);
    else if (type == MediaType::Video)
        m_folderVideoFiles = mediaCatalogView(type, m_videoFileSearch);
    else
        m_folderImageFiles = mediaCatalogView(type, m_imageFileSearch);
    emit mediaCatalogChanged();
}

void ApplicationController::refreshFavoriteMedia()
{
    QVariantList updated;
    QHash<QString, MediaItem> playlistItems;
    if (m_mediaRepository) {
        for (const auto &item : m_mediaRepository->playlistItems()) {
            playlistItems.insert(item.path, item);
        }
    }

    const auto favoritePaths = m_favoriteMediaPaths;
    for (const auto &path : favoritePaths) {
        const QFileInfo info(path);
        if (!info.isFile()) continue;

        if (playlistItems.contains(path)) {
            const auto item = playlistItems.value(path);
            auto mapped = mapMedia(item, thumbnailSourceFor(item.path, item.type));
            mapped.insert(QStringLiteral("fileName"), info.fileName());
            mapped.insert(QStringLiteral("favorite"), true);
            mapped.insert(QStringLiteral("inPlaylist"), true);
            updated.append(mapped);
            continue;
        }

        const auto catalogEntry = std::find_if(
            m_mediaCatalogEntries.cbegin(), m_mediaCatalogEntries.cend(),
            [&](const auto &entry) { return entry.path == path; });
        if (catalogEntry != m_mediaCatalogEntries.cend()) {
            updated.append(mapCatalogEntry(*catalogEntry, false, true,
                                           thumbnailSourceFor(catalogEntry->path,
                                                              catalogEntry->type)));
            continue;
        }

        const auto type = MediaFolderScanner::mediaTypeForFile(path);
        if (!type.has_value()) continue;
        const MediaCatalogEntry entry{
            .type = type.value(),
            .fileName = info.fileName(),
            .title = info.completeBaseName(),
            .path = path,
            .folderPath = info.absolutePath(),
        };
        updated.append(mapCatalogEntry(entry, false, true,
                                       thumbnailSourceFor(entry.path, entry.type)));
    }
    m_favoriteMedia = updated;
    emit favoriteMediaChanged();
}

void ApplicationController::refreshMediaPlaylist()
{
    QVariantList updated;
    if (m_mediaRepository) {
        for (const auto &item : m_mediaRepository->playlistItems()) {
            updated.append(mapMedia(item, thumbnailSourceFor(item.path, item.type)));
        }
    }
    m_mediaPlaylist = updated;
    emit mediaPlaylistChanged();
    emit currentMediaChanged();
    refreshMediaCatalogViews();
    refreshFavoriteMedia();
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
            updated.append(mapMedia(item, thumbnailSourceFor(item.path, item.type)));
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
        for (const auto &item : m_mediaRepository->items(MediaType::Video)) {
            updated.append(mapMedia(item, thumbnailSourceFor(item.path, item.type)));
        }
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
        for (const auto &item : playlist) {
            updated.append(mapMedia(item, thumbnailSourceFor(item.path, item.type)));
        }
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
            auto item = mapBibleTranslation(translation);
            const auto source = m_bibleRepository->translationSource(translation.id);
            if (source.has_value()) {
                item.insert(QStringLiteral("sourceKind"), bibleSourceKindName(source->kind));
                item.insert(QStringLiteral("sourceLocation"), source->location);
                item.insert(QStringLiteral("sourceRevision"), source->revision);
                item.insert(QStringLiteral("license"), source->license);
                item.insert(QStringLiteral("publisher"), source->publisher);
                item.insert(QStringLiteral("importedAt"), source->importedAt);
                item.insert(QStringLiteral("canUpdate"), true);
            } else {
                item.insert(QStringLiteral("canUpdate"), false);
            }
            updated.append(item);
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
    if (!m_presentationRepository || currentPresentationId().isEmpty() || !m_autosave) return;
    m_autosave->markDirty();
}

bool ApplicationController::persistCurrentPresentation()
{
    if (!m_presentationRepository || currentPresentationId().isEmpty()) return true;
    if (!m_presentationRepository->save(m_textPresentation.presentation()).isEmpty()) {
        refreshTextPresentations(); emit textSlidesChanged(); emit currentSlideChanged();
        return true;
    }
    return false;
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
        auto id=m_textPresentation.presentation().defaultTheme;
        if (id.isEmpty() && m_textPresentation.presentation().type == PresentationType::Bible)
            id = m_bibleThemeId;
        else if (id.isEmpty()
                 && m_textPresentation.presentation().type == PresentationType::Song)
            id = m_lyricsThemeId;
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
