#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/ApplicationController.h"
#include "library/PresentationRepository.h"
#include "core/CommandCatalog.h"

#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
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
    void operatorPresentationNavigationUsesCommandAndEventBuses();
    void undoAndRedoUseCommandBus();
    void autosaveDebouncesPresentationEdits();
    void presentationEditsSupportUndoAndRedo();
    void canonicalBibleImportRequiresLicenseThenCompletesAsynchronously();
    void remoteServerRequiresPasswordAndPersistsLocalConfiguration();
    void broadcastProfilesAreValidatedAndSurviveARestart();
    void integrationsAreValidatedExecutedAndRecordedWithoutSecrets();
};

void ApplicationCommandBridgeTest::operatorBlackoutUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QCOMPARE(controller.diagnostics().value(QStringLiteral("schemaVersion")).toInt(), 4);
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
    server.route(QStringLiteral("/hook"), QHttpServerRequest::Method::Post,
                 [&receivedAuthorization](const QHttpServerRequest &request) {
        receivedAuthorization = request.value(QByteArrayLiteral("Authorization"));
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

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    QGuiApplication app(argc, argv);
    QTemporaryDir dataDirectory;
    if (!dataDirectory.isValid()) return 2;
    qputenv("HOLYSCREEN_DATA_DIR", dataDirectory.path().toUtf8());
    ApplicationCommandBridgeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "ApplicationCommandBridgeTest.moc"
