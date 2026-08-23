#include "integrations/adapters/HttpIntegrationAdapter.h"
#include "integrations/secrets/InMemorySecretStore.h"
#include "integrations/transports/QtHttpTransport.h"

#include <QHttpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTest>

using namespace churchpresenter;

namespace {

//! Transporte falso que registra a requisição e devolve a resposta programada.
class FakeHttpTransport final : public IHttpTransport {
public:
    void send(const HttpRequest &request, Completion completion) override
    {
        lastRequest = request;
        ++sent;
        if (silent) return;
        completion(response);
    }
    void cancelAll() override { ++cancellations; }

    HttpRequest lastRequest;
    HttpResponse response{.completed = true, .status = 200};
    int sent = 0;
    int cancellations = 0;
    bool silent = false;
};

IntegrationDefinition webhook()
{
    return IntegrationDefinition{
        .id = QStringLiteral("hook"),
        .name = QStringLiteral("Webhook"),
        .type = IntegrationType::Http,
        .enabled = true,
        .configuration = {{QStringLiteral("url"), QStringLiteral("https://exemplo.local/hook")},
                          {QStringLiteral("method"), QStringLiteral("POST")}},
        .timeoutMs = 2000,
    };
}

} // namespace

class HttpIntegrationAdapterTest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsOnlyHttpAndHttpsWithSupportedMethods();
    void appliesTemplatesToUrlHeadersAndBody();
    void resolvesHeaderSecretsFromTheStoreWithoutLeakingThem();
    void reportsStatusAndAllowedHeadersOnly();
    void testUsesHeadSoItNeverTriggersTheWebhook();
    void retriesOnlyIdempotentMethods();
    void talksToARealLocalServer();
    void enforcesTheResponseSizeLimitAgainstARealServer();
};

void HttpIntegrationAdapterTest::acceptsOnlyHttpAndHttpsWithSupportedMethods()
{
    FakeHttpTransport transport;
    HttpIntegrationAdapter adapter(transport);

    QVERIFY(adapter.validate(webhook()).valid);

    auto definition = webhook();
    definition.configuration.insert(QStringLiteral("url"),
                                    QStringLiteral("ftp://exemplo.local/x"));
    QVERIFY(!adapter.validate(definition).valid);

    definition = webhook();
    definition.configuration.insert(QStringLiteral("url"), QStringLiteral("file:///etc/passwd"));
    QVERIFY(!adapter.validate(definition).valid);

    definition = webhook();
    definition.configuration.insert(QStringLiteral("method"), QStringLiteral("TRACE"));
    QVERIFY(!adapter.validate(definition).valid);

    definition = webhook();
    definition.configuration.insert(QStringLiteral("body"),
                                    QString(HttpIntegrationAdapter::MaximumBodyBytes + 1,
                                            QLatin1Char('x')));
    QVERIFY(!adapter.validate(definition).valid);

    for (const auto &method : HttpIntegrationAdapter::supportedMethods()) {
        definition = webhook();
        definition.configuration.insert(QStringLiteral("method"), method);
        QVERIFY2(adapter.validate(definition).valid, qPrintable(method));
    }
}

void HttpIntegrationAdapterTest::appliesTemplatesToUrlHeadersAndBody()
{
    FakeHttpTransport transport;
    HttpIntegrationAdapter adapter(transport);

    auto definition = webhook();
    definition.configuration.insert(QStringLiteral("url"),
                                    QStringLiteral("https://exemplo.local/{{ evento }}"));
    definition.configuration.insert(
        QStringLiteral("headers"),
        QVariantMap{{QStringLiteral("X-Slide"), QStringLiteral("{{slide}}")}});
    definition.configuration.insert(QStringLiteral("body"),
                                    QStringLiteral("{\"titulo\":\"{{titulo}}\"}"));

    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("request.send"),
                                       .payload = {{QStringLiteral("evento"),
                                                    QStringLiteral("culto")},
                                                   {QStringLiteral("slide"), 3},
                                                   {QStringLiteral("titulo"),
                                                    QStringLiteral("Louvor")}}},
                    [](const IntegrationResult &) {});

    QCOMPARE(transport.lastRequest.url, QStringLiteral("https://exemplo.local/culto"));
    QCOMPARE(transport.lastRequest.headers.value(QStringLiteral("X-Slide")).toString(),
             QStringLiteral("3"));
    QCOMPARE(transport.lastRequest.body, QByteArrayLiteral("{\"titulo\":\"Louvor\"}"));

    // Sem corpo configurado, o payload vira JSON automaticamente.
    definition.configuration.remove(QStringLiteral("body"));
    adapter.execute(definition,
                    IntegrationRequest{.operation = QStringLiteral("request.send"),
                                       .payload = {{QStringLiteral("slide"), 7}}},
                    [](const IntegrationResult &) {});
    QCOMPARE(QJsonDocument::fromJson(transport.lastRequest.body).object()
                 .value(QStringLiteral("slide")).toInt(),
             7);
    QCOMPARE(transport.lastRequest.headers.value(QStringLiteral("Content-Type")).toString(),
             QStringLiteral("application/json"));
}

void HttpIntegrationAdapterTest::resolvesHeaderSecretsFromTheStoreWithoutLeakingThem()
{
    FakeHttpTransport transport;
    InMemorySecretStore secrets;
    secrets.store(QStringLiteral("hook/authorization"), QStringLiteral("Bearer 12345"));
    HttpIntegrationAdapter adapter(transport, &secrets);

    auto definition = webhook();
    definition.secretReferences = {QStringLiteral("hook/authorization")};
    definition.configuration.insert(
        QStringLiteral("headers"),
        QVariantMap{{QStringLiteral("Authorization"), QStringLiteral("hook/authorization")}});

    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("request.send")},
                    [](const IntegrationResult &) {});

    QCOMPARE(transport.lastRequest.headers.value(QStringLiteral("Authorization")).toString(),
             QStringLiteral("Bearer 12345"));
    // A definição continua guardando apenas a referência.
    QCOMPARE(definition.configuration.value(QStringLiteral("headers")).toMap()
                 .value(QStringLiteral("Authorization")).toString(),
             QStringLiteral("hook/authorization"));
}

void HttpIntegrationAdapterTest::reportsStatusAndAllowedHeadersOnly()
{
    FakeHttpTransport transport;
    HttpIntegrationAdapter adapter(transport);
    transport.response = HttpResponse{
        .completed = true,
        .status = 202,
        .headers = {{QStringLiteral("Content-Type"), QStringLiteral("application/json")},
                    {QStringLiteral("Set-Cookie"), QStringLiteral("session=abc")},
                    {QStringLiteral("Authorization"), QStringLiteral("Bearer 12345")}},
    };

    IntegrationResult result;
    adapter.execute(webhook(), IntegrationRequest{.operation = QStringLiteral("request.send")},
                    [&result](const IntegrationResult &value) { result = value; });

    QVERIFY(result.accepted);
    QCOMPARE(result.responseMetadata.value(QStringLiteral("status")).toInt(), 202);
    QCOMPARE(result.responseMetadata.value(QStringLiteral("content-type")).toString(),
             QStringLiteral("application/json"));
    QVERIFY(!result.responseMetadata.contains(QStringLiteral("set-cookie")));
    QVERIFY(!result.responseMetadata.contains(QStringLiteral("authorization")));

    transport.response.status = 503;
    adapter.execute(webhook(), IntegrationRequest{.operation = QStringLiteral("request.send")},
                    [&result](const IntegrationResult &value) { result = value; });
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("temporarily_unavailable"));

    transport.response.status = 404;
    adapter.execute(webhook(), IntegrationRequest{.operation = QStringLiteral("request.send")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("rejected"));

    adapter.execute(webhook(), IntegrationRequest{.operation = QStringLiteral("scene.change")},
                    [&result](const IntegrationResult &value) { result = value; });
    QCOMPARE(result.errorCode, QStringLiteral("unsupported_operation"));
}

void HttpIntegrationAdapterTest::testUsesHeadSoItNeverTriggersTheWebhook()
{
    FakeHttpTransport transport;
    HttpIntegrationAdapter adapter(transport);
    auto definition = webhook();
    definition.configuration.insert(QStringLiteral("body"), QStringLiteral("disparar"));

    IntegrationResult result;
    adapter.test(definition, [&result](const IntegrationResult &value) { result = value; });

    QCOMPARE(transport.lastRequest.method, QStringLiteral("HEAD"));
    QVERIFY(transport.lastRequest.body.isEmpty());
    QVERIFY(result.accepted);
}

void HttpIntegrationAdapterTest::retriesOnlyIdempotentMethods()
{
    FakeHttpTransport transport;
    HttpIntegrationAdapter adapter(transport);

    auto definition = webhook();
    QVERIFY(!adapter.isRetriable(definition, QStringLiteral("request.send")));
    QVERIFY(adapter.isRetriable(definition, QStringLiteral("connection.test")));

    definition.configuration.insert(QStringLiteral("method"), QStringLiteral("GET"));
    QVERIFY(adapter.isRetriable(definition, QStringLiteral("request.send")));
}

void HttpIntegrationAdapterTest::talksToARealLocalServer()
{
    QHttpServer server;
    QByteArray receivedBody;
    QByteArray receivedHeader;
    int headRequests = 0;
    server.route(QStringLiteral("/hook"), QHttpServerRequest::Method::Post,
                 [&receivedBody, &receivedHeader](const QHttpServerRequest &request) {
        receivedBody = request.body();
        receivedHeader = request.value(QByteArrayLiteral("X-Origin"));
        return QHttpServerResponse(QJsonObject{{QStringLiteral("ok"), true}});
    });
    server.route(QStringLiteral("/hook"), QHttpServerRequest::Method::Head,
                 [&headRequests] {
        ++headRequests;
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Ok);
    });
    QTcpServer tcp;
    QVERIFY(tcp.listen(QHostAddress::LocalHost, 0));
    const auto port = tcp.serverPort();
    QVERIFY(server.bind(&tcp));

    QtHttpTransport transport;
    HttpIntegrationAdapter adapter(transport);
    auto definition = webhook();
    definition.configuration.insert(
        QStringLiteral("url"), QStringLiteral("http://127.0.0.1:%1/hook").arg(port));
    definition.configuration.insert(
        QStringLiteral("headers"),
        QVariantMap{{QStringLiteral("X-Origin"), QStringLiteral("HolyScreen")}});
    definition.configuration.insert(QStringLiteral("body"), QStringLiteral("{\"slide\":1}"));

    IntegrationResult result;
    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("request.send")},
                    [&result](const IntegrationResult &value) { result = value; });
    QTRY_VERIFY(result.responseMetadata.contains(QStringLiteral("status")));
    QVERIFY2(result.accepted, qPrintable(result.message));
    QCOMPARE(result.responseMetadata.value(QStringLiteral("status")).toInt(), 200);
    QCOMPARE(receivedBody, QByteArrayLiteral("{\"slide\":1}"));
    QCOMPARE(receivedHeader, QByteArrayLiteral("HolyScreen"));

    IntegrationResult tested;
    adapter.test(definition, [&tested](const IntegrationResult &value) { tested = value; });
    QTRY_VERIFY(tested.accepted);
    QCOMPARE(headRequests, 1);

    // Porta fechada devolve falha transitória, não travamento.
    definition.configuration.insert(QStringLiteral("url"),
                                    QStringLiteral("http://127.0.0.1:1/hook"));
    IntegrationResult refused;
    adapter.execute(definition, IntegrationRequest{.operation = QStringLiteral("request.send")},
                    [&refused](const IntegrationResult &value) { refused = value; });
    QTRY_VERIFY(!refused.errorCode.isEmpty());
    QVERIFY(!refused.accepted);
}

void HttpIntegrationAdapterTest::enforcesTheResponseSizeLimitAgainstARealServer()
{
    QHttpServer server;
    server.route(QStringLiteral("/big"), [] {
        return QHttpServerResponse("text/plain", QByteArray(64 * 1024, 'x'));
    });
    QTcpServer tcp;
    QVERIFY(tcp.listen(QHostAddress::LocalHost, 0));
    const auto port = tcp.serverPort();
    QVERIFY(server.bind(&tcp));

    QtHttpTransport transport;
    HttpRequest request{
        .method = QStringLiteral("GET"),
        .url = QStringLiteral("http://127.0.0.1:%1/big").arg(port),
        .timeoutMs = 3000,
        .maximumResponseBytes = 1024,
    };
    HttpResponse response;
    bool finished = false;
    transport.send(request, [&response, &finished](const HttpResponse &value) {
        response = value;
        finished = true;
    });
    QTRY_VERIFY(finished);
    QVERIFY(response.body.size() <= 1024);
}

QTEST_MAIN(HttpIntegrationAdapterTest)
#include "HttpIntegrationAdapterTest.moc"
