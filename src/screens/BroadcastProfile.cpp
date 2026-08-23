#include "screens/BroadcastProfile.h"

#include <QColor>

#include <algorithm>

namespace churchpresenter {
namespace {

constexpr double MaximumSafeAreaPercent = 45.0;

double clampSafeArea(double value)
{
    if (!std::isfinite(value) || value < 0.0) return 0.0;
    return std::min(value, MaximumSafeAreaPercent);
}

double readMargin(const QVariantMap &map, const QString &key, double fallback)
{
    if (!map.contains(key)) return fallback;
    bool ok = false;
    const auto value = map.value(key).toDouble(&ok);
    return ok ? clampSafeArea(value) : fallback;
}

bool readFlag(const QVariantMap &map, const QString &key, bool fallback)
{
    return map.contains(key) ? map.value(key).toBool() : fallback;
}

} // namespace

QString broadcastBackgroundModeName(BroadcastBackgroundMode mode)
{
    return mode == BroadcastBackgroundMode::Transparent ? QStringLiteral("transparent")
                                                        : QStringLiteral("chroma");
}

std::optional<BroadcastBackgroundMode> broadcastBackgroundModeFromName(const QString &name)
{
    const auto normalized = name.trimmed().toLower();
    if (normalized == QStringLiteral("transparent")) return BroadcastBackgroundMode::Transparent;
    if (normalized == QStringLiteral("chroma")) return BroadcastBackgroundMode::Chroma;
    return std::nullopt;
}

QStringList broadcastBackgroundModeNames()
{
    return {QStringLiteral("transparent"), QStringLiteral("chroma")};
}

QString broadcastAspectPresetName(BroadcastAspectPreset preset)
{
    return preset == BroadcastAspectPreset::Portrait ? QStringLiteral("9:16")
                                                     : QStringLiteral("16:9");
}

std::optional<BroadcastAspectPreset> broadcastAspectPresetFromName(const QString &name)
{
    const auto normalized = name.trimmed();
    if (normalized == QStringLiteral("16:9")) return BroadcastAspectPreset::Landscape;
    if (normalized == QStringLiteral("9:16")) return BroadcastAspectPreset::Portrait;
    return std::nullopt;
}

QStringList broadcastAspectPresetNames()
{
    return {QStringLiteral("16:9"), QStringLiteral("9:16")};
}

double broadcastAspectRatio(BroadcastAspectPreset preset)
{
    return preset == BroadcastAspectPreset::Portrait ? 9.0 / 16.0 : 16.0 / 9.0;
}

bool isValidChromaColor(const QString &color)
{
    return QColor::isValidColorName(color);
}

BroadcastProfile normalizedBroadcastProfile(const BroadcastProfile &profile)
{
    BroadcastProfile normalized = profile;
    normalized.screenFingerprint = profile.screenFingerprint.trimmed();
    if (!isValidChromaColor(normalized.chromaColor)) {
        normalized.chromaColor = BroadcastProfile{}.chromaColor;
    }
    normalized.safeArea = QMarginsF(clampSafeArea(profile.safeArea.left()),
                                    clampSafeArea(profile.safeArea.top()),
                                    clampSafeArea(profile.safeArea.right()),
                                    clampSafeArea(profile.safeArea.bottom()));
    return normalized;
}

QVariantMap broadcastProfileToMap(const BroadcastProfile &profile)
{
    return {
        {QStringLiteral("screenFingerprint"), profile.screenFingerprint},
        {QStringLiteral("backgroundMode"), broadcastBackgroundModeName(profile.backgroundMode)},
        {QStringLiteral("chromaColor"), profile.chromaColor},
        {QStringLiteral("safeAreaLeft"), profile.safeArea.left()},
        {QStringLiteral("safeAreaTop"), profile.safeArea.top()},
        {QStringLiteral("safeAreaRight"), profile.safeArea.right()},
        {QStringLiteral("safeAreaBottom"), profile.safeArea.bottom()},
        {QStringLiteral("aspectPreset"), broadcastAspectPresetName(profile.aspectPreset)},
        {QStringLiteral("aspectRatio"), broadcastAspectRatio(profile.aspectPreset)},
        {QStringLiteral("showClock"), profile.showClock},
        {QStringLiteral("showLowerThird"), profile.showLowerThird},
        {QStringLiteral("showAlerts"), profile.showAlerts},
        {QStringLiteral("showAudienceMessage"), profile.showAudienceMessage},
    };
}

BroadcastProfile broadcastProfileFromMap(const QVariantMap &map, const BroadcastProfile &fallback)
{
    BroadcastProfile profile = fallback;
    if (map.contains(QStringLiteral("screenFingerprint"))) {
        profile.screenFingerprint = map.value(QStringLiteral("screenFingerprint")).toString();
    }
    if (const auto mode = broadcastBackgroundModeFromName(
            map.value(QStringLiteral("backgroundMode")).toString())) {
        profile.backgroundMode = *mode;
    }
    const auto chroma = map.value(QStringLiteral("chromaColor")).toString().trimmed();
    if (isValidChromaColor(chroma)) profile.chromaColor = chroma;
    if (const auto preset = broadcastAspectPresetFromName(
            map.value(QStringLiteral("aspectPreset")).toString())) {
        profile.aspectPreset = *preset;
    }
    profile.safeArea = QMarginsF(
        readMargin(map, QStringLiteral("safeAreaLeft"), fallback.safeArea.left()),
        readMargin(map, QStringLiteral("safeAreaTop"), fallback.safeArea.top()),
        readMargin(map, QStringLiteral("safeAreaRight"), fallback.safeArea.right()),
        readMargin(map, QStringLiteral("safeAreaBottom"), fallback.safeArea.bottom()));
    profile.showClock = readFlag(map, QStringLiteral("showClock"), fallback.showClock);
    profile.showLowerThird = readFlag(map, QStringLiteral("showLowerThird"),
                                      fallback.showLowerThird);
    profile.showAlerts = readFlag(map, QStringLiteral("showAlerts"), fallback.showAlerts);
    profile.showAudienceMessage = readFlag(map, QStringLiteral("showAudienceMessage"),
                                           fallback.showAudienceMessage);
    return normalizedBroadcastProfile(profile);
}

bool operator==(const BroadcastProfile &left, const BroadcastProfile &right)
{
    return left.screenFingerprint == right.screenFingerprint
        && left.backgroundMode == right.backgroundMode
        && left.chromaColor == right.chromaColor
        && left.safeArea == right.safeArea
        && left.aspectPreset == right.aspectPreset
        && left.showClock == right.showClock
        && left.showLowerThird == right.showLowerThird
        && left.showAlerts == right.showAlerts
        && left.showAudienceMessage == right.showAudienceMessage;
}

} // namespace churchpresenter
