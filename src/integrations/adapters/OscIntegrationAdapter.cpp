#include "integrations/adapters/OscIntegrationAdapter.h"

#include "integrations/adapters/HttpIntegrationAdapter.h"
#include "integrations/adapters/OscMessage.h"

#include <QHostAddress>
#include <QRegularExpression>

namespace churchpresenter {
namespace {

constexpr auto HostKey = "host";
constexpr auto PortKey = "port";
constexpr auto AddressKey = "address";
constexpr auto ArgumentsKey = "arguments";

//! Os marcadores `{{campo}}` são resolvidos no envio; para validar a
//! configuração eles viram um segmento neutro.
QString withoutPlaceholders(const QString &address)
{
    static const QRegularExpression placeholder(QStringLiteral("\\{\\{[^}]*\\}\\}"));
    QString stripped = address;
    return stripped.replace(placeholder, QStringLiteral("x"));
}

QString hostOf(const IntegrationDefinition &definition)
{
    return definition.configuration.value(QLatin1StringView(HostKey)).toString().trimmed();
}

int portOf(const IntegrationDefinition &definition)
{
    return definition.configuration.value(QLatin1StringView(PortKey)).toInt();
}

} // namespace

OscIntegrationAdapter::OscIntegrationAdapter(IOscTransport &transport)
    : m_transport(transport)
{
}

IntegrationValidation OscIntegrationAdapter::validate(
    const IntegrationDefinition &definition) const
{
    IntegrationValidation validation{.valid = true, .errors = {}};
    QHostAddress address;
    if (!address.setAddress(hostOf(definition))) {
        validation.errors.append(QStringLiteral("Informe um endereço IPv4 ou IPv6 válido."));
    }
    const auto port = portOf(definition);
    if (port <= 0 || port > 65535) {
        validation.errors.append(QStringLiteral("A porta deve ficar entre 1 e 65535."));
    }
    const auto oscAddress = withoutPlaceholders(
        definition.configuration.value(QLatin1StringView(AddressKey)).toString());
    if (!OscMessage::isValidAddress(oscAddress)) {
        validation.errors.append(
            QStringLiteral("O caminho OSC precisa começar com / e não pode ter espaços."));
    }
    if (!OscMessage::encode(oscAddress,
                            definition.configuration.value(QLatin1StringView(ArgumentsKey))
                                .toList()).has_value()) {
        validation.errors.append(
            QStringLiteral("Os argumentos precisam ser int32, float32, texto ou booleano."));
    }
    validation.valid = validation.errors.isEmpty();
    return validation;
}

void OscIntegrationAdapter::test(const IntegrationDefinition &definition, Completion completion)
{
    // OSC é sem conexão: o teste confere a configuração e a rota até o host,
    // sem enviar nenhum comando ao dispositivo.
    const auto validation = validate(definition);
    if (!validation.valid) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_configuration"),
                                     .message = validation.errors.join(QLatin1Char(' '))});
        return;
    }
    completion(IntegrationResult{
        .accepted = true,
        .message = QStringLiteral("Configuração válida para %1:%2. OSC não confirma entrega.")
                       .arg(hostOf(definition))
                       .arg(portOf(definition)),
        .responseMetadata = {{QStringLiteral("host"), hostOf(definition)},
                             {QStringLiteral("port"), portOf(definition)}},
    });
}

void OscIntegrationAdapter::execute(const IntegrationDefinition &definition,
                                    const IntegrationRequest &request, Completion completion)
{
    if (request.operation != QStringLiteral("message.send")) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("unsupported_operation"),
                                     .message = QStringLiteral("Operação não suportada: %1.")
                                                    .arg(request.operation)});
        return;
    }
    const auto validation = validate(definition);
    if (!validation.valid) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_configuration"),
                                     .message = validation.errors.join(QLatin1Char(' '))});
        return;
    }

    auto oscAddress = definition.configuration.value(QLatin1StringView(AddressKey)).toString();
    oscAddress = HttpIntegrationAdapter::applyTemplate(oscAddress, request.payload);
    auto arguments = request.payload.value(QLatin1StringView(ArgumentsKey)).toList();
    if (arguments.isEmpty()) {
        arguments = definition.configuration.value(QLatin1StringView(ArgumentsKey)).toList();
    }

    const auto datagram = OscMessage::encode(oscAddress, arguments);
    if (!datagram.has_value()) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_payload"),
                                     .message = QStringLiteral("Mensagem OSC inválida.")});
        return;
    }
    const auto sent = m_transport.send(hostOf(definition),
                                       static_cast<quint16>(portOf(definition)), *datagram);
    completion(IntegrationResult{
        .accepted = sent,
        .errorCode = sent ? QString{} : QStringLiteral("connection_failed"),
        .message = sent ? QStringLiteral("Datagrama enviado.") : m_transport.lastError(),
        .responseMetadata = {{QStringLiteral("bytes"), datagram->size()},
                             {QStringLiteral("address"), oscAddress}},
    });
}

void OscIntegrationAdapter::cancelAll()
{
    // UDP não mantém chamadas pendentes.
}

} // namespace churchpresenter
