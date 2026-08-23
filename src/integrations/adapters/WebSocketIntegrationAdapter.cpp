#include "integrations/adapters/WebSocketIntegrationAdapter.h"

#include "integrations/adapters/HttpIntegrationAdapter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace churchpresenter {
namespace {

constexpr auto UrlKey = "url";
constexpr auto MessageKey = "message";

} // namespace

WebSocketIntegrationAdapter::WebSocketIntegrationAdapter(IWebSocketTransport &transport)
    : m_transport(transport)
{
}

QStringList WebSocketIntegrationAdapter::supportedOperations()
{
    return {QStringLiteral("message.send"), QStringLiteral("connection.open"),
            QStringLiteral("connection.close")};
}

QString WebSocketIntegrationAdapter::urlOf(const IntegrationDefinition &definition)
{
    return definition.configuration.value(QLatin1StringView(UrlKey)).toString().trimmed();
}

QString WebSocketIntegrationAdapter::messageOf(const IntegrationDefinition &definition,
                                               const IntegrationRequest &request)
{
    const auto explicitText = request.payload.value(QStringLiteral("text")).toString();
    if (!explicitText.isEmpty()) {
        return HttpIntegrationAdapter::applyTemplate(explicitText, request.payload);
    }
    const auto configured = definition.configuration.value(QLatin1StringView(MessageKey))
                                .toString();
    if (!configured.isEmpty()) {
        return HttpIntegrationAdapter::applyTemplate(configured, request.payload);
    }
    if (request.payload.isEmpty()) return {};
    return QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(request.payload)).toJson(QJsonDocument::Compact));
}

IntegrationValidation WebSocketIntegrationAdapter::validate(
    const IntegrationDefinition &definition) const
{
    IntegrationValidation validation{.valid = true, .errors = {}};
    const QUrl url(urlOf(definition));
    if (!url.isValid() || url.host().isEmpty()) {
        validation.errors.append(QStringLiteral("Informe uma URL válida."));
    } else if (url.scheme() != QStringLiteral("ws") && url.scheme() != QStringLiteral("wss")) {
        validation.errors.append(QStringLiteral("Somente ws e wss são aceitos."));
    }
    const auto message = definition.configuration.value(QLatin1StringView(MessageKey)).toString();
    if (message.toUtf8().size() > MaximumMessageBytes) {
        validation.errors.append(QStringLiteral("A mensagem excede %1 KiB.")
                                     .arg(MaximumMessageBytes / 1024));
    }
    validation.valid = validation.errors.isEmpty();
    return validation;
}

QString WebSocketIntegrationAdapter::state(const IntegrationDefinition &definition) const
{
    return m_transport.state(urlOf(definition));
}

void WebSocketIntegrationAdapter::test(const IntegrationDefinition &definition,
                                       Completion completion)
{
    const auto url = urlOf(definition);
    m_transport.connectTo(url, definition.timeoutMs,
                          [this, url, completion](bool accepted, const QString &errorCode,
                                                  const QString &message) {
        completion(IntegrationResult{
            .accepted = accepted,
            .errorCode = errorCode,
            .message = accepted ? QStringLiteral("Conectado.") : message,
            .responseMetadata = {{QStringLiteral("state"), m_transport.state(url)}},
        });
    });
}

void WebSocketIntegrationAdapter::execute(const IntegrationDefinition &definition,
                                          const IntegrationRequest &request,
                                          Completion completion)
{
    if (!supportedOperations().contains(request.operation)) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("unsupported_operation"),
                                     .message = QStringLiteral("Operação não suportada: %1.")
                                                    .arg(request.operation)});
        return;
    }
    const auto url = urlOf(definition);

    if (request.operation == QStringLiteral("connection.close")) {
        m_transport.disconnectFrom(url);
        completion(IntegrationResult{.accepted = true,
                                     .message = QStringLiteral("Conexão encerrada."),
                                     .responseMetadata = {{QStringLiteral("state"),
                                                           m_transport.state(url)}}});
        return;
    }

    if (request.operation == QStringLiteral("connection.open")) {
        m_transport.connectTo(url, definition.timeoutMs,
                              [this, url, completion](bool accepted, const QString &errorCode,
                                                      const QString &message) {
            completion(IntegrationResult{.accepted = accepted,
                                         .errorCode = errorCode,
                                         .message = message,
                                         .responseMetadata = {{QStringLiteral("state"),
                                                               m_transport.state(url)}}});
        });
        return;
    }

    const auto text = messageOf(definition, request);
    if (text.isEmpty()) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_payload"),
                                     .message = QStringLiteral("Nenhuma mensagem para enviar.")});
        return;
    }
    if (text.toUtf8().size() > MaximumMessageBytes) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("payload_too_large"),
                                     .message = QStringLiteral("A mensagem excede %1 KiB.")
                                                    .arg(MaximumMessageBytes / 1024)});
        return;
    }
    m_transport.sendText(url, text,
                         [this, url, completion](bool accepted, const QString &errorCode,
                                                 const QString &message) {
        completion(IntegrationResult{.accepted = accepted,
                                     .errorCode = errorCode,
                                     .message = accepted ? QStringLiteral("Mensagem enviada.")
                                                         : message,
                                     .responseMetadata = {{QStringLiteral("state"),
                                                           m_transport.state(url)}}});
    });
}

void WebSocketIntegrationAdapter::cancelAll()
{
    m_transport.cancelAll();
}

bool WebSocketIntegrationAdapter::isRetriable(const IntegrationDefinition &, 
                                              const QString &operation) const
{
    // Abrir ou testar a conexão é idempotente; reenviar uma mensagem não é.
    return operation == QStringLiteral("connection.test")
        || operation == QStringLiteral("connection.open");
}

} // namespace churchpresenter
