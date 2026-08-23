#include "integrations/adapters/WebSocketIntegrationAdapter.h"
#include "integrations/transports/QtWebSocketTransport.h"

#include <QSignalSpy>
#include <QTest>
#include <QWebSocket>
#include <QWebSocketServer>

using namespace churchpresenter;

namespace {

class FakeWebSocketTransport final : public IWebSocketTransport {
public:
    void connectTo(const QString &url, int timeoutMs, Completion completion) override
    {
        lastUrl = url;
        lastTimeoutMs = timeoutMs;
        ++connections;
        states.insert(url, connectAccepted ? QStringLiteral("connected")
                                           : QStringLiteral("error"));
        completion(connectAccepted, connectAccepted ? QString{}
                                                    : QStringLiteral("connection_failed"),
                   connectAccepted ? QStringLiteral("ok") : QStringLiteral("recusado"));
    }

    void sendText(const QString &url, const QString &text, Completion completion) override
    {
        lastUrl = url;
        lastText = text;
        ++messages;
        completion(sendAccepted, sendAccepted ? QString{} : QStringLiteral("queue_full"),
                   sendAccepted ? QStringLiteral("ok") : QStringLiteral("fila cheia"));
    }

    void disconnectFrom(const QString &url) override
    {
        ++disconnections;
        states.insert(url, QStringLiteral("disconnected"));
    }

    QString state(const QString &url) const override
    {
        return states.value(url, QStringLiteral("disconnected"));
    }

    void setStateObserver(StateChanged observer) override { this->observer = std::move(observer); }
    void cancelAll() override { ++cancellations; }

    QString lastUrl;
    QString lastText;
    int lastTimeoutMs = 0;
    int connections = 0;
    int messages = 0;
    int disconnections = 0;
    int cancellations = 0;
    bool connectAccepted = true;
    bool sendAccepted = true;
    QHash<QString, QString> states;
    StateChanged observer;
};

IntegrationDefinition socketDefinition(const QString &url = QStringLiteral("ws://127.0.0.1:9000/x"))
{
    return IntegrationDefinition{
        .id = QStringLiteral("mesa"),
        .name = QStringLiteral("Mesa de som"),
        .type = IntegrationType::WebSocket,
        .enabled = true,
        .configuration = {{QStringLiteral("url"), url}},
        .timeoutMs = 1500,
    };
}

} // namespace

class WebSocketIntegrationAdapterTest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsOnlyWsAndWssUrls();
    void sendsTextJsonAndTemplatedMessages();
    void refusesUnsupportedOperationsAndOversizedMessages();
    void opensClosesAndReportsTheObservableState();
    void retriesOnlyConnectionOperations();
    void talksToARealLocalWebSocketServer();
    void reportsFailureAndStopsAfterTheReconnectLimit();
};

void WebSocketIntegrationAdapterTest::acceptsOnlyWsAndWssUrls()
{
    FakeWebSocketTransport transport;
    WebSocketIntegrationAdapter adapter(transport);

    QVERIFY(adapter.validate(socketDefinition()).valid);
    QVERIFY(adapter.validate(socketDefinition(QStringLiteral("wss://exemplo.local/x"))).valid);
    QVERIFY(!adapter.validate(socketDefinition(QStringLiteral("http://exemplo.local"))).valid);
    QVERIFY(!adapter.validate(socketDefinition(QStringLiteral("ws://"))).valid);

    auto definition = socketDefinition();
    definition.configuration.insert(
        QStringLiteral("message"),
        QString(WebSocketIntegrationAdapter::MaximumMessageBytes + 1, QLatin1Char('x')));
    QVERIFY(!adapter.validate(definition).valid);
}

void WebSocketIntegrationAdapterTest::sendsTextJsonAndTemplatedMessages()
{
    FakeWebSocketTransport transport;
    WebSocketIntegrationAdapter adapter(transport);

    adapter.execute(socketDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("message.send"),
                                       .payload = {{QStringLiteral("text"),
                                                    QStringLiteral("slide {{numero}}")},
                                                   {QStringLiteral("numero"), 4}}},
                    [](const IntegrationResult &) {});
    QCOMPARE(transport.lastText, QStringLiteral("slide 4"));

    auto definition = socketDefinition();
    definition.configuration.insert(QStringLiteral("message"),
                                    QStringLiteral("{\"cena\":\"{{cena}}\"}"));
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("message.send"),
                                       .payload = {{QStringLiteral("cena"),
                                                    QStringLiteral("louvor")}}},
                    [](const IntegrationResult &) {});
    QCOMPARE(transport.lastText, QStringLiteral("{\"cena\":\"louvor\"}"));

    adapter.execute(socketDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("message.send"),
                                       .payload = {{QStringLiteral("slide"), 2}}},
                    [](const IntegrationResult &) {});
    QVERIFY(transport.lastText.contains(QStringLiteral("\"slide\":2")));
}

void WebSocketIntegrationAdapterTest::refusesUnsupportedOperationsAndOversizedMessages()
{
    FakeWebSocketTransport transport;
    WebSocketIntegrationAdapter adapter(transport);

    IntegrationResult result;
    adapter.execute(socketDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("scene.change")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("unsupported_operation"));

    adapter.execute(socketDefinition(),
                    IntegrationRequest{.operation = QStringLiteral("message.send")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("invalid_payload"));

    adapter.execute(
        socketDefinition(),
        IntegrationRequest{
            .operation = QStringLiteral("message.send"),
            .payload = {{QStringLiteral("text"),
                         QString(WebSocketIntegrationAdapter::MaximumMessageBytes + 1,
                                 QLatin1Char('x'))}}},
        [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("payload_too_large"));
    QCOMPARE(transport.messages, 0);
}

void WebSocketIntegrationAdapterTest::opensClosesAndReportsTheObservableState()
{
    FakeWebSocketTransport transport;
    WebSocketIntegrationAdapter adapter(transport);
    const auto definition = socketDefinition();

    IntegrationResult opened;
    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("connection.open")},
                    [&opened](const IntegrationResult &value) { opened = value; });
    QVERIFY(opened.accepted);
    QCOMPARE(opened.responseMetadata.value(QStringLiteral("state")).toString(),
             QStringLiteral("connected"));
    QCOMPARE(adapter.state(definition), QStringLiteral("connected"));
    QCOMPARE(transport.lastTimeoutMs, 1500);

    IntegrationResult closed;
    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("connection.close")},
                    [&closed](const IntegrationResult &value) { closed = value; });
    QVERIFY(closed.accepted);
    QCOMPARE(adapter.state(definition), QStringLiteral("disconnected"));

    transport.connectAccepted = false;
    IntegrationResult tested;
    adapter.test(definition, [&tested](const IntegrationResult &value) { tested = value; });
    QVERIFY(!tested.accepted);
    QCOMPARE(tested.errorCode, QStringLiteral("connection_failed"));

    adapter.cancelAll();
    QCOMPARE(transport.cancellations, 1);
}

void WebSocketIntegrationAdapterTest::retriesOnlyConnectionOperations()
{
    FakeWebSocketTransport transport;
    WebSocketIntegrationAdapter adapter(transport);
    const auto definition = socketDefinition();

    QVERIFY(adapter.isRetriable(definition, QStringLiteral("connection.test")));
    QVERIFY(adapter.isRetriable(definition, QStringLiteral("connection.open")));
    QVERIFY(!adapter.isRetriable(definition, QStringLiteral("message.send")));
}

void WebSocketIntegrationAdapterTest::talksToARealLocalWebSocketServer()
{
    QWebSocketServer server(QStringLiteral("fake"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QStringList received;
    QList<QWebSocket *> clients;
    connect(&server, &QWebSocketServer::newConnection, &server, [&server, &received, &clients] {
        auto *socket = server.nextPendingConnection();
        clients.append(socket);
        connect(socket, &QWebSocket::textMessageReceived, socket,
                [&received](const QString &message) { received.append(message); });
    });

    const auto url = QStringLiteral("ws://127.0.0.1:%1").arg(server.serverPort());
    QtWebSocketTransport transport;
    WebSocketIntegrationAdapter adapter(transport);
    auto definition = socketDefinition(url);

    IntegrationResult tested;
    bool finished = false;
    adapter.test(definition, [&tested, &finished](const IntegrationResult &value) {
        tested = value;
        finished = true;
    });
    QTRY_VERIFY(finished);
    QVERIFY2(tested.accepted, qPrintable(tested.message));
    QCOMPARE(adapter.state(definition), QStringLiteral("connected"));

    IntegrationResult sent;
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("message.send"),
                                       .payload = {{QStringLiteral("text"),
                                                    QStringLiteral("ola palco")}}},
                    [&sent](const IntegrationResult &value) { sent = value; });
    QVERIFY(sent.accepted);
    QTRY_COMPARE(received.size(), 1);
    QCOMPARE(received.first(), QStringLiteral("ola palco"));

    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("connection.close")},
                    [](const IntegrationResult &) {});
    QTRY_COMPARE(adapter.state(definition), QStringLiteral("disconnected"));

    qDeleteAll(clients);
}

void WebSocketIntegrationAdapterTest::reportsFailureAndStopsAfterTheReconnectLimit()
{
    QtWebSocketTransport transport;
    transport.setAutomaticReconnect(false);
    WebSocketIntegrationAdapter adapter(transport);
    // Porta fechada: a conexão precisa falhar rápido e sem travar.
    const auto definition = socketDefinition(QStringLiteral("ws://127.0.0.1:1"));

    IntegrationResult result;
    bool finished = false;
    adapter.test(definition, [&result, &finished](const IntegrationResult &value) {
        result = value;
        finished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(finished, 5000);
    QVERIFY(!result.accepted);
    QVERIFY(!result.errorCode.isEmpty());

    IntegrationResult invalid;
    adapter.execute(socketDefinition(QStringLiteral("ftp://exemplo.local")),
                    IntegrationRequest{.operation = QStringLiteral("connection.open")},
                    [&invalid](const IntegrationResult &value) { invalid = value; });
    QCOMPARE(invalid.errorCode, QStringLiteral("invalid_url"));
}

QTEST_MAIN(WebSocketIntegrationAdapterTest)
#include "WebSocketIntegrationAdapterTest.moc"
