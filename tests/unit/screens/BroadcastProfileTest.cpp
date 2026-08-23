#include <QtTest/QTest>

#include "screens/BroadcastProfile.h"

using namespace churchpresenter;

class BroadcastProfileTest final : public QObject {
    Q_OBJECT

private slots:
    void roundTripsEveryBackgroundModeAndPreset();
    void rejectsUnknownBackgroundModesInsteadOfFallingBack();
    void keepsFallbackValuesForAbsentFields();
    void clampsSafeAreaAndRepairsInvalidChroma();
    void exposesTheAspectRatioOfEachPreset();
};

void BroadcastProfileTest::roundTripsEveryBackgroundModeAndPreset()
{
    for (const auto &mode : broadcastBackgroundModeNames()) {
        const auto parsed = broadcastBackgroundModeFromName(mode);
        QVERIFY(parsed.has_value());
        QCOMPARE(broadcastBackgroundModeName(*parsed), mode);
    }
    for (const auto &preset : broadcastAspectPresetNames()) {
        const auto parsed = broadcastAspectPresetFromName(preset);
        QVERIFY(parsed.has_value());
        QCOMPARE(broadcastAspectPresetName(*parsed), preset);
    }

    const BroadcastProfile profile{
        .screenFingerprint = QStringLiteral("hdmi-2"),
        .backgroundMode = BroadcastBackgroundMode::Transparent,
        .chromaColor = QStringLiteral("#123456"),
        .safeArea = QMarginsF(4.0, 3.0, 4.5, 2.5),
        .aspectPreset = BroadcastAspectPreset::Portrait,
        .showClock = true,
        .showLowerThird = false,
        .showAlerts = false,
        .showAudienceMessage = true,
    };

    QCOMPARE(broadcastProfileFromMap(broadcastProfileToMap(profile)), profile);
}

void BroadcastProfileTest::rejectsUnknownBackgroundModesInsteadOfFallingBack()
{
    QCOMPARE(broadcastBackgroundModeFromName(QStringLiteral("alpha")), std::nullopt);
    QCOMPARE(broadcastAspectPresetFromName(QStringLiteral("4:3")), std::nullopt);

    const BroadcastProfile fallback{.backgroundMode = BroadcastBackgroundMode::Transparent};
    const auto parsed = broadcastProfileFromMap(
        {{QStringLiteral("backgroundMode"), QStringLiteral("alpha")}}, fallback);
    QCOMPARE(parsed.backgroundMode, BroadcastBackgroundMode::Transparent);
}

void BroadcastProfileTest::keepsFallbackValuesForAbsentFields()
{
    const BroadcastProfile fallback{
        .screenFingerprint = QStringLiteral("hdmi-3"),
        .chromaColor = QStringLiteral("#0000ff"),
        .safeArea = QMarginsF(7.0, 7.0, 7.0, 7.0),
        .showClock = true,
    };

    const auto parsed = broadcastProfileFromMap(
        {{QStringLiteral("showClock"), false}}, fallback);
    QCOMPARE(parsed.screenFingerprint, QStringLiteral("hdmi-3"));
    QCOMPARE(parsed.chromaColor, QStringLiteral("#0000ff"));
    QCOMPARE(parsed.safeArea, QMarginsF(7.0, 7.0, 7.0, 7.0));
    QVERIFY(!parsed.showClock);
}

void BroadcastProfileTest::clampsSafeAreaAndRepairsInvalidChroma()
{
    const auto parsed = broadcastProfileFromMap({
        {QStringLiteral("safeAreaLeft"), 120.0},
        {QStringLiteral("safeAreaTop"), -5.0},
        {QStringLiteral("chromaColor"), QStringLiteral("verde-limão")},
    });
    QCOMPARE(parsed.safeArea.left(), 45.0);
    QCOMPARE(parsed.safeArea.top(), 0.0);
    QCOMPARE(parsed.chromaColor, BroadcastProfile{}.chromaColor);
    QVERIFY(isValidChromaColor(parsed.chromaColor));
}

void BroadcastProfileTest::exposesTheAspectRatioOfEachPreset()
{
    QCOMPARE(broadcastAspectRatio(BroadcastAspectPreset::Landscape), 16.0 / 9.0);
    QCOMPARE(broadcastAspectRatio(BroadcastAspectPreset::Portrait), 9.0 / 16.0);
}

QTEST_APPLESS_MAIN(BroadcastProfileTest)
#include "BroadcastProfileTest.moc"
