#include "screens/OutputManager.h"

#include <algorithm>

namespace churchpresenter {

EnableOutputResult OutputManager::enable(const ScreenDescriptor &screen)
{
    if (!screen.connected) {
        return {.accepted = false, .reason = EnableOutputResult::ScreenDisconnected};
    }

    const auto existing = std::find_if(m_outputs.cbegin(), m_outputs.cend(), [&](const OutputDescriptor &output) {
        return output.screenFingerprint == screen.fingerprint;
    });

    if (existing != m_outputs.cend()) {
        auto &output = m_outputs[std::distance(m_outputs.cbegin(), existing)];
        output.screenId = screen.id;
        output.displayName = screen.displayName;
        output.connected = true;
        output.state = OutputConnectionState::Connected;
        return {.accepted = true};
    }

    if (m_outputs.size() >= MaximumOutputs) {
        return {.accepted = false, .reason = EnableOutputResult::LimitReached};
    }

    m_outputs.append(OutputDescriptor{
        .screenId = screen.id,
        .screenFingerprint = screen.fingerprint,
        .displayName = screen.displayName,
        .enabled = true,
        .connected = true,
        .role = OutputRole::Audience,
        .state = OutputConnectionState::Connected,
    });

    return {.accepted = true};
}

qsizetype OutputManager::enableAllAudienceScreens(const QVector<ScreenDescriptor> &screens)
{
    qsizetype enabledCount = 0;
    for (const auto &screen : screens) {
        if (screen.primary || !screen.connected) continue;
        if (enable(screen).accepted) ++enabledCount;
    }
    return enabledCount;
}

bool OutputManager::setBibleTranslation(
    const QString &screenFingerprint, const QString &translationId)
{
    const auto output = std::find_if(m_outputs.begin(), m_outputs.end(), [&](const auto &candidate) {
        return candidate.screenFingerprint == screenFingerprint;
    });
    if (output == m_outputs.end()) return false;
    output->bibleTranslationId = translationId;
    return true;
}

bool OutputManager::setRole(const QString &screenFingerprint, OutputRole role)
{
    const auto output = std::find_if(m_outputs.begin(), m_outputs.end(), [&](const auto &candidate) {
        return candidate.screenFingerprint == screenFingerprint;
    });
    if (output == m_outputs.end()) return false;
    output->role = role;
    return true;
}

void OutputManager::applyScreens(const QVector<ScreenDescriptor> &screens)
{
    for (auto &output : m_outputs) {
        const auto matchingScreen = std::find_if(screens.cbegin(), screens.cend(), [&](const ScreenDescriptor &screen) {
            return screen.connected && screen.fingerprint == output.screenFingerprint;
        });

        if (matchingScreen == screens.cend()) {
            output.connected = false;
            output.state = OutputConnectionState::Missing;
            continue;
        }

        output.screenId = matchingScreen->id;
        output.displayName = matchingScreen->displayName;
        output.connected = true;
        output.state = OutputConnectionState::Connected;
    }
}

void OutputManager::replaceOutputs(const QVector<OutputDescriptor> &outputs)
{
    m_outputs = outputs;
}

void OutputManager::restore(const OutputDescriptor &output)
{
    if (m_outputs.size() < MaximumOutputs) {
        m_outputs.append(output);
    }
}

const QVector<OutputDescriptor> &OutputManager::activeOutputs() const
{
    return m_outputs;
}

} // namespace churchpresenter
