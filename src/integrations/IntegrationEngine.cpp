#include "integrations/IntegrationEngine.h"

#include "integrations/IntegrationSanitizer.h"

#include <QTimer>
#include <QUuid>

#include <algorithm>

namespace churchpresenter {
namespace {

//! Falhas transitórias que podem ser repetidas quando o adapter declara a
//! operação segura para reenvio.
bool isTransientError(const QString &errorCode)
{
    return errorCode == QStringLiteral("timeout")
        || errorCode == QStringLiteral("connection_failed")
        || errorCode == QStringLiteral("temporarily_unavailable");
}

QString newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace

struct IntegrationEngine::PendingCall {
    IntegrationDefinition definition;
    IntegrationRequest request;
    bool isTest = false;
    int attempts = 0;
    bool settled = false;
    QDateTime startedAt;
    Completion completion;
    QTimer *guard = nullptr;
};

IntegrationEngine::IntegrationEngine(QObject *parent)
    : QObject(parent)
    , m_clock([] { return QDateTime::currentDateTimeUtc(); })
{
}

IntegrationEngine::~IntegrationEngine()
{
    cancelAll();
}

void IntegrationEngine::registerAdapter(IntegrationType type, IIntegrationAdapter *adapter)
{
    if (!adapter) {
        m_adapters.remove(type);
        return;
    }
    m_adapters.insert(type, adapter);
}

void IntegrationEngine::setRepository(IIntegrationRepository *repository)
{
    m_repository = repository;
    reload();
}

void IntegrationEngine::setSecretStore(ISecretStore *secretStore)
{
    m_secretStore = secretStore;
}

void IntegrationEngine::setClock(std::function<QDateTime()> clock)
{
    if (clock) m_clock = std::move(clock);
}

void IntegrationEngine::setHistoryRetention(int maximumEntriesPerIntegration)
{
    m_historyRetention = std::max(0, maximumEntriesPerIntegration);
}

void IntegrationEngine::reload()
{
    m_definitions = m_repository ? m_repository->definitions() : QVector<IntegrationDefinition>{};
}

QVector<IntegrationDefinition> IntegrationEngine::definitions() const
{
    return m_definitions;
}

std::optional<IntegrationDefinition> IntegrationEngine::definition(const QString &id) const
{
    const auto found = std::find_if(m_definitions.cbegin(), m_definitions.cend(),
                                    [&id](const IntegrationDefinition &candidate) {
        return candidate.id == id;
    });
    if (found == m_definitions.cend()) return std::nullopt;
    return *found;
}

QVector<IntegrationDefinition> IntegrationEngine::exportableDefinitions() const
{
    QVector<IntegrationDefinition> exported;
    exported.reserve(m_definitions.size());
    for (const auto &definition : m_definitions) {
        exported.append(IntegrationSanitizer::sanitizedDefinition(definition));
    }
    return exported;
}

IntegrationValidation IntegrationEngine::validate(const IntegrationDefinition &definition) const
{
    IntegrationValidation validation{.valid = true, .errors = {}};
    if (definition.id.trimmed().isEmpty()) {
        validation.errors.append(QStringLiteral("A integração precisa de um identificador."));
    }
    if (definition.name.trimmed().isEmpty()) {
        validation.errors.append(QStringLiteral("A integração precisa de um nome."));
    }
    if (definition.timeoutMs < MinimumTimeoutMs || definition.timeoutMs > MaximumTimeoutMs) {
        validation.errors.append(QStringLiteral("O timeout deve ficar entre %1 e %2 ms.")
                                     .arg(MinimumTimeoutMs)
                                     .arg(MaximumTimeoutMs));
    }
    if (definition.retryPolicy.maximumAttempts < 1
        || definition.retryPolicy.maximumAttempts > MaximumAttempts) {
        validation.errors.append(QStringLiteral("O número de tentativas deve ficar entre 1 e %1.")
                                     .arg(MaximumAttempts));
    }
    if (definition.retryPolicy.backoffMs < 0) {
        validation.errors.append(QStringLiteral("O intervalo entre tentativas não pode ser negativo."));
    }
    for (auto it = definition.configuration.cbegin(); it != definition.configuration.cend(); ++it) {
        if (IntegrationSanitizer::isSensitiveKey(it.key()) && !it.value().toString().isEmpty()
            && !definition.secretReferences.contains(it.value().toString())) {
            validation.errors.append(
                QStringLiteral("O campo %1 deve referenciar um segredo do cofre, "
                               "não conter o valor.").arg(it.key()));
        }
    }
    const auto adapter = m_adapters.value(definition.type, nullptr);
    if (!adapter) {
        validation.errors.append(QStringLiteral("Nenhum adapter registrado para o tipo %1.")
                                     .arg(integrationTypeName(definition.type)));
    } else {
        const auto adapterValidation = adapter->validate(definition);
        if (!adapterValidation.valid) validation.errors.append(adapterValidation.errors);
    }
    validation.valid = validation.errors.isEmpty();
    return validation;
}

IntegrationValidation IntegrationEngine::save(const IntegrationDefinition &definition)
{
    const auto validation = validate(definition);
    if (!validation.valid) return validation;
    if (!m_repository) {
        return IntegrationValidation::failure(
            QStringLiteral("Nenhum repositório de integrações configurado."));
    }
    if (!m_repository->save(definition)) {
        return IntegrationValidation::failure(
            QStringLiteral("A integração não pôde ser salva."));
    }
    reload();
    return IntegrationValidation::ok();
}

bool IntegrationEngine::remove(const QString &integrationId)
{
    if (!m_repository || !m_repository->remove(integrationId)) return false;
    reload();
    return true;
}

void IntegrationEngine::execute(const IntegrationRequest &request, Completion completion)
{
    auto prepared = request;
    if (prepared.id.isEmpty()) prepared.id = newId();
    if (!prepared.issuedAt.isValid()) prepared.issuedAt = now();

    const auto reject = [this, &completion, &prepared](const QString &code,
                                                       const QString &message) {
        const IntegrationResult result{.accepted = false, .errorCode = code, .message = message};
        if (completion) completion(result);
        emit callFinished(prepared, result);
    };

    const auto found = definition(prepared.integrationId);
    if (!found.has_value() || !m_adapters.contains(found->type)) {
        reject(QStringLiteral("unknown_integration"),
               QStringLiteral("Integração não encontrada."));
        return;
    }
    if (!found->enabled) {
        reject(QStringLiteral("integration_disabled"),
               QStringLiteral("A integração está desativada."));
        return;
    }
    start(*found, prepared, false, std::move(completion));
}

void IntegrationEngine::test(const QString &integrationId, Completion completion)
{
    IntegrationRequest request{
        .id = newId(),
        .integrationId = integrationId,
        .operation = QStringLiteral("connection.test"),
        .payload = {},
        .correlationId = newId(),
        .issuedAt = now(),
    };
    const auto found = definition(integrationId);
    if (!found.has_value() || !m_adapters.contains(found->type)) {
        IntegrationResult result{.accepted = false,
                                 .errorCode = QStringLiteral("unknown_integration"),
                                 .message = QStringLiteral("Integração não encontrada.")};
        if (completion) completion(result);
        emit callFinished(request, result);
        return;
    }
    // O teste de conexão funciona mesmo com a integração desativada.
    start(*found, request, true, std::move(completion));
}

void IntegrationEngine::start(const IntegrationDefinition &definition,
                              const IntegrationRequest &request, bool isTest,
                              Completion completion)
{
    auto call = std::make_shared<PendingCall>();
    call->definition = definition;
    call->request = request;
    call->isTest = isTest;
    call->startedAt = now();
    call->completion = std::move(completion);
    m_pending.append(call);
    attempt(call);
}

void IntegrationEngine::attempt(const std::shared_ptr<PendingCall> &call)
{
    auto *adapter = m_adapters.value(call->definition.type, nullptr);
    if (!adapter) {
        finish(call, IntegrationResult{.accepted = false,
                                       .errorCode = QStringLiteral("unknown_integration"),
                                       .message = QStringLiteral("Adapter indisponível.")});
        return;
    }
    ++call->attempts;

    // Guarda de timeout: o adapter deve respeitar o seu próprio limite, mas o
    // motor nunca deixa uma chamada pendente para sempre.
    auto *guard = new QTimer(this);
    guard->setSingleShot(true);
    guard->setInterval(call->definition.timeoutMs + 500);
    connect(guard, &QTimer::timeout, this, [this, call] {
        if (call->settled) return;
        finish(call, IntegrationResult{.accepted = false,
                                       .errorCode = QStringLiteral("timeout"),
                                       .message = QStringLiteral("A integração não respondeu.")});
    });
    call->guard = guard;
    guard->start();

    const auto handleResult = [this, call](const IntegrationResult &result) {
        if (call->settled) return;
        if (call->guard) {
            call->guard->stop();
            call->guard->deleteLater();
            call->guard = nullptr;
        }
        auto *adapterForRetry = m_adapters.value(call->definition.type, nullptr);
        const bool canRetry = !result.accepted && !m_cancelling
            && call->attempts < call->definition.retryPolicy.maximumAttempts
            && isTransientError(result.errorCode) && adapterForRetry
            && adapterForRetry->isRetriable(call->definition, call->request.operation);
        if (canRetry) {
            QTimer::singleShot(std::max(0, call->definition.retryPolicy.backoffMs), this,
                               [this, call] {
                if (!call->settled) attempt(call);
            });
            return;
        }
        finish(call, result);
    };

    if (call->isTest) {
        adapter->test(call->definition, handleResult);
    } else {
        adapter->execute(call->definition, call->request, handleResult);
    }
}

void IntegrationEngine::finish(const std::shared_ptr<PendingCall> &call, IntegrationResult result)
{
    if (call->settled) return;
    call->settled = true;
    if (call->guard) {
        call->guard->stop();
        call->guard->deleteLater();
        call->guard = nullptr;
    }

    result.attempts = std::max(1, call->attempts);
    if (result.durationMs <= 0) {
        result.durationMs = static_cast<int>(
            std::max<qint64>(0, call->startedAt.msecsTo(now())));
    }
    const auto sanitized = IntegrationSanitizer::sanitizedResult(result,
                                                                 knownSecrets(call->definition));

    if (m_repository) {
        m_repository->recordCall(IntegrationCall{
            .id = newId(),
            .integrationId = call->definition.id,
            .operation = call->request.operation,
            .correlationId = call->request.correlationId,
            .accepted = sanitized.accepted,
            .errorCode = sanitized.errorCode,
            .message = sanitized.message,
            .durationMs = sanitized.durationMs,
            .attempts = sanitized.attempts,
            .occurredAt = now(),
        });
        if (m_historyRetention > 0) m_repository->pruneHistory(m_historyRetention);
    }

    m_pending.removeIf([&call](const std::shared_ptr<PendingCall> &pending) {
        return pending == call;
    });

    if (call->completion) call->completion(sanitized);
    emit callFinished(call->request, sanitized);
}

void IntegrationEngine::cancelAll()
{
    m_cancelling = true;
    for (auto *adapter : std::as_const(m_adapters)) adapter->cancelAll();
    const auto pending = m_pending;
    for (const auto &call : pending) {
        finish(call, IntegrationResult{.accepted = false,
                                       .errorCode = QStringLiteral("cancelled"),
                                       .message = QStringLiteral("Chamada cancelada.")});
    }
    m_pending.clear();
    m_cancelling = false;
}

QVector<IntegrationCall> IntegrationEngine::history(const QString &integrationId, int limit) const
{
    if (!m_repository) return {};
    return m_repository->history(integrationId, limit);
}

QStringList IntegrationEngine::knownSecrets(const IntegrationDefinition &definition) const
{
    QStringList secrets;
    if (!m_secretStore) return secrets;
    for (const auto &reference : definition.secretReferences) {
        if (const auto secret = m_secretStore->retrieve(reference)) {
            if (!secret->isEmpty()) secrets.append(*secret);
        }
    }
    return secrets;
}

QDateTime IntegrationEngine::now() const
{
    return m_clock ? m_clock() : QDateTime::currentDateTimeUtc();
}

} // namespace churchpresenter
