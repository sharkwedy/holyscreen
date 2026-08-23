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

} // namespace churchpresenter
