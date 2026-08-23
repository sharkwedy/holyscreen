#include "integrations/adapters/HttpIntegrationAdapter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

namespace churchpresenter {
namespace {

constexpr auto UrlKey = "url";
constexpr auto MethodKey = "method";
constexpr auto HeadersKey = "headers";
constexpr auto BodyKey = "body";

QString methodOf(const IntegrationDefinition &definition)
{
    const auto method = definition.configuration.value(QLatin1StringView(MethodKey))
                            .toString().trimmed().toUpper();
    return method.isEmpty() ? QStringLiteral("POST") : method;
}

bool isIdempotent(const QString &method)
{
    return method == QStringLiteral("GET") || method == QStringLiteral("HEAD");
}

} // namespace

HttpIntegrationAdapter::HttpIntegrationAdapter(IHttpTransport &transport,
                                               const ISecretStore *secretStore)
    : m_transport(transport)
    , m_secretStore(secretStore)
{
}

QStringList HttpIntegrationAdapter::supportedMethods()
{
    return {QStringLiteral("GET"), QStringLiteral("POST"), QStringLiteral("PUT"),
            QStringLiteral("PATCH"), QStringLiteral("DELETE")};
}

QStringList HttpIntegrationAdapter::allowedResponseHeaders()
{
    return {QStringLiteral("content-type"), QStringLiteral("content-length"),
            QStringLiteral("retry-after"), QStringLiteral("location")};
}

QString HttpIntegrationAdapter::applyTemplate(const QString &source, const QVariantMap &payload)
{
    static const QRegularExpression placeholder(QStringLiteral("\\{\\{\\s*([\\w.-]+)\\s*\\}\\}"));
    QString result;
    result.reserve(source.size());
    qsizetype cursor = 0;
    auto matches = placeholder.globalMatch(source);
    while (matches.hasNext()) {
        const auto match = matches.next();
        result.append(source.mid(cursor, match.capturedStart() - cursor));
        result.append(payload.value(match.captured(1)).toString());
        cursor = match.capturedEnd();
    }
    result.append(source.mid(cursor));
    return result;
}

IntegrationValidation HttpIntegrationAdapter::validate(
    const IntegrationDefinition &definition) const
{
    IntegrationValidation validation{.valid = true, .errors = {}};
    const QUrl url(definition.configuration.value(QLatin1StringView(UrlKey)).toString().trimmed());
    if (!url.isValid() || url.host().isEmpty()) {
        validation.errors.append(QStringLiteral("Informe uma URL válida."));
    } else if (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https")) {
        validation.errors.append(QStringLiteral("Somente http e https são aceitos."));
    }
    if (!supportedMethods().contains(methodOf(definition))) {
        validation.errors.append(QStringLiteral("O método deve ser %1.")
                                     .arg(supportedMethods().join(QStringLiteral(", "))));
    }
    const auto headers = definition.configuration.value(QLatin1StringView(HeadersKey)).toMap();
    if (headers.size() > MaximumHeaders) {
        validation.errors.append(QStringLiteral("No máximo %1 cabeçalhos.").arg(MaximumHeaders));
    }
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        if (it.key().trimmed().isEmpty()) {
            validation.errors.append(QStringLiteral("Cabeçalho sem nome."));
            break;
        }
    }
    const auto body = definition.configuration.value(QLatin1StringView(BodyKey)).toString();
    if (body.toUtf8().size() > MaximumBodyBytes) {
        validation.errors.append(QStringLiteral("O corpo excede %1 KiB.")
                                     .arg(MaximumBodyBytes / 1024));
    }
    validation.valid = validation.errors.isEmpty();
    return validation;
}

QString HttpIntegrationAdapter::resolveSecret(const QString &reference) const
{
    if (!m_secretStore) return {};
    const auto secret = m_secretStore->retrieve(reference);
    return secret.value_or(QString{});
}

HttpRequest HttpIntegrationAdapter::buildRequest(const IntegrationDefinition &definition,
                                                 const IntegrationRequest &request) const
{
    HttpRequest built;
    built.method = methodOf(definition);
    built.url = applyTemplate(
        definition.configuration.value(QLatin1StringView(UrlKey)).toString().trimmed(),
        request.payload);
    built.timeoutMs = definition.timeoutMs;
    built.maximumResponseBytes =
        definition.configuration.value(QStringLiteral("maximumResponseBytes"),
                                       static_cast<qint64>(MaximumResponseBytes)).toLongLong();

    const auto headers = definition.configuration.value(QLatin1StringView(HeadersKey)).toMap();
    for (auto it = headers.cbegin(); it != headers.cend(); ++it) {
        auto value = it.value().toString();
        // Um cabeçalho pode apontar para uma referência do cofre em vez de
        // conter o valor.
        if (definition.secretReferences.contains(value)) {
            value = resolveSecret(value);
        } else {
            value = applyTemplate(value, request.payload);
        }
        built.headers.insert(it.key(), value);
    }

    const auto bodyTemplate = definition.configuration.value(QLatin1StringView(BodyKey)).toString();
    if (!bodyTemplate.isEmpty()) {
        built.body = applyTemplate(bodyTemplate, request.payload).toUtf8();
    } else if (!request.payload.isEmpty()
               && built.method != QStringLiteral("GET")
               && built.method != QStringLiteral("HEAD")) {
        built.body = QJsonDocument(QJsonObject::fromVariantMap(request.payload))
                         .toJson(QJsonDocument::Compact);
        if (!built.headers.contains(QStringLiteral("Content-Type"))) {
            built.headers.insert(QStringLiteral("Content-Type"),
                                 QStringLiteral("application/json"));
        }
    }
    return built;
}

void HttpIntegrationAdapter::test(const IntegrationDefinition &definition, Completion completion)
{
    auto request = buildRequest(definition, IntegrationRequest{});
    // Um teste nunca pode disparar a ação real do webhook.
    request.method = QStringLiteral("HEAD");
    request.body.clear();
    m_transport.send(request, [completion](const HttpResponse &response) {
        if (!response.completed) {
            completion(IntegrationResult{.accepted = false,
                                         .errorCode = response.errorCode,
                                         .message = response.message});
            return;
        }
        completion(IntegrationResult{
            .accepted = response.status > 0 && response.status < 500,
            .errorCode = response.status >= 500 ? QStringLiteral("temporarily_unavailable")
                                                : QString{},
            .message = QStringLiteral("O servidor respondeu %1.").arg(response.status),
            .responseMetadata = {{QStringLiteral("status"), response.status}},
        });
    });
}

void HttpIntegrationAdapter::execute(const IntegrationDefinition &definition,
                                     const IntegrationRequest &request, Completion completion)
{
    if (request.operation != QStringLiteral("request.send")) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("unsupported_operation"),
                                     .message = QStringLiteral("Operação não suportada: %1.")
                                                    .arg(request.operation)});
        return;
    }
    m_transport.send(buildRequest(definition, request), [completion](const HttpResponse &response) {
        if (!response.completed) {
            completion(IntegrationResult{.accepted = false,
                                         .errorCode = response.errorCode,
                                         .message = response.message});
            return;
        }
        QVariantMap metadata{{QStringLiteral("status"), response.status}};
        for (auto it = response.headers.cbegin(); it != response.headers.cend(); ++it) {
            if (allowedResponseHeaders().contains(it.key().toLower())) {
                metadata.insert(it.key().toLower(), it.value());
            }
        }
        const bool accepted = response.status >= 200 && response.status < 400;
        completion(IntegrationResult{
            .accepted = accepted,
            .errorCode = accepted ? QString{}
                                  : response.status >= 500
                                        ? QStringLiteral("temporarily_unavailable")
                                        : QStringLiteral("rejected"),
            .message = QStringLiteral("O servidor respondeu %1.").arg(response.status),
            .responseMetadata = metadata,
        });
    });
}

void HttpIntegrationAdapter::cancelAll()
{
    m_transport.cancelAll();
}

bool HttpIntegrationAdapter::isRetriable(const IntegrationDefinition &definition,
                                         const QString &operation) const
{
    // O teste de conexão usa HEAD; um envio só pode ser repetido quando o
    // método é idempotente, senão um POST repetido criaria efeito duplicado.
    if (operation == QStringLiteral("connection.test")) return true;
    return isIdempotent(methodOf(definition));
}

} // namespace churchpresenter
