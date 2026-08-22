#include "remote/LocalApiServer.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTest>
#include <QWebSocket>

using namespace churchpresenter;

namespace {

struct HttpResult {
    int status = 0;
    QByteArray body;
};

HttpResult request(QNetworkAccessManager &network, const QUrl &url,
                   const QByteArray &method = QByteArrayLiteral("GET"),
                   const QByteArray &body = {}, const QString &token = {},
                   const QByteArray &origin = {})
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!token.isEmpty()) request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    if (!origin.isEmpty()) request.setRawHeader("Origin", origin);
    QNetworkReply *reply = method == QByteArrayLiteral("GET") ? network.get(request)
        : method == QByteArrayLiteral("POST") ? network.post(request, body)
        : network.sendCustomRequest(request, method, body);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const HttpResult result{
        .status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
        .body = reply->readAll(),
    };
    reply->deleteLater();
    return result;
}

QJsonObject object(const QByteArray &json)
{
    return QJsonDocument::fromJson(json).object();
}

} // namespace

class LocalApiServerTest final : public QObject {
    Q_OBJECT
private slots:
    void servesAuthenticatedVersionedApiAndRejectsLegacyRoutes();
    void enforcesOriginPayloadAndCommandAllowlist();
    void enforcesHttpBruteForceAndCommandRateLimit();
    void startsOnAnAvailableLocalPortAndServesTheRemotePage();
    void authenticatesWebSocketBeforePublishingStateAndCommands();
    void reportsWhyAPortCannotBeOpened();
};

void LocalApiServerTest::servesAuthenticatedVersionedApiAndRejectsLegacyRoutes()
{
    LocalApiServer server;
    RemoteAuthManager credentials;
    QVERIFY(server.loadCredentials(credentials.setPassword(QStringLiteral("senha"), 1000)));
    server.setStateProvider([] { return QJsonObject{{QStringLiteral("slideIndex"), 2}}; });
    server.setCommandDispatcher([](const Command &command) {
        return CommandResult{.accepted = command.type == QStringLiteral("presentation.slide.next"),
                             .message = QStringLiteral("ok"), .stateRevision = 7};
    });
    QVERIFY(server.start(0, QHostAddress::LocalHost));
    const auto base = QStringLiteral("http://127.0.0.1:%1").arg(server.port());
    QNetworkAccessManager network;

    QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/state"))).status, 404);
    QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/v1/state"))).status, 401);
    const auto login = request(network, QUrl(base + QStringLiteral("/api/v1/session")), "POST",
                               R"({"password":"senha"})");
    QCOMPARE(login.status, 201);
    const auto token = object(login.body).value(QStringLiteral("token")).toString();
    QVERIFY(!token.isEmpty());
    const auto state = request(network, QUrl(base + QStringLiteral("/api/v1/state")),
                               "GET", {}, token);
    QCOMPARE(state.status, 200);
    QCOMPARE(object(state.body).value(QStringLiteral("slideIndex")).toInt(), 2);
    const auto command = request(network, QUrl(base + QStringLiteral("/api/v1/commands")),
                                 "POST", R"({"id":"cmd-1","type":"presentation.slide.next","payload":{}})", token);
    QCOMPARE(command.status, 200);
    QCOMPARE(object(command.body).value(QStringLiteral("stateRevision")).toInt(), 7);
    QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/v1/session")),
                     "DELETE", {}, token).status, 204);
    QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/v1/state")),
                     "GET", {}, token).status, 401);
}

void LocalApiServerTest::enforcesOriginPayloadAndCommandAllowlist()
{
    LocalApiServer server;
    RemoteAuthManager credentials;
    QVERIFY(server.loadCredentials(credentials.setPassword(QStringLiteral("senha"), 1000)));
    server.setCommandDispatcher([](const Command &) {
        return CommandResult{.accepted = true, .stateRevision = 1};
    });
    QVERIFY(server.start(0, QHostAddress::LocalHost));
    const auto base = QStringLiteral("http://127.0.0.1:%1").arg(server.port());
    QNetworkAccessManager network;
    QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/v1/session")), "POST",
                     R"({"password":"senha"})", {}, "http://evil.invalid").status, 403);
    QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/v1/session")), "POST",
                     R"({"password":"senha"})", {},
                     QStringLiteral("https://127.0.0.1:%1").arg(server.port()).toUtf8()).status,
             403);
    const auto login = request(network, QUrl(base + QStringLiteral("/api/v1/session")), "POST",
                               R"({"password":"senha"})");
    const auto token = object(login.body).value(QStringLiteral("token")).toString();
    const auto unknown = request(network, QUrl(base + QStringLiteral("/api/v1/commands")), "POST",
                                 R"({"id":"cmd-2","type":"system.shutdown","payload":{}})", token);
    QCOMPARE(unknown.status, 403);
    QCOMPARE(object(unknown.body).value(QStringLiteral("errorCode")).toString(),
             QStringLiteral("unknown_command"));
    const QByteArray oversized(LocalApiServer::MaximumPayloadBytes + 1, 'x');
    QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/v1/commands")),
                     "POST", oversized, token).status, 413);
}

void LocalApiServerTest::enforcesHttpBruteForceAndCommandRateLimit()
{
    {
        LocalApiServer server;
        RemoteAuthManager credentials;
        QVERIFY(server.loadCredentials(credentials.setPassword(QStringLiteral("senha"), 1000)));
        QVERIFY(server.start(0, QHostAddress::LocalHost));
        const auto sessionUrl = QUrl(QStringLiteral("http://127.0.0.1:%1/api/v1/session")
                                         .arg(server.port()));
        QNetworkAccessManager network;
        for (int attempt = 0; attempt < 5; ++attempt)
            QCOMPARE(request(network, sessionUrl, "POST", R"({"password":"errada"})").status,
                     401);
        const auto blocked = request(network, sessionUrl, "POST", R"({"password":"senha"})");
        QCOMPARE(blocked.status, 429);
        QCOMPARE(object(blocked.body).value(QStringLiteral("errorCode")).toString(),
                 QStringLiteral("login_blocked"));
    }

    LocalApiServer server;
    RemoteAuthManager credentials;
    QVERIFY(server.loadCredentials(credentials.setPassword(QStringLiteral("senha"), 1000)));
    server.setCommandDispatcher([](const Command &) {
        return CommandResult{.accepted = true, .stateRevision = 1};
    });
    QVERIFY(server.start(0, QHostAddress::LocalHost));
    const auto base = QStringLiteral("http://127.0.0.1:%1").arg(server.port());
    QNetworkAccessManager network;
    const auto login = request(network, QUrl(base + QStringLiteral("/api/v1/session")),
                               "POST", R"({"password":"senha"})");
    const auto token = object(login.body).value(QStringLiteral("token")).toString();
    QVERIFY(!token.isEmpty());
    for (int command = 0; command < 30; ++command) {
        const auto body = QStringLiteral(
            R"({"id":"rate-%1","type":"presentation.slide.next","payload":{}})")
                              .arg(command).toUtf8();
        QCOMPARE(request(network, QUrl(base + QStringLiteral("/api/v1/commands")),
                         "POST", body, token).status,
                 200);
    }
    const auto limited = request(
        network, QUrl(base + QStringLiteral("/api/v1/commands")), "POST",
        R"({"id":"rate-31","type":"presentation.slide.next","payload":{}})", token);
    QCOMPARE(limited.status, 429);
    QCOMPARE(object(limited.body).value(QStringLiteral("errorCode")).toString(),
             QStringLiteral("rate_limited"));
}

void LocalApiServerTest::startsOnAnAvailableLocalPortAndServesTheRemotePage()
{
    LocalApiServer server;
    QVERIFY(server.start(0, QHostAddress::LocalHost));
    QVERIFY(server.running());
    QVERIFY(server.port() > 0);
    QVERIFY(server.remoteUrl().startsWith(QStringLiteral("http://")));
    QVERIFY(LocalApiServer::remotePage().contains("/api/v1/session"));
    const auto openApi = LocalApiServer::openApiDocument();
    const auto paths = openApi.value(QStringLiteral("paths")).toObject();
    const auto session = paths.value(QStringLiteral("/api/v1/session")).toObject();
    QVERIFY(session.contains(QStringLiteral("post")));
    QVERIFY(session.contains(QStringLiteral("delete")));
    const auto commands = paths.value(QStringLiteral("/api/v1/commands")).toObject()
                              .value(QStringLiteral("post")).toObject();
    QVERIFY(commands.value(QStringLiteral("security")).isArray());
    const auto components = openApi.value(QStringLiteral("components")).toObject();
    const auto bearer = components.value(QStringLiteral("securitySchemes")).toObject()
                            .value(QStringLiteral("bearerAuth")).toObject();
    QCOMPARE(bearer.value(QStringLiteral("type")).toString(), QStringLiteral("http"));
    QCOMPARE(bearer.value(QStringLiteral("scheme")).toString(), QStringLiteral("bearer"));
    const auto commandSchema = components.value(QStringLiteral("schemas")).toObject()
                                   .value(QStringLiteral("CommandRequest")).toObject();
    QCOMPARE(commandSchema.value(QStringLiteral("required")).toArray().size(), 3);
    QVERIFY(paths.value(QStringLiteral("/api/v1/ws")).toObject()
                .value(QStringLiteral("get")).toObject()
                .value(QStringLiteral("x-websocket")).toBool());
    server.stop();
    QVERIFY(!server.running());
}

void LocalApiServerTest::authenticatesWebSocketBeforePublishingStateAndCommands()
{
    LocalApiServer server;
    RemoteAuthManager credentials;
    QVERIFY(server.loadCredentials(credentials.setPassword(QStringLiteral("senha"), 1000)));
    server.setStateProvider([] { return QJsonObject{{QStringLiteral("slideIndex"), 4}}; });
    server.setCommandDispatcher([](const Command &command) {
        return CommandResult{.accepted = command.type == QStringLiteral("presentation.slide.next"),
                             .stateRevision = 9};
    });
    QVERIFY(server.start(0, QHostAddress::LocalHost));
    const auto httpBase = QStringLiteral("http://127.0.0.1:%1").arg(server.port());
    QNetworkAccessManager network;
    const auto login = request(network, QUrl(httpBase + QStringLiteral("/api/v1/session")),
                               "POST", R"({"password":"senha"})");
    const auto token = object(login.body).value(QStringLiteral("token")).toString();

    QWebSocket socket;
    QSignalSpy connected(&socket, &QWebSocket::connected);
    QSignalSpy messages(&socket, &QWebSocket::textMessageReceived);
    socket.open(QUrl(QStringLiteral("ws://127.0.0.1:%1/api/v1/ws").arg(server.port())));
    QTRY_COMPARE_WITH_TIMEOUT(connected.count(), 1, 2000);
    socket.sendTextMessage(QString::fromUtf8(QJsonDocument(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("authenticate")},
        {QStringLiteral("token"), token},
    }).toJson(QJsonDocument::Compact)));
    QTRY_VERIFY_WITH_TIMEOUT(messages.count() >= 2, 2000);
    QCOMPARE(server.clientCount(), 1);
    socket.sendTextMessage(QStringLiteral(
        R"({"type":"command","command":{"id":"ws-1","type":"presentation.slide.next","payload":{}}})"));
    QTRY_VERIFY_WITH_TIMEOUT(messages.count() >= 3, 2000);
    bool receivedResult = false;
    for (const auto &message : messages) {
        const auto response = QJsonDocument::fromJson(message.front().toString().toUtf8()).object();
        if (response.value(QStringLiteral("type")).toString() == QStringLiteral("commandResult")) {
            receivedResult = response.value(QStringLiteral("accepted")).toBool()
                && response.value(QStringLiteral("stateRevision")).toInt() == 9;
        }
    }
    QVERIFY(receivedResult);

    QWebSocket unauthenticated;
    QSignalSpy unauthorizedConnected(&unauthenticated, &QWebSocket::connected);
    QSignalSpy disconnected(&unauthenticated, &QWebSocket::disconnected);
    unauthenticated.open(QUrl(QStringLiteral("ws://127.0.0.1:%1/api/v1/ws").arg(server.port())));
    QTRY_COMPARE_WITH_TIMEOUT(unauthorizedConnected.count(), 1, 2000);
    unauthenticated.sendTextMessage(QStringLiteral(
        R"({"id":"ws-2","type":"presentation.slide.next","payload":{}})"));
    QTRY_COMPARE_WITH_TIMEOUT(disconnected.count(), 1, 2000);
    socket.close();
}

void LocalApiServerTest::reportsWhyAPortCannotBeOpened()
{
    QTcpServer occupiedPort;
    QVERIFY(occupiedPort.listen(QHostAddress::LocalHost, 0));
    LocalApiServer server;
    QVERIFY(!server.start(occupiedPort.serverPort(), QHostAddress::LocalHost));
    QVERIFY2(!server.lastError().isEmpty(), "A falha de bind deve expor um diagnóstico acionável");
    QVERIFY(server.lastError().contains(QString::number(occupiedPort.serverPort())));
}

QTEST_MAIN(LocalApiServerTest)
#include "LocalApiServerTest.moc"
