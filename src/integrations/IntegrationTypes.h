#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <optional>

namespace churchpresenter {

enum class IntegrationType {
    Http,
    WebSocket,
    Obs,
    Midi,
    Osc,
};

[[nodiscard]] QString integrationTypeName(IntegrationType type);
[[nodiscard]] std::optional<IntegrationType> integrationTypeFromName(const QString &name);
[[nodiscard]] QStringList integrationTypeNames();

//! Política de repetição de uma integração. A repetição só é aplicada quando o
//! adapter declara a operação segura para reenvio.
struct RetryPolicy {
    int maximumAttempts = 1;
    int backoffMs = 250;
};

struct IntegrationDefinition {
    QString id;
    QString name;
    IntegrationType type = IntegrationType::Http;
    bool enabled = true;
    QVariantMap configuration;
    //! Chaves do `ISecretStore` usadas por esta integração. Os valores nunca
    //! são gravados na configuração.
    QStringList secretReferences;
    int timeoutMs = 5000;
    RetryPolicy retryPolicy;
};

struct IntegrationRequest {
    QString id;
    QString integrationId;
    QString operation;
    QVariantMap payload;
    QString correlationId;
    QDateTime issuedAt;
};

struct IntegrationResult {
    bool accepted = false;
    QString errorCode;
    QString message;
    int durationMs = 0;
    QVariantMap responseMetadata;
    int attempts = 1;
};

//! Registro persistido de uma chamada, já sanitizado.
struct IntegrationCall {
    QString id;
    QString integrationId;
    QString operation;
    QString correlationId;
    bool accepted = false;
    QString errorCode;
    QString message;
    int durationMs = 0;
    int attempts = 1;
    QDateTime occurredAt;
};

struct IntegrationValidation {
    bool valid = false;
    QStringList errors;

    static IntegrationValidation ok() { return {.valid = true, .errors = {}}; }
    static IntegrationValidation failure(const QString &error)
    {
        return {.valid = false, .errors = {error}};
    }
};

} // namespace churchpresenter
