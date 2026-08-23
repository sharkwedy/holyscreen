#include "integrations/adapters/ObsIntegrationAdapter.h"

#include <QHostAddress>
#include <QJsonObject>

namespace churchpresenter {
namespace {

constexpr auto HostKey = "host";
constexpr auto PortKey = "port";
constexpr auto PasswordKey = "passwordReference";

QString hostOf(const IntegrationDefinition &definition)
{
    const auto host = definition.configuration.value(QLatin1StringView(HostKey)).toString().trimmed();
    return host.isEmpty() ? QStringLiteral("127.0.0.1") : host;
}

quint16 portOf(const IntegrationDefinition &definition)
{
    const auto port = definition.configuration.value(QLatin1StringView(PortKey), 4455).toInt();
    return static_cast<quint16>(port);
}

} // namespace

ObsIntegrationAdapter::ObsIntegrationAdapter(IObsClient &client, const ISecretStore *secretStore)
    : m_client(client)
    , m_secretStore(secretStore)
{
}

QStringList ObsIntegrationAdapter::supportedOperations()
{
    return {QStringLiteral("scene.set"),      QStringLiteral("recording.start"),
            QStringLiteral("recording.stop"), QStringLiteral("streaming.start"),
            QStringLiteral("streaming.stop"), QStringLiteral("input.mute.set"),
            QStringLiteral("input.trigger"),  QStringLiteral("version.query")};
}

QString ObsIntegrationAdapter::passwordOf(const IntegrationDefinition &definition) const
{
    const auto reference = definition.configuration.value(QLatin1StringView(PasswordKey))
                               .toString().trimmed();
    if (reference.isEmpty() || !m_secretStore) return {};
    return m_secretStore->retrieve(reference).value_or(QString{});
}

IntegrationValidation ObsIntegrationAdapter::validate(
    const IntegrationDefinition &definition) const
{
    IntegrationValidation validation{.valid = true, .errors = {}};
    if (hostOf(definition).isEmpty()) {
        validation.errors.append(QStringLiteral("Informe o host do OBS."));
    }
    const auto port = definition.configuration.value(QLatin1StringView(PortKey), 4455).toInt();
    if (port <= 0 || port > 65535) {
        validation.errors.append(QStringLiteral("A porta deve ficar entre 1 e 65535."));
    }
    const auto reference = definition.configuration.value(QLatin1StringView(PasswordKey))
                               .toString().trimmed();
    if (!reference.isEmpty() && !definition.secretReferences.contains(reference)) {
        validation.errors.append(
            QStringLiteral("A senha do OBS precisa estar no cofre e ser listada em "
                           "secretReferences."));
    }
    validation.valid = validation.errors.isEmpty();
    return validation;
}

ObsIntegrationAdapter::ObsCall ObsIntegrationAdapter::callFor(const IntegrationRequest &request)
{
    const auto &payload = request.payload;
    if (request.operation == QStringLiteral("scene.set")) {
        const auto scene = payload.value(QStringLiteral("sceneName")).toString().trimmed();
        if (scene.isEmpty()) {
            return {.errorCode = QStringLiteral("invalid_payload"),
                    .message = QStringLiteral("Informe o nome da cena.")};
        }
        return {.requestType = QStringLiteral("SetCurrentProgramScene"),
                .requestData = {{QStringLiteral("sceneName"), scene}}};
    }
    if (request.operation == QStringLiteral("recording.start")) {
        return {.requestType = QStringLiteral("StartRecord")};
    }
    if (request.operation == QStringLiteral("recording.stop")) {
        return {.requestType = QStringLiteral("StopRecord")};
    }
    if (request.operation == QStringLiteral("streaming.start")) {
        return {.requestType = QStringLiteral("StartStream")};
    }
    if (request.operation == QStringLiteral("streaming.stop")) {
        return {.requestType = QStringLiteral("StopStream")};
    }
    if (request.operation == QStringLiteral("version.query")) {
        return {.requestType = QStringLiteral("GetVersion")};
    }
    if (request.operation == QStringLiteral("input.mute.set")) {
        const auto input = payload.value(QStringLiteral("inputName")).toString().trimmed();
        if (input.isEmpty()) {
            return {.errorCode = QStringLiteral("invalid_payload"),
                    .message = QStringLiteral("Informe o nome do input.")};
        }
        return {.requestType = QStringLiteral("SetInputMute"),
                .requestData = {{QStringLiteral("inputName"), input},
                                {QStringLiteral("inputMuted"),
                                 payload.value(QStringLiteral("muted")).toBool()}}};
    }
    if (request.operation == QStringLiteral("input.trigger")) {
        const auto input = payload.value(QStringLiteral("inputName")).toString().trimmed();
        if (input.isEmpty()) {
            return {.errorCode = QStringLiteral("invalid_payload"),
                    .message = QStringLiteral("Informe o nome do input.")};
        }
        const auto action = payload.value(QStringLiteral("action"),
                                          QStringLiteral("OBS_WEBSOCKET_MEDIA_INPUT_ACTION_RESTART"))
                                .toString();
        return {.requestType = QStringLiteral("TriggerMediaInputAction"),
                .requestData = {{QStringLiteral("inputName"), input},
                                {QStringLiteral("mediaAction"), action}}};
    }
    return {.errorCode = QStringLiteral("unsupported_operation"),
            .message = QStringLiteral("Operação não suportada: %1.").arg(request.operation)};
}

void ObsIntegrationAdapter::withConnection(const IntegrationDefinition &definition,
                                           Completion onFailure, std::function<void()> onReady)
{
    if (m_client.state() == QStringLiteral("identified")) {
        onReady();
        return;
    }
    m_client.connectAndIdentify(hostOf(definition), portOf(definition), passwordOf(definition),
                                definition.timeoutMs,
                                [onFailure, onReady](const ObsResponse &response) {
        if (!response.success) {
            onFailure(IntegrationResult{.accepted = false,
                                        .errorCode = response.errorCode.isEmpty()
                                            ? QStringLiteral("connection_failed")
                                            : response.errorCode,
                                        .message = response.message});
            return;
        }
        onReady();
    });
}

void ObsIntegrationAdapter::test(const IntegrationDefinition &definition, Completion completion)
{
    const auto validation = validate(definition);
    if (!validation.valid) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_configuration"),
                                     .message = validation.errors.join(QLatin1Char(' '))});
        return;
    }
    withConnection(definition, completion, [this, definition, completion] {
        // GetVersion apenas consulta: nenhum efeito na transmissão.
        m_client.request(QStringLiteral("GetVersion"), {}, definition.timeoutMs,
                         [completion](const ObsResponse &response) {
            if (!response.success) {
                completion(IntegrationResult{.accepted = false,
                                             .errorCode = response.errorCode,
                                             .message = response.message});
                return;
            }
            completion(IntegrationResult{
                .accepted = true,
                .message = QStringLiteral("OBS %1 conectado.")
                               .arg(response.data.value(QStringLiteral("obsVersion")).toString()),
                .responseMetadata = {
                    {QStringLiteral("obsVersion"),
                     response.data.value(QStringLiteral("obsVersion")).toString()},
                    {QStringLiteral("rpcVersion"),
                     response.data.value(QStringLiteral("rpcVersion")).toInt()},
                },
            });
        });
    });
}

void ObsIntegrationAdapter::execute(const IntegrationDefinition &definition,
                                    const IntegrationRequest &request, Completion completion)
{
    const auto call = callFor(request);
    if (!call.errorCode.isEmpty()) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = call.errorCode,
                                     .message = call.message});
        return;
    }
    const auto validation = validate(definition);
    if (!validation.valid) {
        completion(IntegrationResult{.accepted = false,
                                     .errorCode = QStringLiteral("invalid_configuration"),
                                     .message = validation.errors.join(QLatin1Char(' '))});
        return;
    }

    withConnection(definition, completion, [this, definition, call, completion] {
        m_client.request(call.requestType, call.requestData, definition.timeoutMs,
                         [completion, call](const ObsResponse &response) {
            QVariantMap metadata{{QStringLiteral("requestType"), call.requestType}};
            for (auto it = response.data.constBegin(); it != response.data.constEnd(); ++it) {
                metadata.insert(it.key(), it.value().toVariant());
            }
            completion(IntegrationResult{
                .accepted = response.success,
                .errorCode = response.success ? QString{} : response.errorCode,
                .message = response.success ? QStringLiteral("%1 aceito pelo OBS.")
                                                  .arg(call.requestType)
                                            : response.message,
                .responseMetadata = metadata,
            });
        });
    });
}

void ObsIntegrationAdapter::cancelAll()
{
    m_client.cancelAll();
}

bool ObsIntegrationAdapter::isRetriable(const IntegrationDefinition &, const QString &operation) const
{
    // Consultas e troca de cena são idempotentes; iniciar ou parar gravação e
    // transmissão não podem ser reenviados às cegas.
    return operation == QStringLiteral("connection.test")
        || operation == QStringLiteral("version.query")
        || operation == QStringLiteral("scene.set");
}

} // namespace churchpresenter
