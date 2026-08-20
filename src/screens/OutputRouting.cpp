#include "screens/OutputRouting.h"

#include <algorithm>

namespace churchpresenter {

QVector<OutputPlacement> routeAudienceOutputs(
    const QVector<OutputDescriptor> &outputs,
    const QVector<ScreenDescriptor> &screens)
{
    QVector<OutputPlacement> placements;
    for (const auto &output : outputs) {
        if (!output.enabled || !output.connected || output.role != OutputRole::Audience) continue;
        const auto screen = std::find_if(screens.cbegin(), screens.cend(), [&](const auto &candidate) {
            return candidate.connected && candidate.fingerprint == output.screenFingerprint;
        });
        if (screen == screens.cend() || screen->primary) continue;
        placements.append(OutputPlacement{
            .screenFingerprint = output.screenFingerprint,
            .screenId = screen->id,
            .displayName = output.displayName,
            .screenIndex = std::distance(screens.cbegin(), screen),
            .geometry = screen->geometry,
            .bibleTranslationId = output.bibleTranslationId,
            .role = output.role,
        });
    }
    return placements;
}

QVector<OutputPlacement> routeOutputs(
    const QVector<OutputDescriptor> &outputs,
    const QVector<ScreenDescriptor> &screens)
{
    QVector<OutputPlacement> placements;
    for (const auto &output : outputs) {
        if (!output.enabled || !output.connected) continue;
        const auto screen = std::find_if(screens.cbegin(), screens.cend(), [&](const auto &candidate) {
            return candidate.connected && candidate.fingerprint == output.screenFingerprint;
        });
        if (screen == screens.cend() || screen->primary) continue;
        placements.append(OutputPlacement{
            .screenFingerprint = output.screenFingerprint,
            .screenId = screen->id,
            .displayName = output.displayName,
            .screenIndex = std::distance(screens.cbegin(), screen),
            .geometry = screen->geometry,
            .bibleTranslationId = output.bibleTranslationId,
            .role = output.role,
        });
    }
    return placements;
}

} // namespace churchpresenter
