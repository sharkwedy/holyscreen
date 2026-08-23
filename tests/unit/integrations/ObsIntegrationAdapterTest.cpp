#include "integrations/adapters/ObsAuthentication.h"
#include "integrations/adapters/ObsIntegrationAdapter.h"
#include "integrations/secrets/InMemorySecretStore.h"
#include "integrations/transports/ObsWebSocketClient.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QWebSocket>
#include <QWebSocketServer>

using namespace churchpresenter;

namespace {

//! Servidor de conformidade do protocolo OBS WebSocket v5: envia Hello com
//! desafio, valida a autenticação e responde aos pedidos correlacionados.
class FakeObsServer final : public QObject {
    Q_OBJECT

public:
    explicit FakeObsServer(QObject *parent = nullptr)
        : QObject(parent)
        , m_server(QStringLiteral("fake-obs"), QWebSocketServer::NonSecureMode, this)
    {
        connect(&m_server, &QWebSocketServer::newConnection, this, &FakeObsServer::onConnection);
    }

    bool listen() { return m_server.listen(QHostAddress::LocalHost); }
    [[nodiscard]] quint16 port() const { return m_server.serverPort(); }

    QString password = QStringLiteral("obs-secreta");
    QString salt = QStringLiteral("PZVbYpvAnZut2SS6JNJytDm9");
    QString challenge = QStringLiteral("ztTBnnuqrqaKDzRM3xcVdbYm");
    bool requireAuthentication = true;
    bool failNextRequest = false;
    bool ignoreNextRequest = false;
    QStringList receivedRequests;
    QList<QJsonObject> receivedData;
    QString lastAuthentication;

private slots:
    void onConnection()
    {
        auto *socket = m_server.nextPendingConnection();
        m_clients.append(socket);
        connect(socket, &QWebSocket::textMessageReceived, socket,
                [this, socket](const QString &message) { onMessage(socket, message); });

        QJsonObject hello{{QStringLiteral("obsWebSocketVersion"), QStringLiteral("5.4.0")},
                          {QStringLiteral("rpcVersion"), 1}};
        if (requireAuthentication) {
            hello.insert(QStringLiteral("authentication"),
                         QJsonObject{{QStringLiteral("challenge"), challenge},
                                     {QStringLiteral("salt"), salt}});
        }
        send(socket, 0, hello);
    }

private:
    void onMessage(QWebSocket *socket, const QString &message)
    {
        const auto envelope = QJsonDocument::fromJson(message.toUtf8()).object();
        const auto op = envelope.value(QStringLiteral("op")).toInt(-1);
        const auto data = envelope.value(QStringLiteral("d")).toObject();

        if (op == 1) {
            lastAuthentication = data.value(QStringLiteral("authentication")).toString();
            if (requireAuthentication
                && lastAuthentication != ObsAuthentication::response(password, salt, challenge)) {
                socket->close(QWebSocketProtocol::CloseCodeNormal,
                              QStringLiteral("Authentication failed"));
                return;
            }
            send(socket, 2, QJsonObject{{QStringLiteral("negotiatedRpcVersion"), 1}});
            return;
        }

        if (op == 6) {
            const auto requestType = data.value(QStringLiteral("requestType")).toString();
            receivedRequests.append(requestType);
            receivedData.append(data.value(QStringLiteral("requestData")).toObject());
            if (ignoreNextRequest) {
                ignoreNextRequest = false;
                return;
            }
            QJsonObject responseData;
            if (requestType == QStringLiteral("GetVersion")) {
                responseData = {{QStringLiteral("obsVersion"), QStringLiteral("30.1.2")},
                                {QStringLiteral("rpcVersion"), 1}};
            }
            const bool success = !failNextRequest;
            failNextRequest = false;
            send(socket, 7,
                 QJsonObject{
                     {QStringLiteral("requestType"), requestType},
                     {QStringLiteral("requestId"), data.value(QStringLiteral("requestId"))},
                     {QStringLiteral("requestStatus"),
                      QJsonObject{{QStringLiteral("result"), success},
                                  {QStringLiteral("code"), success ? 100 : 604},
                                  {QStringLiteral("comment"),
                                   success ? QString{} : QStringLiteral("Cena inexistente")}}},
                     {QStringLiteral("responseData"), responseData}});
        }
    }

    static void send(QWebSocket *socket, int op, const QJsonObject &data)
    {
        socket->sendTextMessage(QString::fromUtf8(
            QJsonDocument(QJsonObject{{QStringLiteral("op"), op}, {QStringLiteral("d"), data}})
                .toJson(QJsonDocument::Compact)));
        socket->flush();
    }

    QWebSocketServer m_server;
    QList<QWebSocket *> m_clients;
};

class FakeObsClient final : public IObsClient {
public:
    void connectAndIdentify(const QString &host, quint16 port, const QString &password,
                            int timeoutMs, Completion completion) override
    {
        lastHost = host;
        lastPort = port;
        lastPassword = password;
        lastTimeoutMs = timeoutMs;
        ++connections;
        if (connectSucceeds) currentState = QStringLiteral("identified");
        completion(ObsResponse{.completed = true,
                               .success = connectSucceeds,
                               .errorCode = connectSucceeds ? QString{}
                                                            : QStringLiteral("connection_failed"),
                               .message = connectSucceeds ? QString{}
                                                          : QStringLiteral("OBS indisponível")});
    }

    void request(const QString &requestType, const QJsonObject &requestData, int,
                 Completion completion) override
    {
        requests.append(requestType);
        data.append(requestData);
        completion(response);
    }

    void disconnect() override { currentState = QStringLiteral("disconnected"); }
    QString state() const override { return currentState; }
    void cancelAll() override { ++cancellations; }

    QString lastHost;
    quint16 lastPort = 0;
    QString lastPassword;
    int lastTimeoutMs = 0;
    int connections = 0;
    int cancellations = 0;
    bool connectSucceeds = true;
    QString currentState = QStringLiteral("disconnected");
    QStringList requests;
    QList<QJsonObject> data;
    ObsResponse response{.completed = true, .success = true};
};

IntegrationDefinition obsDefinition(quint16 port = 4455)
{
    return IntegrationDefinition{
        .id = QStringLiteral("obs"),
        .name = QStringLiteral("OBS do púlpito"),
        .type = IntegrationType::Obs,
        .enabled = true,
        .configuration = {{QStringLiteral("host"), QStringLiteral("127.0.0.1")},
                          {QStringLiteral("port"), port},
                          {QStringLiteral("passwordReference"), QStringLiteral("obs/password")}},
        .secretReferences = {QStringLiteral("obs/password")},
        .timeoutMs = 3000,
    };
}

} // namespace

class ObsIntegrationAdapterTest final : public QObject {
    Q_OBJECT

private slots:
    void computesTheDocumentedChallengeResponse();
    void requiresThePasswordToLiveInTheVault();
    void mapsEveryOperationToItsObsRequest();
    void reportsObsErrorsAndUnsupportedOperations();
    void retriesOnlyIdempotentOperations();
    void identifiesAgainstAConformanceServerAndRunsRequests();
    void reportsAuthenticationFailureAndRequestTimeout();
};

void ObsIntegrationAdapterTest::computesTheDocumentedChallengeResponse()
{
    const auto password = QStringLiteral("supersecretpassword");
    const auto salt = QStringLiteral("PZVbYpvAnZut2SS6JNJytDm9");
    const auto challenge = QStringLiteral("ztTBnnuqrqaKDzRM3xcVdbYm");

    // Os dois passos do protocolo v5, calculados aqui de forma independente:
    // base64(sha256(senha + salt)) e depois base64(sha256(disso + challenge)).
    const auto secret = QCryptographicHash::hash((password + salt).toUtf8(),
                                                 QCryptographicHash::Sha256).toBase64();
    const auto expected = QString::fromLatin1(
        QCryptographicHash::hash(secret + challenge.toUtf8(),
                                 QCryptographicHash::Sha256).toBase64());

    QCOMPARE(ObsAuthentication::response(password, salt, challenge), expected);
    // Constante de regressão conferida contra uma implementação independente.
    QCOMPARE(expected, QStringLiteral("zZgWipvwSGrw748kHN4gNpBC1IaeiiWX3Hjkrm849Sc="));

    // Senha diferente muda a resposta, e o segredo nunca aparece nela.
    QVERIFY(ObsAuthentication::response(QStringLiteral("outra"), salt, challenge) != expected);
    QVERIFY(!expected.contains(password));
}

void ObsIntegrationAdapterTest::requiresThePasswordToLiveInTheVault()
{
    FakeObsClient client;
    InMemorySecretStore secrets;
    secrets.store(QStringLiteral("obs/password"), QStringLiteral("obs-secreta"));
    ObsIntegrationAdapter adapter(client, &secrets);

    QVERIFY(adapter.validate(obsDefinition()).valid);

    auto definition = obsDefinition();
    definition.secretReferences.clear();
    QVERIFY(!adapter.validate(definition).valid);

    definition = obsDefinition();
    definition.configuration.insert(QStringLiteral("port"), 0);
    QVERIFY(!adapter.validate(definition).valid);

    adapter.execute(obsDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("recording.start")},
                    [](const IntegrationResult &) {});
    QCOMPARE(client.lastPassword, QStringLiteral("obs-secreta"));
    QCOMPARE(client.lastHost, QStringLiteral("127.0.0.1"));
    QCOMPARE(client.lastTimeoutMs, 3000);
}

void ObsIntegrationAdapterTest::mapsEveryOperationToItsObsRequest()
{
    FakeObsClient client;
    InMemorySecretStore secrets;
    secrets.store(QStringLiteral("obs/password"), QStringLiteral("x"));
    ObsIntegrationAdapter adapter(client, &secrets);
    const auto definition = obsDefinition();

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("scene.set"),
                                       .payload = {{QStringLiteral("sceneName"),
                                                    QStringLiteral("Louvor")}}},
                    [](const IntegrationResult &) {});
    QCOMPARE(client.requests.last(), QStringLiteral("SetCurrentProgramScene"));
    QCOMPARE(client.data.last().value(QStringLiteral("sceneName")).toString(),
             QStringLiteral("Louvor"));

    const QList<std::pair<QString, QString>> operations{
        {QStringLiteral("recording.start"), QStringLiteral("StartRecord")},
        {QStringLiteral("recording.stop"), QStringLiteral("StopRecord")},
        {QStringLiteral("streaming.start"), QStringLiteral("StartStream")},
        {QStringLiteral("streaming.stop"), QStringLiteral("StopStream")},
        {QStringLiteral("version.query"), QStringLiteral("GetVersion")},
    };
    for (const auto &[operation, requestType] : operations) {
        adapter.execute(definition, IntegrationRequest{.operation = operation},
                        [](const IntegrationResult &) {});
        QCOMPARE(client.requests.last(), requestType);
    }

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("input.mute.set"),
                                       .payload = {{QStringLiteral("inputName"),
                                                    QStringLiteral("Microfone")},
                                                   {QStringLiteral("muted"), true}}},
                    [](const IntegrationResult &) {});
    QCOMPARE(client.requests.last(), QStringLiteral("SetInputMute"));
    QVERIFY(client.data.last().value(QStringLiteral("inputMuted")).toBool());

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("input.trigger"),
                                       .payload = {{QStringLiteral("inputName"),
                                                    QStringLiteral("Vinheta")}}},
                    [](const IntegrationResult &) {});
    QCOMPARE(client.requests.last(), QStringLiteral("TriggerMediaInputAction"));
    QCOMPARE(client.data.last().value(QStringLiteral("mediaAction")).toString(),
             QStringLiteral("OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART"));

    // Uma conexão já identificada não é refeita a cada pedido.
    QCOMPARE(client.connections, 1);
}

void ObsIntegrationAdapterTest::reportsObsErrorsAndUnsupportedOperations()
{
    FakeObsClient client;
    InMemorySecretStore secrets;
    secrets.store(QStringLiteral("obs/password"), QStringLiteral("x"));
    ObsIntegrationAdapter adapter(client, &secrets);

    IntegrationResult result;
    adapter.execute(obsDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("scene.set")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("invalid_payload"));

    adapter.execute(obsDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("filter.toggle")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("unsupported_operation"));

    client.response = ObsResponse{.completed = true,
                                  .success = false,
                                  .errorCode = QStringLiteral("obs_error_604"),
                                  .message = QStringLiteral("Cena inexistente")};
    adapter.execute(obsDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("scene.set"),
                                       .payload = {{QStringLiteral("sceneName"),
                                                    QStringLiteral("Fantasma")}}},
                    [&result](const IntegrationResult &value) { result = value; });
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("obs_error_604"));

    client.connectSucceeds = false;
    client.currentState = QStringLiteral("disconnected");
    adapter.execute(obsDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("recording.start")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("connection_failed"));

    adapter.cancelAll();
    QCOMPARE(client.cancellations, 1);
}

void ObsIntegrationAdapterTest::retriesOnlyIdempotentOperations()
{
    FakeObsClient client;
    ObsIntegrationAdapter adapter(client);
    const auto definition = obsDefinition();

    QVERIFY(adapter.isRetriable(definition, QStringLiteral("connection.test")));
    QVERIFY(adapter.isRetriable(definition, QStringLiteral("scene.set")));
    QVERIFY(adapter.isRetriable(definition, QStringLiteral("version.query")));
    QVERIFY(!adapter.isRetriable(definition, QStringLiteral("recording.start")));
    QVERIFY(!adapter.isRetriable(definition, QStringLiteral("streaming.stop")));
}

void ObsIntegrationAdapterTest::identifiesAgainstAConformanceServerAndRunsRequests()
{
    FakeObsServer server;
    QVERIFY(server.listen());

    InMemorySecretStore secrets;
    secrets.store(QStringLiteral("obs/password"), server.password);
    ObsWebSocketClient client;
    ObsIntegrationAdapter adapter(client, &secrets);
    const auto definition = obsDefinition(server.port());

    IntegrationResult tested;
    bool finished = false;
    adapter.test(definition, [&tested, &finished](const IntegrationResult &value) {
        tested = value;
        finished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(finished, 5000);
    QVERIFY2(tested.accepted, qPrintable(tested.message));
    QCOMPARE(tested.responseMetadata.value(QStringLiteral("obsVersion")).toString(),
             QStringLiteral("30.1.2"));
    QCOMPARE(server.lastAuthentication,
             ObsAuthentication::response(server.password, server.salt, server.challenge));
    QCOMPARE(client.state(), QStringLiteral("identified"));

    IntegrationResult scene;
    bool sceneFinished = false;
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("scene.set"),
                                       .payload = {{QStringLiteral("sceneName"),
                                                    QStringLiteral("Pregação")}}},
                    [&scene, &sceneFinished](const IntegrationResult &value) {
        scene = value;
        sceneFinished = true;
    });
    QTRY_VERIFY(sceneFinished);
    QVERIFY2(scene.accepted, qPrintable(scene.message));
    QVERIFY(server.receivedRequests.contains(QStringLiteral("SetCurrentProgramScene")));
    QCOMPARE(server.receivedData.last().value(QStringLiteral("sceneName")).toString(),
             QStringLiteral("Pregação"));

    server.failNextRequest = true;
    IntegrationResult refused;
    bool refusedFinished = false;
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("recording.start")},
                    [&refused, &refusedFinished](const IntegrationResult &value) {
        refused = value;
        refusedFinished = true;
    });
    QTRY_VERIFY(refusedFinished);
    QVERIFY(!refused.accepted);
    QCOMPARE(refused.errorCode, QStringLiteral("obs_error_604"));
    QCOMPARE(refused.message, QStringLiteral("Cena inexistente"));

    client.disconnect();
}

void ObsIntegrationAdapterTest::reportsAuthenticationFailureAndRequestTimeout()
{
    FakeObsServer server;
    QVERIFY(server.listen());

    InMemorySecretStore secrets;
    secrets.store(QStringLiteral("obs/password"), QStringLiteral("senha-errada"));
    ObsWebSocketClient client;
    ObsIntegrationAdapter adapter(client, &secrets);
    auto definition = obsDefinition(server.port());
    definition.timeoutMs = 1000;

    IntegrationResult rejected;
    bool finished = false;
    adapter.test(definition, [&rejected, &finished](const IntegrationResult &value) {
        rejected = value;
        finished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(finished, 5000);
    QVERIFY(!rejected.accepted);
    // Nenhuma mensagem pode expor a senha usada.
    QVERIFY(!rejected.message.contains(QStringLiteral("senha-errada")));

    // Um pedido sem resposta precisa expirar em vez de ficar pendente.
    FakeObsServer silentServer;
    QVERIFY(silentServer.listen());
    silentServer.requireAuthentication = false;
    silentServer.ignoreNextRequest = true;
    ObsWebSocketClient silentClient;
    ObsIntegrationAdapter silentAdapter(silentClient, &secrets);
    auto silentDefinition = obsDefinition(silentServer.port());
    silentDefinition.configuration.remove(QStringLiteral("passwordReference"));
    silentDefinition.secretReferences.clear();
    silentDefinition.timeoutMs = 700;

    IntegrationResult expired;
    bool expiredFinished = false;
    silentAdapter.execute(silentDefinition,
                          IntegrationRequest{.operation = QStringLiteral("recording.start")},
                          [&expired, &expiredFinished](const IntegrationResult &value) {
        expired = value;
        expiredFinished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(expiredFinished, 5000);
    QCOMPARE(expired.errorCode, QStringLiteral("timeout"));
}

QTEST_MAIN(ObsIntegrationAdapterTest)
#include "ObsIntegrationAdapterTest.moc"
