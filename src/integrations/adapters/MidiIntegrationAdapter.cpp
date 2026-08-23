#include "integrations/adapters/MidiIntegrationAdapter.h"

namespace churchpresenter {
namespace {

constexpr auto PortKey = "port";
constexpr auto ChannelKey = "channel";

constexpr int NoteOffStatus = 0x80;
constexpr int NoteOnStatus = 0x90;
constexpr int ControlChangeStatus = 0xB0;
constexpr int ProgramChangeStatus = 0xC0;

QString portOf(const IntegrationDefinition &definition)
{
    return definition.configuration.value(QLatin1StringView(PortKey)).toString().trimmed();
}

int channelOf(const IntegrationDefinition &definition, const IntegrationRequest &request)
{
    const auto override = request.payload.value(QLatin1StringView(ChannelKey));
    if (override.isValid()) return override.toInt();
    return definition.configuration.value(QLatin1StringView(ChannelKey), 1).toInt();
}

bool isValueInRange(int value)
{
    return value >= 0 && value <= MidiIntegrationAdapter::MaximumValue;
}

} // namespace

MidiIntegrationAdapter::MidiIntegrationAdapter(IMidiTransport &transport)
    : m_transport(transport)
{
}

QStringList MidiIntegrationAdapter::supportedOperations()
{
    return {QStringLiteral("note.on"), QStringLiteral("note.off"),
            QStringLiteral("control.change"), QStringLiteral("program.change")};
}

QStringList MidiIntegrationAdapter::availablePorts() const
{
    return m_transport.outputPorts();
}

IntegrationValidation MidiIntegrationAdapter::validate(
    const IntegrationDefinition &definition) const
{
    IntegrationValidation validation{.valid = true, .errors = {}};
    if (portOf(definition).isEmpty()) {
        validation.errors.append(QStringLiteral("Escolha uma porta MIDI de saída."));
    }
    const auto channel = definition.configuration.value(QLatin1StringView(ChannelKey), 1).toInt();
    if (channel < MinimumChannel || channel > MaximumChannel) {
        validation.errors.append(QStringLiteral("O canal MIDI deve ficar entre %1 e %2.")
                                     .arg(MinimumChannel)
                                     .arg(MaximumChannel));
    }
    validation.valid = validation.errors.isEmpty();
    return validation;
}

void MidiIntegrationAdapter::test(const IntegrationDefinition &definition, Completion completion)
{
    const auto validation = validate(definition);
    if (!validation.valid) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_configuration"),
                                     .message = validation.errors.join(QLatin1Char(' '))});
        return;
    }
    const auto ports = m_transport.outputPorts();
    const auto port = portOf(definition);
    if (!ports.contains(port)) {
        completion(IntegrationResult{
            .accepted = false,
            .errorCode = QStringLiteral("port_unavailable"),
            .message = QStringLiteral("A porta %1 não está conectada.").arg(port),
            .responseMetadata = {{QStringLiteral("availablePorts"), ports}},
        });
        return;
    }
    // Abrir a porta prova o acesso sem tocar nenhuma nota no dispositivo.
    if (!m_transport.openPort(port)) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("connection_failed"),
                                     .message = m_transport.lastError()});
        return;
    }
    completion(IntegrationResult{
        .accepted = true,
        .message = QStringLiteral("Porta %1 disponível.").arg(port),
        .responseMetadata = {{QStringLiteral("availablePorts"), ports}},
    });
}

void MidiIntegrationAdapter::execute(const IntegrationDefinition &definition,
                                     const IntegrationRequest &request, Completion completion)
{
    if (!supportedOperations().contains(request.operation)) {
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

    const auto channel = channelOf(definition, request);
    if (channel < MinimumChannel || channel > MaximumChannel) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_payload"),
                                     .message = QStringLiteral("O canal MIDI deve ficar entre %1 e %2.")
                                                    .arg(MinimumChannel)
                                                    .arg(MaximumChannel)});
        return;
    }

    MidiMessage message;
    message.channel = channel;
    if (request.operation == QStringLiteral("note.on")
        || request.operation == QStringLiteral("note.off")) {
        message.status = request.operation == QStringLiteral("note.on") ? NoteOnStatus
                                                                        : NoteOffStatus;
        message.data1 = request.payload.value(QStringLiteral("note")).toInt();
        message.data2 = request.payload.value(QStringLiteral("velocity"), 100).toInt();
    } else if (request.operation == QStringLiteral("control.change")) {
        message.status = ControlChangeStatus;
        message.data1 = request.payload.value(QStringLiteral("controller")).toInt();
        message.data2 = request.payload.value(QStringLiteral("value")).toInt();
    } else {
        message.status = ProgramChangeStatus;
        message.data1 = request.payload.value(QStringLiteral("program")).toInt();
        message.data2 = 0;
    }

    if (!isValueInRange(message.data1) || !isValueInRange(message.data2)) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_payload"),
                                     .message = QStringLiteral("Os valores MIDI devem ficar entre 0 e %1.")
                                                    .arg(MaximumValue)});
        return;
    }

    const auto port = portOf(definition);
    if (!m_transport.send(port, message)) {
        const auto ports = m_transport.outputPorts();
        completion(IntegrationResult{
            .accepted = false,
            .errorCode = ports.contains(port) ? QStringLiteral("connection_failed")
                                              : QStringLiteral("port_unavailable"),
            .message = m_transport.lastError(),
        });
        return;
    }
    completion(IntegrationResult{
        .accepted = true,
        .message = QStringLiteral("Mensagem MIDI enviada."),
        .responseMetadata = {{QStringLiteral("channel"), message.channel},
                             {QStringLiteral("status"), message.status},
                             {QStringLiteral("data1"), message.data1},
                             {QStringLiteral("data2"), message.data2}},
    });
}

void MidiIntegrationAdapter::cancelAll()
{
    m_transport.closeAll();
}

} // namespace churchpresenter
