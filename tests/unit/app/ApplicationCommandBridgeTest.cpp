#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/ApplicationController.h"
#include "library/PresentationRepository.h"
#include "core/CommandCatalog.h"

#include <QGuiApplication>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <algorithm>

#include <QHttpServer>
#include <QTemporaryDir>
#include <QTcpServer>

using namespace churchpresenter;

namespace {

struct HttpResult {
    int status = 0;
    QJsonObject body;
};

HttpResult postJson(QNetworkAccessManager &network, const QUrl &url,
                    const QByteArray &body, const QString &token = {})
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!token.isEmpty())
        request.setRawHeader(QByteArrayLiteral("Authorization"),
                             QByteArrayLiteral("Bearer ") + token.toUtf8());
    auto *reply = network.post(request, body);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const HttpResult result{
        .status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
        .body = QJsonDocument::fromJson(reply->readAll()).object(),
    };
    reply->deleteLater();
    return result;
}

} // namespace

class ApplicationCommandBridgeTest final : public QObject {
    Q_OBJECT

private slots:
    void operatorBlackoutUsesCommandAndEventBuses();
    void registersEveryCatalogCommand();
    void operatorOverlayUsesCommandAndEventBuses();
    void operatorMediaUsesCommandAndEventBuses();
    void stopThenPlayRestartsPlaylistFromBeginning();
    void operatorPresentationNavigationUsesCommandAndEventBuses();
    void undoAndRedoUseCommandBus();
    void autosaveDebouncesPresentationEdits();
    void presentationEditsSupportUndoAndRedo();
    void canonicalBibleImportRequiresLicenseThenCompletesAsynchronously();
    void remoteServerRequiresPasswordAndPersistsLocalConfiguration();
    void broadcastProfilesAreValidatedAndSurviveARestart();
    void integrationsAreValidatedExecutedAndRecordedWithoutSecrets();
    void automationsReactToDomainEventsWithoutLooping();
    void acceptedRemoteAndTimerCommandsBecomeAutomationFacts();
    void automationDefinitionsRoundTripWithoutOverwritingOrSecrets();
    void facadesExposeBoundedQmlContracts();
    void onboardingStepsCanBeDeferredAndRestored();
    void interfaceScaleIsValidatedAndPersisted();
    void keyboardShortcutsRejectConflictsAndPersist();
};

void ApplicationCommandBridgeTest::keyboardShortcutsRejectConflictsAndPersist()
{
    ApplicationController controller;
    const auto originalNext = controller.shortcuts().value(QStringLiteral("next")).toString();
    const auto originalPrevious = controller.shortcuts().value(QStringLiteral("previous")).toString();
    const auto customSequence = QStringLiteral("Ctrl+Alt+Shift+F12");

    QVERIFY(controller.setShortcut(QStringLiteral("next"), customSequence));
    QCOMPARE(controller.shortcuts().value(QStringLiteral("next")).toString(), customSequence);
    QVERIFY(!controller.setShortcut(QStringLiteral("previous"), customSequence));
    QCOMPARE(controller.shortcuts().value(QStringLiteral("previous")).toString(), originalPrevious);
    QVERIFY(!controller.setShortcut(QStringLiteral("unknown"), QStringLiteral("F11")));

    {
        ApplicationController restored;
        QCOMPARE(restored.shortcuts().value(QStringLiteral("next")).toString(), customSequence);
    }

    QVERIFY(controller.setShortcut(QStringLiteral("next"), originalNext));
}

void ApplicationCommandBridgeTest::interfaceScaleIsValidatedAndPersisted()
{
    {
        ApplicationController controller;
        controller.setInterfaceScale(1.0);
        QSignalSpy preferencesSpy(&controller, &ApplicationController::preferencesChanged);
        controller.setInterfaceScale(1.5);
        QCOMPARE(controller.interfaceScale(), 1.5);
        QCOMPARE(preferencesSpy.count(), 1);
        controller.setInterfaceScale(1.25);
        QCOMPARE(controller.interfaceScale(), 1.5);
        QCOMPARE(preferencesSpy.count(), 1);
    }

    ApplicationController restored;
    QCOMPARE(restored.interfaceScale(), 1.5);
    restored.setInterfaceScale(1.0);
}

void ApplicationCommandBridgeTest::onboardingStepsCanBeDeferredAndRestored()
{
    {
        ApplicationController controller;
        controller.resumeOnboardingStep(QStringLiteral("broadcast"));
        QSignalSpy onboardingSpy(&controller, &ApplicationController::onboardingChanged);
        QVERIFY(!controller.skipOnboardingStep(QStringLiteral("unknown")));
        QVERIFY(controller.skipOnboardingStep(QStringLiteral("broadcast")));
        QVERIFY(!controller.skipOnboardingStep(QStringLiteral("broadcast")));
        QCOMPARE(controller.onboardingSkippedSteps(), QStringList{QStringLiteral("broadcast")});
        QCOMPARE(onboardingSpy.count(), 1);
    }

    ApplicationController restored;
    QCOMPARE(restored.onboardingSkippedSteps(), QStringList{QStringLiteral("broadcast")});
    QVERIFY(restored.resumeOnboardingStep(QStringLiteral("broadcast")));
    QVERIFY(restored.onboardingSkippedSteps().isEmpty());
    QVERIFY(!restored.resumeOnboardingStep(QStringLiteral("broadcast")));
}

void ApplicationCommandBridgeTest::facadesExposeBoundedQmlContracts()
{
    ApplicationController controller;
    auto *automation = controller.automationContext();
    auto *bible = controller.bibleContext();
    auto *event = controller.eventContext();
    auto *integration = controller.integrationContext();
    auto *maintenance = controller.maintenanceContext();
    auto *media = controller.mediaContext();
    auto *output = controller.outputContext();

    QVERIFY(automation);
    QVERIFY(bible);
    QVERIFY(event);
    QVERIFY(integration);
    QVERIFY(maintenance);
    QVERIFY(media);
    QVERIFY(output);
    QCOMPARE(automation->automations(), controller.automations());
    QCOMPARE(automation->automationTriggerTypes(), controller.automationTriggerTypeList());
    QCOMPARE(integration->integrations(), controller.integrations());
    QCOMPARE(integration->integrationTypes(), controller.integrationTypes());
    QCOMPARE(bible->bibleBooks(), controller.bibleBooks());
    QCOMPARE(bible->favoriteBibleVerses(), controller.favoriteBibleVerses());
    QCOMPARE(event->events(), controller.events());
    QCOMPARE(maintenance->updateEndpoint(), controller.updateEndpoint());
    QCOMPARE(media->mediaPlaylist(), controller.mediaPlaylist());
    QCOMPARE(output->screens(), controller.screens());

    const auto automationMeta = automation->metaObject();
    QVERIFY(automationMeta->indexOfProperty("automations") >= 0);
    QVERIFY(automationMeta->indexOfMethod("saveAutomation(QVariantMap)") >= 0);
    QVERIFY(automationMeta->indexOfMethod("importAutomations(QUrl)") >= 0);
    const auto integrationMeta = integration->metaObject();
    QVERIFY(integrationMeta->indexOfProperty("integrations") >= 0);
    QVERIFY(integrationMeta->indexOfMethod("saveIntegration(QVariantMap)") >= 0);
    QVERIFY(integrationMeta->indexOfMethod("executeIntegration(QString,QString,QVariantMap)") >= 0);
    const auto bibleMeta = bible->metaObject();
    QVERIFY(bibleMeta->indexOfProperty("bibleTranslations") >= 0);
    QVERIFY(bibleMeta->indexOfProperty("favoriteBibleVerses") >= 0);
    QVERIFY(bibleMeta->indexOfMethod("presentBibleReference(int,int,int)") >= 0);
    QVERIFY(bibleMeta->indexOfMethod("toggleFavoriteBibleVerse(int)") >= 0);
    const auto eventMeta = event->metaObject();
    QVERIFY(eventMeta->indexOfProperty("eventItems") >= 0);
    QVERIFY(eventMeta->indexOfMethod("addEventItem(QString,QString,QString,qlonglong)") >= 0);
    const auto maintenanceMeta = maintenance->metaObject();
    QVERIFY(maintenanceMeta->indexOfProperty("diagnostics") >= 0);
    QVERIFY(maintenanceMeta->indexOfProperty("updateInstallable") >= 0);
    QVERIFY(maintenanceMeta->indexOfMethod("exportDiagnostics(QUrl)") >= 0);
    QVERIFY(maintenanceMeta->indexOfMethod("installDownloadedUpdate()") >= 0);
    const auto mediaMeta = media->metaObject();
    QVERIFY(mediaMeta->indexOfProperty("mediaPlaylist") >= 0);
    QVERIFY(mediaMeta->indexOfMethod("playMedia(QString)") >= 0);
    QVERIFY(mediaMeta->indexOfMethod("requestMediaThumbnail(QString,QString)") >= 0);
    const auto outputMeta = output->metaObject();
    QVERIFY(outputMeta->indexOfProperty("screens") >= 0);
    QVERIFY(outputMeta->indexOfMethod("toggleScreen(QString,bool)") >= 0);

    const auto controllerMeta = controller.metaObject();
    QVERIFY(controllerMeta->indexOfProperty("bibleThemeId") >= 0);
    QVERIFY(controllerMeta->indexOfProperty("lyricsThemeId") >= 0);
    QVERIFY(controllerMeta->indexOfMethod("updateThemeById(QString,QVariantMap)") >= 0);
    QVERIFY(controllerMeta->indexOfMethod("applyThemeForContent(QString,QString)") >= 0);
}

void ApplicationCommandBridgeTest::operatorBlackoutUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QCOMPARE(controller.diagnostics().value(QStringLiteral("schemaVersion")).toInt(), 5);
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.setBlackout(true);

    QVERIFY(controller.blackout());
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);

    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto result = qvariant_cast<CommandResult>(commandSpy.first().at(1));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("presentation.blackout.set"));
    QCOMPARE(command.source, QStringLiteral("operator"));
    QVERIFY(result.accepted);
    QCOMPARE(event.type, QStringLiteral("presentation.blackout.changed"));
    QCOMPARE(event.correlationId, command.id);
}

void ApplicationCommandBridgeTest::registersEveryCatalogCommand()
{
    ApplicationController controller;
    for (const auto &descriptor : CommandCatalog::descriptors()) {
        QVERIFY2(controller.commandBus().hasHandler(descriptor.type),
                 qPrintable(QStringLiteral("Handler ausente: %1").arg(descriptor.type)));
    }
}

void ApplicationCommandBridgeTest::operatorOverlayUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.setAudienceMessage(QStringLiteral("Aviso importante"));

    QCOMPARE(controller.audienceMessage(), QStringLiteral("Aviso importante"));
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);
    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("overlay.audience-message.set"));
    QCOMPARE(event.type, QStringLiteral("overlay.state.changed"));
    QCOMPARE(event.correlationId, command.id);
}

void ApplicationCommandBridgeTest::operatorMediaUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.stopMedia();

    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);
    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("media.stop"));
    QCOMPARE(event.type, QStringLiteral("media.state.changed"));
    QCOMPARE(event.correlationId, command.id);
}

void ApplicationCommandBridgeTest::stopThenPlayRestartsPlaylistFromBeginning()
{
    ApplicationController controller;
    controller.clearMediaPlaylist();

    const auto directory = qEnvironmentVariable("HOLYSCREEN_DATA_DIR");
    const auto firstPath = QDir(directory).filePath(QStringLiteral("first-slide.png"));
    const auto secondPath = QDir(directory).filePath(QStringLiteral("second-slide.png"));
    QImage firstImage(4, 4, QImage::Format_ARGB32_Premultiplied);
    QImage secondImage(4, 4, QImage::Format_ARGB32_Premultiplied);
    firstImage.fill(Qt::red);
    secondImage.fill(Qt::blue);
    QVERIFY(firstImage.save(firstPath));
    QVERIFY(secondImage.save(secondPath));
    QCOMPARE(controller.importImageFiles({QUrl::fromLocalFile(firstPath),
                                          QUrl::fromLocalFile(secondPath)}), 2);

    const auto playlist = controller.mediaPlaylist();
    QCOMPARE(playlist.size(), 2);
    const auto firstId = playlist.at(0).toMap().value(QStringLiteral("id")).toString();
    const auto secondId = playlist.at(1).toMap().value(QStringLiteral("id")).toString();
    QVERIFY(!firstId.isEmpty());
    QVERIFY(!secondId.isEmpty());

    controller.playMedia(secondId);
    QCOMPARE(controller.currentMediaId(), secondId);
    controller.stopMedia();
    QCOMPARE(controller.mediaState(), QStringLiteral("stopped"));
    controller.toggleMediaPause();
    QCOMPARE(controller.currentMediaId(), firstId);
    QCOMPARE(controller.mediaState(), QStringLiteral("playing"));
}

void ApplicationCommandBridgeTest::operatorPresentationNavigationUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QVERIFY(!controller.createTextPresentation(QStringLiteral("Avisos")).isEmpty());
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.showTextSlide(0);

    QVERIFY(controller.textVisible());
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);
    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("presentation.slide.show"));
    QCOMPARE(command.payload.value(QStringLiteral("index")).toInt(), 0);
    QCOMPARE(event.type, QStringLiteral("presentation.state.changed"));
    QCOMPARE(event.payload.value(QStringLiteral("slideIndex")).toInt(), 0);
    QCOMPARE(event.correlationId, command.id);
}

void ApplicationCommandBridgeTest::undoAndRedoUseCommandBus()
{
    qRegisterMetaType<Command>();
    ApplicationController controller;
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);

    controller.setBlackout(true);
    QVERIFY(controller.blackout());
    QVERIFY(controller.canUndo());

    controller.undo();
    QVERIFY(!controller.blackout());
    QVERIFY(controller.canRedo());
    QCOMPARE(qvariant_cast<Command>(commandSpy.last().at(0)).type,
             QStringLiteral("system.undo"));

    controller.redo();
    QVERIFY(controller.blackout());
    QVERIFY(controller.canUndo());
    QCOMPARE(qvariant_cast<Command>(commandSpy.last().at(0)).type,
             QStringLiteral("system.redo"));
}

void ApplicationCommandBridgeTest::autosaveDebouncesPresentationEdits()
{
    ApplicationController controller;
    const auto presentationId = controller.createTextPresentation(QStringLiteral("Avisos"));
    QVERIFY(!presentationId.isEmpty());
    const auto slideId = controller.currentSlideId();
    QVERIFY(!slideId.isEmpty());

    controller.updateTextSlide(slideId, QStringLiteral("1"), QStringLiteral("Primeiro texto"));
    controller.updateTextSlide(slideId, QStringLiteral("1"), QStringLiteral("Texto final"));

    QVERIFY(controller.autosavePending());
    QTRY_VERIFY_WITH_TIMEOUT(!controller.autosavePending(), 2000);

    PresentationRepository persisted(
        qEnvironmentVariable("HOLYSCREEN_DATA_DIR") + QStringLiteral("/presenter.db"));
    QVERIFY(persisted.open());
    const auto reloaded = persisted.presentation(presentationId);
    QCOMPARE(reloaded.slides.size(), 1);
    QCOMPARE(reloaded.slides.front().text, QStringLiteral("Texto final"));
}

void ApplicationCommandBridgeTest::presentationEditsSupportUndoAndRedo()
{
    ApplicationController controller;
    QVERIFY(!controller.createTextPresentation(QStringLiteral("Avisos")).isEmpty());
    const auto slideId = controller.currentSlideId();
    controller.updateTextSlide(slideId, QStringLiteral("1"), QStringLiteral("Texto alterado"));
    QCOMPARE(controller.currentSlideText(), QStringLiteral("Texto alterado"));

    QVERIFY(controller.canUndo());
    controller.undo();
    QCOMPARE(controller.currentSlideText(), QString{});
    QVERIFY(controller.canRedo());
    controller.redo();
    QCOMPARE(controller.currentSlideText(), QStringLiteral("Texto alterado"));
}

void ApplicationCommandBridgeTest::canonicalBibleImportRequiresLicenseThenCompletesAsynchronously()
{
    QTemporaryDir source;
    const auto translation = source.filePath(QStringLiteral("data/canonical/DEMO"));
    QVERIFY(QDir().mkpath(translation));
    QFile metadata(QDir(translation).filePath(QStringLiteral("meta.json")));
    QVERIFY(metadata.open(QIODevice::WriteOnly));
    QVERIFY(metadata.write(
        R"JSON({"code":"DEMO","name":"Demonstração","license":"copyright"})JSON") > 0);
    metadata.close();
    QFile book(QDir(translation).filePath(QStringLiteral("JHN.json")));
    QVERIFY(book.open(QIODevice::WriteOnly));
    QVERIFY(book.write(
        R"JSON({"id":43,"chapters":[{"number":3,"verses":[{"number":16,"text":"Texto de demonstração"}]}]})JSON") > 0);
    book.close();

    ApplicationController controller;
    QSignalSpy stateSpy(&controller, &ApplicationController::bibleImportStateChanged);
    QVERIFY(controller.importBibleFolder(QUrl::fromLocalFile(source.path())));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.bibleImportRunning(), 3000);
    QVERIFY(controller.bibleImportRequiresLicenseConfirmation());
    QVERIFY(controller.bibleTranslations().isEmpty());

    QVERIFY(controller.confirmBibleImportLicenses());
    QTRY_VERIFY_WITH_TIMEOUT(!controller.bibleImportRunning(), 3000);
    QVERIFY(!controller.bibleImportRequiresLicenseConfirmation());
    QCOMPARE(controller.bibleTranslations().size(), 1);
    QCOMPARE(controller.bibleImportProgress(), 100);
    QVERIFY(controller.bibleImportMessage().contains(QStringLiteral("1 tradução")));
    QVERIFY(stateSpy.count() > 2);
}

void ApplicationCommandBridgeTest::remoteServerRequiresPasswordAndPersistsLocalConfiguration()
{
    QTcpServer portProbe;
    QVERIFY(portProbe.listen(QHostAddress::LocalHost, 0));
    const auto port = portProbe.serverPort();
    portProbe.close();

    {
        ApplicationController controller;
        QVERIFY(!controller.remoteEnabled());
        controller.setRemoteEnabled(true);
        QVERIFY(!controller.remoteEnabled());
        QVERIFY(controller.setRemotePassword(QStringLiteral("senha-local")));
        controller.setRemoteInterface(QStringLiteral("127.0.0.1"));
        controller.setRemotePort(port);
        controller.setRemoteEnabled(true);
        QVERIFY2(controller.remoteEnabled(), qPrintable(controller.remoteError()));
        QVERIFY(controller.remoteUrl().contains(QString::number(port)));
        QVERIFY(controller.remoteQrCode().startsWith(
            QStringLiteral("data:image/svg+xml;base64,")));

        QNetworkAccessManager network;
        const auto login = postJson(
            network, QUrl(controller.remoteUrl() + QStringLiteral("/api/v1/session")),
            QByteArrayLiteral(R"JSON({"password":"senha-local"})JSON"));
        QCOMPARE(login.status, 201);
        const auto token = login.body.value(QStringLiteral("token")).toString();
        QVERIFY(!token.isEmpty());
        const auto command = postJson(
            network, QUrl(controller.remoteUrl() + QStringLiteral("/api/v1/commands")),
            QByteArrayLiteral(R"JSON({"id":"bridge-e2e","type":"stage.message.set","payload":{"message":"Mensagem remota"}})JSON"),
            token);
        QCOMPARE(command.status, 200);
        QVERIFY(command.body.value(QStringLiteral("accepted")).toBool());
        QCOMPARE(controller.stageMessage(), QStringLiteral("Mensagem remota"));
    }

    ApplicationController restored;
    QVERIFY(restored.remotePasswordConfigured());
    QCOMPARE(restored.remotePort(), static_cast<int>(port));
    QCOMPARE(restored.remoteInterface(), QStringLiteral("127.0.0.1"));
    QVERIFY2(restored.remoteEnabled(), qPrintable(restored.remoteError()));
}

void ApplicationCommandBridgeTest::broadcastProfilesAreValidatedAndSurviveARestart()
{
    QString fingerprint;
    {
        ApplicationController controller;
        QVERIFY(!controller.screens().isEmpty());
        fingerprint = controller.screens().first().toMap()
                          .value(QStringLiteral("fingerprint")).toString();
        QVERIFY(!fingerprint.isEmpty());

        const auto defaults = controller.outputBroadcastProfile(fingerprint);
        QCOMPARE(defaults.value(QStringLiteral("backgroundMode")).toString(),
                 QStringLiteral("chroma"));
        QCOMPARE(defaults.value(QStringLiteral("chromaColor")).toString(),
                 QStringLiteral("#00b140"));

        QVERIFY(!controller.setOutputBroadcastProfile(
            fingerprint, {{QStringLiteral("backgroundMode"), QStringLiteral("alpha")}}));
        QCOMPARE(controller.outputBroadcastProfile(fingerprint)
                     .value(QStringLiteral("backgroundMode")).toString(),
                 QStringLiteral("chroma"));

        QVERIFY(controller.setOutputBroadcastProfile(fingerprint, {
            {QStringLiteral("backgroundMode"), QStringLiteral("transparent")},
            {QStringLiteral("aspectPreset"), QStringLiteral("9:16")},
            {QStringLiteral("safeAreaLeft"), 8.0},
            {QStringLiteral("showClock"), true},
            {QStringLiteral("showLowerThird"), false},
        }));
        const auto updated = controller.outputBroadcastProfile(fingerprint);
        QCOMPARE(updated.value(QStringLiteral("backgroundMode")).toString(),
                 QStringLiteral("transparent"));
        QCOMPARE(updated.value(QStringLiteral("aspectPreset")).toString(),
                 QStringLiteral("9:16"));
        QCOMPARE(updated.value(QStringLiteral("safeAreaLeft")).toDouble(), 8.0);
        QVERIFY(updated.value(QStringLiteral("showClock")).toBool());
        QVERIFY(!updated.value(QStringLiteral("showLowerThird")).toBool());
        QVERIFY(controller.canUndo());

        QVERIFY(controller.screens().first().toMap()
                    .value(QStringLiteral("broadcast")).toMap()
                    .value(QStringLiteral("showClock")).toBool());
    }

    ApplicationController restored;
    const auto persisted = restored.outputBroadcastProfile(fingerprint);
    QCOMPARE(persisted.value(QStringLiteral("backgroundMode")).toString(),
             QStringLiteral("transparent"));
    QCOMPARE(persisted.value(QStringLiteral("aspectPreset")).toString(), QStringLiteral("9:16"));
    QCOMPARE(persisted.value(QStringLiteral("safeAreaLeft")).toDouble(), 8.0);
    QVERIFY(persisted.value(QStringLiteral("showClock")).toBool());
    QVERIFY(!persisted.value(QStringLiteral("showLowerThird")).toBool());
}

void ApplicationCommandBridgeTest::integrationsAreValidatedExecutedAndRecordedWithoutSecrets()
{
    QHttpServer server;
    QByteArray receivedAuthorization;
    int receivedRequests = 0;
    server.route(QStringLiteral("/hook"), QHttpServerRequest::Method::Post,
                 [&receivedAuthorization, &receivedRequests](const QHttpServerRequest &request) {
        receivedAuthorization = request.value(QByteArrayLiteral("Authorization"));
        ++receivedRequests;
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Ok);
    });
    QTcpServer tcp;
    QVERIFY(tcp.listen(QHostAddress::LocalHost, 0));
    const auto port = tcp.serverPort();
    QVERIFY(server.bind(&tcp));

    ApplicationController controller;
    QVERIFY(controller.integrations().isEmpty());
    QVERIFY(!controller.integrationSecretBackend().isEmpty());

    // Uma definição inválida é recusada antes de ser persistida.
    const auto invalid = controller.saveIntegration({
        {QStringLiteral("name"), QStringLiteral("Sem URL")},
        {QStringLiteral("type"), QStringLiteral("http")},
    });
    QVERIFY(!invalid.value(QStringLiteral("accepted")).toBool());
    QVERIFY(!invalid.value(QStringLiteral("errors")).toStringList().isEmpty());
    QVERIFY(controller.integrations().isEmpty());

    const auto saved = controller.saveIntegration({
        {QStringLiteral("id"), QStringLiteral("hook")},
        {QStringLiteral("name"), QStringLiteral("Webhook do culto")},
        {QStringLiteral("type"), QStringLiteral("http")},
        {QStringLiteral("timeoutMs"), 3000},
        {QStringLiteral("configuration"),
         QVariantMap{{QStringLiteral("url"),
                      QStringLiteral("http://127.0.0.1:%1/hook").arg(port)},
                     {QStringLiteral("method"), QStringLiteral("POST")}}},
    });
    QVERIFY2(saved.value(QStringLiteral("accepted")).toBool(),
             qPrintable(saved.value(QStringLiteral("errors")).toStringList().join(QLatin1Char(' '))));
    QCOMPARE(controller.integrations().size(), 1);

    // O segredo vai para o cofre e a configuração guarda só a referência.
    const auto reference = controller.setIntegrationSecret(
        QStringLiteral("hook"), QStringLiteral("headers.Authorization"),
        QStringLiteral("Bearer super-secreto"));
    QVERIFY(!reference.isEmpty());
    const auto stored = controller.integrationDefinition(QStringLiteral("hook"));
    const auto configuration = stored.value(QStringLiteral("configuration")).toMap();
    for (const auto &value : configuration) {
        QVERIFY(!value.toString().contains(QStringLiteral("super-secreto")));
    }

    // O campo com ponto virou uma chave dentro do mapa de cabeçalhos, e a
    // cópia entregue ao QML já vem com o valor mascarado.
    QVERIFY(configuration.contains(QStringLiteral("headers")));
    QCOMPARE(configuration.value(QStringLiteral("headers")).toMap()
                 .value(QStringLiteral("Authorization")).toString(),
             QStringLiteral("***"));
    QCOMPARE(stored.value(QStringLiteral("secretReferences")).toStringList(),
             QStringList{reference});

    QSignalSpy historySpy(&controller, &ApplicationController::integrationHistoryChanged);
    QVERIFY(controller.executeIntegration(QStringLiteral("hook"),
                                          QStringLiteral("request.send"),
                                          {{QStringLiteral("slide"), 3}}));
    QTRY_VERIFY(historySpy.count() > 0);
    QTRY_COMPARE(receivedAuthorization, QByteArrayLiteral("Bearer super-secreto"));

    const auto history = controller.integrationHistory();
    QCOMPARE(history.size(), 1);
    const auto call = history.first().toMap();
    QCOMPARE(call.value(QStringLiteral("integrationId")).toString(), QStringLiteral("hook"));
    QCOMPARE(call.value(QStringLiteral("operation")).toString(), QStringLiteral("request.send"));
    QVERIFY(call.value(QStringLiteral("accepted")).toBool());
    for (const auto &value : call) {
        QVERIFY(!value.toString().contains(QStringLiteral("super-secreto")));
    }
    QVERIFY(!controller.integrationStatus().contains(QStringLiteral("super-secreto")));

    // E2E da onda: um fato de slide atravessa EventBus, AutomationEngine,
    // IntegrationEngine e o transporte HTTP real em porta efêmera.
    const auto automated = controller.saveAutomation({
        {QStringLiteral("name"), QStringLiteral("Webhook ao trocar slide")},
        {QStringLiteral("triggerType"), QStringLiteral("slide.changed")},
        {QStringLiteral("actions"), QVariantList{QVariantMap{
            {QStringLiteral("type"), QStringLiteral("integration")},
            {QStringLiteral("parameters"), QVariantMap{
                {QStringLiteral("integrationId"), QStringLiteral("hook")},
                {QStringLiteral("operation"), QStringLiteral("request.send")},
                {QStringLiteral("payload"), QVariantMap{{QStringLiteral("source"),
                                                         QStringLiteral("automation-e2e")}}},
            }},
        }}},
    });
    QVERIFY(automated.value(QStringLiteral("accepted")).toBool());
    QVERIFY(controller.eventBus().publish(DomainEvent{
        .type = QStringLiteral("presentation.state.changed"),
        .payload = {{QStringLiteral("action"), QStringLiteral("slide.next")},
                    {QStringLiteral("slideIndex"), 2}},
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = QStringLiteral("e2e-slide"),
    }));
    QTRY_COMPARE_WITH_TIMEOUT(receivedRequests, 2, 3000);
    QVERIFY(controller.removeAutomation(
        automated.value(QStringLiteral("id")).toString()));

    // O diagnóstico traz nome, tipo e estado, sem configuração.
    const auto diagnostics = controller.diagnostics();
    const auto summary = diagnostics.value(QStringLiteral("integrations")).toStringList();
    QCOMPARE(summary.size(), 1);
    QVERIFY(summary.first().contains(QStringLiteral("Webhook do culto")));
    QVERIFY(summary.first().contains(QStringLiteral("http")));
    QVERIFY(!summary.first().contains(QStringLiteral("127.0.0.1")));

    // Duplicar não copia segredos e mantém a cópia desativada.
    const auto copyId = controller.duplicateIntegration(QStringLiteral("hook"));
    QVERIFY(!copyId.isEmpty());
    const auto copy = controller.integrationDefinition(copyId);
    QVERIFY(!copy.value(QStringLiteral("enabled")).toBool());
    QVERIFY(copy.value(QStringLiteral("secretReferences")).toStringList().isEmpty());

    QVERIFY(controller.removeIntegration(copyId));
    QVERIFY(controller.removeIntegration(QStringLiteral("hook")));
    QVERIFY(controller.integrations().isEmpty());
}

void ApplicationCommandBridgeTest::automationsReactToDomainEventsWithoutLooping()
{
    ApplicationController controller;
    QVERIFY(controller.automations().isEmpty());
    QVERIFY(controller.automationsEnabled());
    // Processos externos vêm desligados.
    QVERIFY(!controller.processActionsEnabled());

    // Uma automação inválida é recusada antes de ser salva.
    const auto invalid = controller.saveAutomation({
        {QStringLiteral("name"), QStringLiteral("Sem gatilho")},
        {QStringLiteral("triggerType"), QStringLiteral("inexistente")},
        {QStringLiteral("actions"),
         QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("command")},
                                  {QStringLiteral("parameters"),
                                   QVariantMap{{QStringLiteral("type"),
                                                QStringLiteral("comando.inexistente")}}}}}},
    });
    QVERIFY(!invalid.value(QStringLiteral("accepted")).toBool());
    QCOMPARE(invalid.value(QStringLiteral("errors")).toStringList().size(), 2);
    QVERIFY(controller.automations().isEmpty());

    const auto saved = controller.saveAutomation({
        {QStringLiteral("name"), QStringLiteral("Blackout no último slide")},
        {QStringLiteral("triggerType"), QStringLiteral("slide.changed")},
        {QStringLiteral("actions"),
         QVariantList{QVariantMap{
             {QStringLiteral("type"), QStringLiteral("command")},
             {QStringLiteral("parameters"),
              QVariantMap{{QStringLiteral("type"), QStringLiteral("presentation.blackout.set")},
                          {QStringLiteral("payload"),
                           QVariantMap{{QStringLiteral("enabled"), true}}}}}}}},
    });
    QVERIFY2(saved.value(QStringLiteral("accepted")).toBool(),
             qPrintable(saved.value(QStringLiteral("errors")).toStringList().join(QLatin1Char(' '))));
    QCOMPARE(controller.automations().size(), 1);

    QVERIFY(!controller.blackout());
    QSignalSpy runsSpy(&controller, &ApplicationController::automationRunsChanged);

    // Um fato real do domínio dispara a automação.
    controller.commandBus().dispatch(Command{
        .id = QStringLiteral("operador-1"),
        .type = QStringLiteral("presentation.slide.next"),
        .payload = {},
        .source = QStringLiteral("operator"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });

    QVERIFY(controller.blackout());
    QVERIFY(runsSpy.count() > 0);
    const auto runs = controller.automationRuns();
    QVERIFY(!runs.isEmpty());
    const auto lastRun = runs.first().toMap();
    QCOMPARE(lastRun.value(QStringLiteral("status")).toString(), QStringLiteral("completed"));
    QVERIFY(lastRun.value(QStringLiteral("correlationId")).toString()
                .startsWith(QStringLiteral("operador-1/")));

    // O comando disparado pela automação não pode reentrar na mesma cadeia.
    const auto completedRuns = std::count_if(
        runs.cbegin(), runs.cend(), [](const QVariant &entry) {
            return entry.toMap().value(QStringLiteral("status")).toString()
                   == QStringLiteral("completed");
        });
    QCOMPARE(completedRuns, 1);

    // Ensaio não altera nada e é registrado como dry-run.
    controller.setBlackout(false);
    const auto dry = controller.dryRunAutomation(
        controller.automations().first().toMap().value(QStringLiteral("id")).toString());
    QCOMPARE(dry.value(QStringLiteral("status")).toString(), QStringLiteral("dry-run"));
    QVERIFY(!controller.blackout());

    // Interruptor global.
    controller.setAutomationsEnabled(false);
    controller.commandBus().dispatch(Command{
        .id = QStringLiteral("operador-2"),
        .type = QStringLiteral("presentation.slide.next"),
        .payload = {},
        .source = QStringLiteral("operator"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
    QVERIFY(!controller.blackout());
    controller.setAutomationsEnabled(true);

    // Uma ação de processo só é aceita com o executável autorizado.
    const auto processHelper =
        QFileInfo(QString::fromUtf8(TEST_PROCESS_HELPER_PATH)).canonicalFilePath();
    QVERIFY(!processHelper.isEmpty());
    const auto withProcess = controller.saveAutomation({
        {QStringLiteral("name"), QStringLiteral("Script")},
        {QStringLiteral("triggerType"), QStringLiteral("media.started")},
        {QStringLiteral("actions"),
         QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("process")},
                                  {QStringLiteral("parameters"),
                                   QVariantMap{{QStringLiteral("executable"),
                                                processHelper}}}}}},
    });
    QVERIFY(!withProcess.value(QStringLiteral("accepted")).toBool());
    QVERIFY(withProcess.value(QStringLiteral("errors")).toStringList().first()
                .contains(QStringLiteral("não está autorizado")));

    QVERIFY(controller.authorizeExecutable(processHelper, QStringLiteral("Helper de teste"))
                .value(QStringLiteral("accepted")).toBool());
    QCOMPARE(controller.authorizedExecutables().size(), 1);
    QVERIFY(controller.saveAutomation({
        {QStringLiteral("name"), QStringLiteral("Script")},
        {QStringLiteral("triggerType"), QStringLiteral("media.started")},
        {QStringLiteral("actions"),
         QVariantList{QVariantMap{{QStringLiteral("type"), QStringLiteral("process")},
                                  {QStringLiteral("parameters"),
                                   QVariantMap{{QStringLiteral("executable"),
                                                processHelper}}}}}},
    }).value(QStringLiteral("accepted")).toBool());
    QVERIFY(controller.revokeExecutable(
        controller.authorizedExecutables().first().toMap()
            .value(QStringLiteral("canonicalPath")).toString()));

    const auto automationId = controller.automations().first().toMap()
                                  .value(QStringLiteral("id")).toString();
    QVERIFY(controller.setAutomationEnabled(automationId, false));
    QVERIFY(controller.removeAutomation(automationId));
}

void ApplicationCommandBridgeTest::acceptedRemoteAndTimerCommandsBecomeAutomationFacts()
{
    qRegisterMetaType<DomainEvent>();
    ApplicationController controller;
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    const auto result = controller.commandBus().dispatch(Command{
        .id = QStringLiteral("remote-timer-1"),
        .type = QStringLiteral("timer.countdown.start"),
        .payload = {{QStringLiteral("seconds"), 1}},
        .source = QStringLiteral("remote:test"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
    QVERIFY(result.accepted);

    const auto countEvents = [&eventSpy](const QString &type) {
        return std::count_if(eventSpy.cbegin(), eventSpy.cend(), [&type](const QList<QVariant> &row) {
            return row.first().value<DomainEvent>().type == type;
        });
    };
    QCOMPARE(countEvents(QStringLiteral("automation.remote.command")), 1);
    QCOMPARE(countEvents(QStringLiteral("automation.timer.started")), 1);
    QTRY_COMPARE_WITH_TIMEOUT(countEvents(QStringLiteral("automation.timer.finished")), 1, 2500);
}

void ApplicationCommandBridgeTest::automationDefinitionsRoundTripWithoutOverwritingOrSecrets()
{
    ApplicationController controller;
    const auto leftovers = controller.automations();
    for (const auto &item : leftovers) {
        QVERIFY(controller.removeAutomation(item.toMap().value(QStringLiteral("id")).toString()));
    }
    QVERIFY(controller.automations().isEmpty());
    const auto saved = controller.saveAutomation({
        {QStringLiteral("name"), QStringLiteral("Exportável")},
        {QStringLiteral("triggerType"), QStringLiteral("slide.changed")},
        {QStringLiteral("actions"),
         QVariantList{QVariantMap{
             {QStringLiteral("type"), QStringLiteral("command")},
             {QStringLiteral("parameters"),
              QVariantMap{{QStringLiteral("type"),
                           QStringLiteral("presentation.slide.next")}}}}}},
    });
    QVERIFY(saved.value(QStringLiteral("accepted")).toBool());

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto exportedPath = directory.filePath(QStringLiteral("automations.json"));
    const auto exported = controller.exportAutomations(QUrl::fromLocalFile(exportedPath));
    QVERIFY(exported.value(QStringLiteral("accepted")).toBool());
    QFile exportedFile(exportedPath);
    QVERIFY(exportedFile.open(QIODevice::ReadOnly));
    const auto bytes = exportedFile.readAll();
    QVERIFY(!bytes.contains("secretReferences"));
    QCOMPARE(QJsonDocument::fromJson(bytes).object()
                 .value(QStringLiteral("schemaVersion")).toInt(), 1);

    const auto imported = controller.importAutomations(QUrl::fromLocalFile(exportedPath));
    QVERIFY2(imported.value(QStringLiteral("accepted")).toBool(),
             qPrintable(imported.value(QStringLiteral("errors")).toStringList().join('\n')));
    QCOMPARE(imported.value(QStringLiteral("count")).toInt(), 1);
    QCOMPARE(controller.automations().size(), 2);
    const auto firstId = controller.automations().at(0).toMap().value(QStringLiteral("id")).toString();
    const auto secondId = controller.automations().at(1).toMap().value(QStringLiteral("id")).toString();
    QVERIFY(firstId != secondId);

    const QJsonObject unresolvedDefinition{
        {QStringLiteral("name"), QStringLiteral("Integração ausente")},
        {QStringLiteral("enabled"), true},
        {QStringLiteral("triggerType"), QStringLiteral("media.started")},
        {QStringLiteral("conditions"), QJsonArray{}},
        {QStringLiteral("actions"), QJsonArray{QJsonObject{
             {QStringLiteral("type"), QStringLiteral("integration")},
             {QStringLiteral("parameters"),
              QJsonObject{{QStringLiteral("integrationId"), QStringLiteral("ausente")},
                          {QStringLiteral("operation"), QStringLiteral("send")}}},
        }}},
    };
    const auto unresolvedPath = directory.filePath(QStringLiteral("unresolved.json"));
    QFile unresolvedFile(unresolvedPath);
    QVERIFY(unresolvedFile.open(QIODevice::WriteOnly));
    unresolvedFile.write(QJsonDocument(QJsonObject{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("automations"), QJsonArray{unresolvedDefinition}},
    }).toJson());
    unresolvedFile.close();

    const auto unresolved = controller.importAutomations(QUrl::fromLocalFile(unresolvedPath));
    QVERIFY(unresolved.value(QStringLiteral("accepted")).toBool());
    QCOMPARE(unresolved.value(QStringLiteral("warnings")).toStringList().size(), 1);
    const auto definitions = controller.automations();
    const auto disabled = std::find_if(definitions.cbegin(), definitions.cend(), [](const QVariant &item) {
        return item.toMap().value(QStringLiteral("name")).toString()
            == QStringLiteral("Integração ausente");
    });
    QVERIFY(disabled != definitions.cend());
    QVERIFY(!disabled->toMap().value(QStringLiteral("enabled")).toBool());

    const auto ids = controller.automations();
    for (const auto &item : ids) {
        QVERIFY(controller.removeAutomation(item.toMap().value(QStringLiteral("id")).toString()));
    }
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    // O teste nunca pode escrever no cofre real da máquina que roda a suíte.
    qputenv("HOLYSCREEN_SECRET_STORE", QByteArrayLiteral("memory"));
    QGuiApplication app(argc, argv);
    QTemporaryDir dataDirectory;
    if (!dataDirectory.isValid()) return 2;
    qputenv("HOLYSCREEN_DATA_DIR", dataDirectory.path().toUtf8());
    ApplicationCommandBridgeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "ApplicationCommandBridgeTest.moc"
