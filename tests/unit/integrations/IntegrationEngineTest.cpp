#include "integrations/IntegrationEngine.h"
#include "integrations/IntegrationSanitizer.h"

#include <QSignalSpy>
#include <QTest>

#include <algorithm>

using namespace churchpresenter;

namespace {

//! Adapter falso: responde imediatamente com o resultado programado, ou não
//! responde nada quando `silent` é verdadeiro, para exercitar o timeout.
class FakeAdapter final : public IIntegrationAdapter {
public:
    IntegrationValidation validate(const IntegrationDefinition &definition) const override
    {
        if (definition.configuration.value(QStringLiteral("url")).toString().isEmpty()) {
            return IntegrationValidation::failure(QStringLiteral("url é obrigatória."));
        }
        return IntegrationValidation::ok();
    }

    void test(const IntegrationDefinition &, Completion completion) override
    {
        ++tests;
        if (silent) return;
        completion(testResult);
    }

    void execute(const IntegrationDefinition &, const IntegrationRequest &request,
                 Completion completion) override
    {
        ++executions;
        lastOperation = request.operation;
        if (silent) return;
        completion(results.isEmpty() ? defaultResult : results.takeFirst());
    }

    void cancelAll() override { ++cancellations; }

    bool isRetriable(const IntegrationDefinition &, const QString &operation) const override
    {
        return retriableOperations.contains(operation);
    }

    int tests = 0;
    int executions = 0;
    int cancellations = 0;
    bool silent = false;
    QString lastOperation;
    QStringList retriableOperations;
    QList<IntegrationResult> results;
    IntegrationResult defaultResult{.accepted = true, .message = QStringLiteral("ok")};
    IntegrationResult testResult{.accepted = true, .message = QStringLiteral("conectado")};
};

class MemoryRepository final : public IIntegrationRepository {
public:
    QVector<IntegrationDefinition> definitions() const override { return stored; }

    bool save(const IntegrationDefinition &definition) override
    {
        for (auto &existing : stored) {
            if (existing.id == definition.id) {
                existing = definition;
                return true;
            }
        }
        stored.append(definition);
        return true;
    }

    bool remove(const QString &integrationId) override
    {
        const auto removed = stored.removeIf([&](const IntegrationDefinition &definition) {
            return definition.id == integrationId;
        });
        return removed > 0;
    }

    bool recordCall(const IntegrationCall &call) override
    {
        calls.append(call);
        return true;
    }

    QVector<IntegrationCall> history(const QString &integrationId, int limit) const override
    {
        QVector<IntegrationCall> filtered;
        for (const auto &call : calls) {
            if (integrationId.isEmpty() || call.integrationId == integrationId) {
                filtered.append(call);
            }
        }
        if (filtered.size() > limit) filtered = filtered.mid(filtered.size() - limit);
        return filtered;
    }

    int pruneHistory(int maximumEntriesPerIntegration) override
    {
        const auto before = calls.size();
        if (calls.size() > maximumEntriesPerIntegration) {
            calls = calls.mid(calls.size() - maximumEntriesPerIntegration);
        }
        return static_cast<int>(before - calls.size());
    }

    QVector<IntegrationDefinition> stored;
    QVector<IntegrationCall> calls;
};

class MemorySecretStore final : public ISecretStore {
public:
    bool store(const QString &reference, const QString &secret) override
    {
        secrets.insert(reference, secret);
        return true;
    }
    std::optional<QString> retrieve(const QString &reference) const override
    {
        const auto found = secrets.constFind(reference);
        if (found == secrets.cend()) return std::nullopt;
        return *found;
    }
    bool remove(const QString &reference) override { return secrets.remove(reference) > 0; }
    QStringList references() const override { return secrets.keys(); }
    bool isPersistent() const override { return false; }
    QString backendName() const override { return QStringLiteral("memória"); }

    QHash<QString, QString> secrets;
};

IntegrationDefinition httpDefinition()
{
    return IntegrationDefinition{
        .id = QStringLiteral("obs-http"),
        .name = QStringLiteral("Servidor da igreja"),
        .type = IntegrationType::Http,
        .enabled = true,
        .configuration = {{QStringLiteral("url"), QStringLiteral("https://exemplo.local/hook")}},
        .secretReferences = {},
        .timeoutMs = 1000,
        .retryPolicy = {.maximumAttempts = 1, .backoffMs = 0},
    };
}

} // namespace

class IntegrationEngineTest final : public QObject {
    Q_OBJECT

private slots:
    void validatesBeforePersisting();
    void refusesSecretsStoredInTheConfiguration();
    void selectsTheAdapterByTypeAndPublishesASanitizedResult();
    void retriesOnlyTransientFailuresOfSafeOperations();
    void failsWithTimeoutWhenTheAdapterNeverAnswers();
    void cancelsPendingCallsOnShutdown();
    void refusesDisabledIntegrationsButStillTestsTheConnection();
    void keepsHistoryWithinTheConfiguredRetention();
    void removesSecretsFromExportedDefinitions();
};

void IntegrationEngineTest::validatesBeforePersisting()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);

    auto definition = httpDefinition();
    definition.configuration.remove(QStringLiteral("url"));
    const auto invalid = engine.save(definition);
    QVERIFY(!invalid.valid);
    QVERIFY(invalid.errors.first().contains(QStringLiteral("url")));
    QVERIFY(repository.stored.isEmpty());

    definition = httpDefinition();
    definition.timeoutMs = 10;
    QVERIFY(!engine.save(definition).valid);

    definition = httpDefinition();
    definition.retryPolicy.maximumAttempts = 99;
    QVERIFY(!engine.save(definition).valid);

    definition = httpDefinition();
    definition.type = IntegrationType::Midi;
    const auto missingAdapter = engine.save(definition);
    QVERIFY(!missingAdapter.valid);
    QVERIFY(missingAdapter.errors.first().contains(QStringLiteral("midi")));

    QVERIFY(engine.save(httpDefinition()).valid);
    QCOMPARE(repository.stored.size(), 1);
    QCOMPARE(engine.definitions().size(), 1);
}

void IntegrationEngineTest::refusesSecretsStoredInTheConfiguration()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);

    auto definition = httpDefinition();
    definition.configuration.insert(QStringLiteral("authorizationHeader"),
                                    QStringLiteral("Bearer 12345"));
    const auto validation = engine.save(definition);
    QVERIFY(!validation.valid);
    QVERIFY(repository.stored.isEmpty());

    definition.configuration.insert(QStringLiteral("authorizationHeader"),
                                    QStringLiteral("obs-http/authorization"));
    definition.secretReferences = {QStringLiteral("obs-http/authorization")};
    QVERIFY(engine.save(definition).valid);
}

void IntegrationEngineTest::selectsTheAdapterByTypeAndPublishesASanitizedResult()
{
    qRegisterMetaType<IntegrationRequest>();
    qRegisterMetaType<IntegrationResult>();

    IntegrationEngine engine;
    MemoryRepository repository;
    MemorySecretStore secrets;
    FakeAdapter http;
    FakeAdapter midi;
    engine.registerAdapter(IntegrationType::Http, &http);
    engine.registerAdapter(IntegrationType::Midi, &midi);
    engine.setRepository(&repository);
    engine.setSecretStore(&secrets);
    secrets.store(QStringLiteral("obs-http/authorization"), QStringLiteral("super-secreto"));

    auto definition = httpDefinition();
    definition.secretReferences = {QStringLiteral("obs-http/authorization")};
    QVERIFY(engine.save(definition).valid);

    http.defaultResult = IntegrationResult{
        .accepted = true,
        .message = QStringLiteral("enviado com super-secreto"),
        .durationMs = 12,
        .responseMetadata = {{QStringLiteral("status"), 200},
                             {QStringLiteral("authorization"), QStringLiteral("super-secreto")},
                             {QStringLiteral("location"),
                              QStringLiteral("https://user:super-secreto@exemplo.local/x")}},
    };

    QSignalSpy spy(&engine, &IntegrationEngine::callFinished);
    IntegrationResult received;
    engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                      .operation = QStringLiteral("request.send")},
                   [&received](const IntegrationResult &result) { received = result; });

    QCOMPARE(http.executions, 1);
    QCOMPARE(midi.executions, 0);
    QVERIFY(received.accepted);
    QCOMPARE(received.message, QStringLiteral("enviado com ***"));
    QCOMPARE(received.responseMetadata.value(QStringLiteral("status")).toInt(), 200);
    QCOMPARE(received.responseMetadata.value(QStringLiteral("authorization")).toString(),
             QStringLiteral("***"));
    QVERIFY(!received.responseMetadata.value(QStringLiteral("location")).toString()
                 .contains(QStringLiteral("super-secreto")));
    QCOMPARE(spy.count(), 1);

    QCOMPARE(repository.calls.size(), 1);
    QCOMPARE(repository.calls.first().integrationId, QStringLiteral("obs-http"));
    QCOMPARE(repository.calls.first().operation, QStringLiteral("request.send"));
    QVERIFY(!repository.calls.first().message.contains(QStringLiteral("super-secreto")));
}

void IntegrationEngineTest::retriesOnlyTransientFailuresOfSafeOperations()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);

    auto definition = httpDefinition();
    definition.retryPolicy = {.maximumAttempts = 3, .backoffMs = 0};
    QVERIFY(engine.save(definition).valid);

    // Operação insegura: nunca repete, mesmo com falha transitória.
    adapter.results = {IntegrationResult{.accepted = false,
                                         .errorCode = QStringLiteral("timeout")}};
    engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                      .operation = QStringLiteral("request.send")});
    QCOMPARE(adapter.executions, 1);

    // Operação segura e falha transitória: repete até obter sucesso.
    adapter.executions = 0;
    adapter.retriableOperations = {QStringLiteral("scene.query")};
    adapter.results = {
        IntegrationResult{.accepted = false, .errorCode = QStringLiteral("connection_failed")},
        IntegrationResult{.accepted = true, .message = QStringLiteral("ok")},
    };
    IntegrationResult received;
    engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                      .operation = QStringLiteral("scene.query")},
                   [&received](const IntegrationResult &result) { received = result; });
    QTRY_COMPARE(received.accepted, true);
    QCOMPARE(adapter.executions, 2);
    QCOMPARE(received.attempts, 2);

    // Falha permanente não é repetida.
    adapter.executions = 0;
    adapter.results = {IntegrationResult{.accepted = false,
                                         .errorCode = QStringLiteral("invalid_payload")}};
    engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                      .operation = QStringLiteral("scene.query")});
    QCOMPARE(adapter.executions, 1);
}

void IntegrationEngineTest::failsWithTimeoutWhenTheAdapterNeverAnswers()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    adapter.silent = true;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);

    auto definition = httpDefinition();
    definition.timeoutMs = IntegrationEngine::MinimumTimeoutMs;
    QVERIFY(engine.save(definition).valid);

    IntegrationResult received;
    engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                      .operation = QStringLiteral("request.send")},
                   [&received](const IntegrationResult &result) { received = result; });

    QTRY_VERIFY_WITH_TIMEOUT(!received.errorCode.isEmpty(), 5000);
    QCOMPARE(received.errorCode, QStringLiteral("timeout"));
    QVERIFY(!received.accepted);
    QCOMPARE(repository.calls.size(), 1);
}

void IntegrationEngineTest::cancelsPendingCallsOnShutdown()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    adapter.silent = true;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);
    QVERIFY(engine.save(httpDefinition()).valid);

    IntegrationResult received;
    engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                      .operation = QStringLiteral("request.send")},
                   [&received](const IntegrationResult &result) { received = result; });
    QVERIFY(received.errorCode.isEmpty());

    engine.cancelAll();
    QCOMPARE(adapter.cancellations, 1);
    QCOMPARE(received.errorCode, QStringLiteral("cancelled"));
}

void IntegrationEngineTest::refusesDisabledIntegrationsButStillTestsTheConnection()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);

    auto definition = httpDefinition();
    definition.enabled = false;
    QVERIFY(engine.save(definition).valid);

    IntegrationResult executed;
    engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                      .operation = QStringLiteral("request.send")},
                   [&executed](const IntegrationResult &result) { executed = result; });
    QCOMPARE(executed.errorCode, QStringLiteral("integration_disabled"));
    QCOMPARE(adapter.executions, 0);

    IntegrationResult tested;
    engine.test(QStringLiteral("obs-http"),
                [&tested](const IntegrationResult &result) { tested = result; });
    QVERIFY(tested.accepted);
    QCOMPARE(adapter.tests, 1);
    QCOMPARE(adapter.executions, 0);

    IntegrationResult unknown;
    engine.test(QStringLiteral("nao-existe"),
                [&unknown](const IntegrationResult &result) { unknown = result; });
    QCOMPARE(unknown.errorCode, QStringLiteral("unknown_integration"));
}

void IntegrationEngineTest::keepsHistoryWithinTheConfiguredRetention()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);
    engine.setHistoryRetention(3);
    QVERIFY(engine.save(httpDefinition()).valid);

    for (int index = 0; index < 6; ++index) {
        engine.execute(IntegrationRequest{.integrationId = QStringLiteral("obs-http"),
                                          .operation = QStringLiteral("request.send")});
    }

    QCOMPARE(repository.calls.size(), 3);
    QCOMPARE(engine.history(QStringLiteral("obs-http"), 10).size(), 3);
}

void IntegrationEngineTest::removesSecretsFromExportedDefinitions()
{
    IntegrationEngine engine;
    MemoryRepository repository;
    FakeAdapter adapter;
    engine.registerAdapter(IntegrationType::Http, &adapter);
    engine.setRepository(&repository);

    auto definition = httpDefinition();
    definition.configuration.insert(QStringLiteral("headers"),
                                    QVariantMap{{QStringLiteral("Authorization"),
                                                 QStringLiteral("obs-http/authorization")}});
    definition.configuration.insert(QStringLiteral("url"),
                                    QStringLiteral("https://user:senha@exemplo.local/hook"));
    definition.secretReferences = {QStringLiteral("obs-http/authorization")};
    QVERIFY(engine.save(definition).valid);

    const auto exported = engine.exportableDefinitions();
    QCOMPARE(exported.size(), 1);
    const auto headers = exported.first().configuration.value(QStringLiteral("headers")).toMap();
    QCOMPARE(headers.value(QStringLiteral("Authorization")).toString(), QStringLiteral("***"));
    const auto url = exported.first().configuration.value(QStringLiteral("url")).toString();
    QVERIFY(!url.contains(QStringLiteral("senha")));
    QVERIFY(url.contains(QStringLiteral("***")));
    // A definição original permanece intacta para o motor.
    QVERIFY(engine.definition(QStringLiteral("obs-http"))
                ->configuration.value(QStringLiteral("url")).toString()
                .contains(QStringLiteral("senha")));
}

QTEST_MAIN(IntegrationEngineTest)
#include "IntegrationEngineTest.moc"
