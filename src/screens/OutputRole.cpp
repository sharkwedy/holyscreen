#include "screens/OutputRole.h"

namespace churchpresenter {
namespace {

struct RoleName {
    OutputRole role;
    const char *name;
};

constexpr RoleName kRoleNames[] = {
    {OutputRole::Audience, "audience"},
    {OutputRole::Stage, "stage"},
    {OutputRole::Broadcast, "broadcast"},
    {OutputRole::Confidence, "confidence"},
    {OutputRole::Custom, "custom"},
};

} // namespace

QString outputRoleName(OutputRole role)
{
    for (const auto &entry : kRoleNames) {
        if (entry.role == role) return QString::fromLatin1(entry.name);
    }
    return QString::fromLatin1(kRoleNames[0].name);
}

std::optional<OutputRole> outputRoleFromName(const QString &name)
{
    const auto normalized = name.trimmed().toLower();
    for (const auto &entry : kRoleNames) {
        if (normalized == QLatin1StringView(entry.name)) return entry.role;
    }
    return std::nullopt;
}

OutputRole outputRoleFromNameOr(const QString &name, OutputRole fallback)
{
    return outputRoleFromName(name).value_or(fallback);
}

bool isOutputRoleName(const QString &name)
{
    return outputRoleFromName(name).has_value();
}

QStringList outputRoleNames()
{
    QStringList names;
    names.reserve(static_cast<qsizetype>(std::size(kRoleNames)));
    for (const auto &entry : kRoleNames) names.append(QString::fromLatin1(entry.name));
    return names;
}

} // namespace churchpresenter
