#include "integrations/IntegrationTypes.h"

namespace churchpresenter {
namespace {

struct TypeName {
    IntegrationType type;
    const char *name;
};

constexpr TypeName kTypeNames[] = {
    {IntegrationType::Http, "http"},
    {IntegrationType::WebSocket, "websocket"},
    {IntegrationType::Obs, "obs"},
    {IntegrationType::Midi, "midi"},
    {IntegrationType::Osc, "osc"},
};

} // namespace

QString integrationTypeName(IntegrationType type)
{
    for (const auto &entry : kTypeNames) {
        if (entry.type == type) return QString::fromLatin1(entry.name);
    }
    return QString::fromLatin1(kTypeNames[0].name);
}

std::optional<IntegrationType> integrationTypeFromName(const QString &name)
{
    const auto normalized = name.trimmed().toLower();
    for (const auto &entry : kTypeNames) {
        if (normalized == QLatin1StringView(entry.name)) return entry.type;
    }
    return std::nullopt;
}

QStringList integrationTypeNames()
{
    QStringList names;
    names.reserve(static_cast<qsizetype>(std::size(kTypeNames)));
    for (const auto &entry : kTypeNames) names.append(QString::fromLatin1(entry.name));
    return names;
}

QVariantMap integrationDefinitionToMap(const IntegrationDefinition &definition)
{
    return {
        {QStringLiteral("id"), definition.id},
        {QStringLiteral("name"), definition.name},
        {QStringLiteral("type"), integrationTypeName(definition.type)},
        {QStringLiteral("enabled"), definition.enabled},
        {QStringLiteral("configuration"), definition.configuration},
        {QStringLiteral("secretReferences"), definition.secretReferences},
        {QStringLiteral("timeoutMs"), definition.timeoutMs},
        {QStringLiteral("retryAttempts"), definition.retryPolicy.maximumAttempts},
        {QStringLiteral("retryBackoffMs"), definition.retryPolicy.backoffMs},
    };
}

IntegrationDefinition integrationDefinitionFromMap(const QVariantMap &map)
{
    IntegrationDefinition definition;
    definition.id = map.value(QStringLiteral("id")).toString().trimmed();
    definition.name = map.value(QStringLiteral("name")).toString().trimmed();
    definition.type = integrationTypeFromName(map.value(QStringLiteral("type")).toString())
                          .value_or(IntegrationType::Http);
    definition.enabled = map.value(QStringLiteral("enabled"), true).toBool();
    definition.configuration = map.value(QStringLiteral("configuration")).toMap();
    definition.secretReferences = map.value(QStringLiteral("secretReferences")).toStringList();
    definition.timeoutMs = map.value(QStringLiteral("timeoutMs"), 5000).toInt();
    definition.retryPolicy.maximumAttempts = map.value(QStringLiteral("retryAttempts"), 1).toInt();
    definition.retryPolicy.backoffMs = map.value(QStringLiteral("retryBackoffMs"), 250).toInt();
    return definition;
}

QVariantMap integrationCallToMap(const IntegrationCall &call)
{
    return {
        {QStringLiteral("id"), call.id},
        {QStringLiteral("integrationId"), call.integrationId},
        {QStringLiteral("operation"), call.operation},
        {QStringLiteral("accepted"), call.accepted},
        {QStringLiteral("errorCode"), call.errorCode},
        {QStringLiteral("message"), call.message},
        {QStringLiteral("durationMs"), call.durationMs},
        {QStringLiteral("attempts"), call.attempts},
        {QStringLiteral("occurredAt"), call.occurredAt.toLocalTime()
                                           .toString(QStringLiteral("dd/MM HH:mm:ss"))},
    };
}

} // namespace churchpresenter
