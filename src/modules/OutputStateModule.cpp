#include "modules/OutputStateModule.h"

#include <QDebug>

#include <algorithm>

namespace churchpresenter {
namespace {

OutputRole restoreRole(const QString &name)
{
    const auto role = outputRoleFromName(name);
    if (!role.has_value() && !name.isEmpty()) {
        qWarning() << "Unknown persisted output role, falling back to audience:" << name;
    }
    return role.value_or(OutputRole::Audience);
}

} // namespace

OutputManager &OutputStateModule::manager() { return m_outputs; }
const OutputManager &OutputStateModule::manager() const { return m_outputs; }

const QVector<OutputDescriptor> &OutputStateModule::activeOutputs() const
{
    return m_outputs.activeOutputs();
}

void OutputStateModule::restore(const QStringList &serialized)
{
    for (const auto &entry : serialized) {
        const auto values = entry.split(FieldSeparator);
        if (values.size() < 2) continue;
        m_outputs.restore(OutputDescriptor{
            .screenId = values[0],
            .screenFingerprint = values[1],
            .displayName = values.size() > 2 ? values[2] : values[0],
            .enabled = true,
            .role = values.size() > 4 ? restoreRole(values[4]) : OutputRole::Audience,
            .bibleTranslationId = values.size() > 3 ? values[3] : QString{},
            .mediaEnabled = values.size() <= 5 || values[5] != QStringLiteral("0"),
        });
    }
}

QStringList OutputStateModule::serialize() const
{
    QStringList serialized;
    serialized.reserve(m_outputs.activeOutputs().size());
    for (const auto &output : m_outputs.activeOutputs()) {
        serialized.append(QStringList{output.screenId, output.screenFingerprint,
                                      output.displayName, output.bibleTranslationId,
                                      outputRoleName(output.role),
                                      output.mediaEnabled ? QStringLiteral("1")
                                                          : QStringLiteral("0")}
                              .join(FieldSeparator));
    }
    return serialized;
}

void OutputStateModule::applyScreens(const QVector<ScreenDescriptor> &screens)
{
    m_outputs.applyScreens(screens);
}

EnableOutputResult OutputStateModule::enable(const ScreenDescriptor &screen)
{
    return m_outputs.enable(screen);
}

void OutputStateModule::disable(const QString &screenFingerprint)
{
    QVector<OutputDescriptor> retained;
    retained.reserve(m_outputs.activeOutputs().size());
    for (const auto &output : m_outputs.activeOutputs()) {
        if (output.screenFingerprint != screenFingerprint) retained.append(output);
    }
    m_outputs.replaceOutputs(retained);
}

qsizetype OutputStateModule::enableAllAudienceScreens(const QVector<ScreenDescriptor> &screens)
{
    return m_outputs.enableAllAudienceScreens(screens);
}

bool OutputStateModule::setRole(const QString &screenFingerprint, const QString &role)
{
    const auto parsed = outputRoleFromName(role);
    if (!parsed.has_value()) return false;
    return m_outputs.setRole(screenFingerprint, *parsed);
}

bool OutputStateModule::setBibleTranslation(const QString &screenFingerprint,
                                            const QString &translationId)
{
    return m_outputs.setBibleTranslation(screenFingerprint, translationId);
}

bool OutputStateModule::setMediaEnabled(const QString &screenFingerprint, bool enabled)
{
    return m_outputs.setMediaEnabled(screenFingerprint, enabled);
}

bool OutputStateModule::setDisplayName(const QString &screenFingerprint,
                                       const QString &displayName)
{
    return m_outputs.setDisplayName(screenFingerprint, displayName);
}

void OutputStateModule::setBroadcastProfiles(const QVector<BroadcastProfile> &profiles)
{
    m_broadcastProfiles.clear();
    for (const auto &profile : profiles) {
        if (profile.screenFingerprint.isEmpty()) continue;
        m_broadcastProfiles.insert(profile.screenFingerprint, normalizedBroadcastProfile(profile));
    }
}

BroadcastProfile OutputStateModule::broadcastProfile(const QString &screenFingerprint) const
{
    const auto found = m_broadcastProfiles.constFind(screenFingerprint);
    if (found != m_broadcastProfiles.cend()) return *found;
    BroadcastProfile profile;
    profile.screenFingerprint = screenFingerprint;
    return profile;
}

BroadcastProfile OutputStateModule::mergeBroadcastProfile(const QString &screenFingerprint,
                                                          const QVariantMap &changes)
{
    auto merged = broadcastProfileFromMap(changes, broadcastProfile(screenFingerprint));
    merged.screenFingerprint = screenFingerprint;
    m_broadcastProfiles.insert(screenFingerprint, merged);
    return merged;
}

QVariantList OutputStateModule::describeScreens(const QVector<ScreenDescriptor> &screens) const
{
    const auto &outputs = m_outputs.activeOutputs();
    QVariantList result;
    result.reserve(screens.size());
    for (const auto &screen : screens) {
        const auto output = std::find_if(outputs.cbegin(), outputs.cend(),
                                         [&screen](const OutputDescriptor &candidate) {
            return candidate.screenFingerprint == screen.fingerprint;
        });
        const bool selected = output != outputs.cend();
        QVariantMap item{
            {QStringLiteral("id"), screen.fingerprint},
            {QStringLiteral("screenName"), screen.id},
            {QStringLiteral("name"), screen.displayName},
            {QStringLiteral("selected"), selected},
            {QStringLiteral("primary"), screen.primary},
            {QStringLiteral("fingerprint"), screen.fingerprint},
            {QStringLiteral("bibleTranslationId"),
             selected ? output->bibleTranslationId : QString{}},
            {QStringLiteral("role"),
             outputRoleName(selected ? output->role : OutputRole::Audience)},
            {QStringLiteral("mediaEnabled"), selected ? output->mediaEnabled : true},
        };
        if (selected && !output->displayName.isEmpty()) {
            item.insert(QStringLiteral("name"), output->displayName);
        }
        item.insert(QStringLiteral("broadcast"),
                    broadcastProfileToMap(broadcastProfile(screen.fingerprint)));
        result.append(item);
    }
    return result;
}

QVariantList OutputStateModule::describeOutputWindows(
    const QVector<ScreenDescriptor> &screens) const
{
    QVariantList result;
    int identifier = 1;
    for (const auto &placement : routeOutputs(m_outputs.activeOutputs(), screens)) {
        result.append(QVariantMap{
            {QStringLiteral("id"), placement.screenFingerprint},
            {QStringLiteral("screenName"), placement.screenId},
            {QStringLiteral("displayName"), placement.displayName},
            {QStringLiteral("screenIndex"), placement.screenIndex},
            {QStringLiteral("screenX"), placement.geometry.x()},
            {QStringLiteral("screenY"), placement.geometry.y()},
            {QStringLiteral("screenWidth"), placement.geometry.width()},
            {QStringLiteral("screenHeight"), placement.geometry.height()},
            {QStringLiteral("identifier"), identifier++},
            {QStringLiteral("bibleTranslationId"), placement.bibleTranslationId},
            {QStringLiteral("role"), outputRoleName(placement.role)},
            {QStringLiteral("mediaEnabled"), placement.mediaEnabled},
            {QStringLiteral("broadcast"),
             broadcastProfileToMap(broadcastProfile(placement.screenFingerprint))},
        });
    }
    return result;
}

} // namespace churchpresenter
