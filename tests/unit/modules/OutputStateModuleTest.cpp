#include "modules/OutputStateModule.h"

#include <QTest>

using namespace churchpresenter;

namespace {

QString legacyEntry(const QStringList &fields)
{
    return fields.join(OutputStateModule::FieldSeparator);
}

QVector<ScreenDescriptor> twoScreens()
{
    return {
        {.id = "integrated", .fingerprint = "internal", .displayName = "Operador",
         .geometry = {0, 0, 1470, 956}, .primary = true, .connected = true},
        {.id = "projector", .fingerprint = "dell", .displayName = "Projetor",
         .geometry = {1470, 0, 1920, 1080}, .connected = true},
    };
}

} // namespace

class OutputStateModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void loadsConfigurationsWrittenBeforeTheBroadcastRole();
    void roundTripsEveryRoleThroughTheSettingsFormat();
    void keepsOutputsWithAnUnknownRoleAsAudience();
    void rejectsUnknownRoleNamesWhenChangingAnOutput();
    void describesScreensWithTheirOutputSettings();
    void describesOneWindowPerConnectedExternalOutput();
};

void OutputStateModuleTest::loadsConfigurationsWrittenBeforeTheBroadcastRole()
{
    OutputStateModule module;
    module.restore({
        legacyEntry({"projector", "dell", "Projetor", "nvi", "audience", "1"}),
        legacyEntry({"confidence", "hdmi-2", "Palco", "naa", "stage", "0"}),
        // 0.9 wrote only the four first fields.
        legacyEntry({"tv", "hdmi-3", "TV", "acf"}),
    });

    const auto outputs = module.activeOutputs();
    QCOMPARE(outputs.size(), 3);
    QCOMPARE(outputs[0].role, OutputRole::Audience);
    QVERIFY(outputs[0].mediaEnabled);
    QCOMPARE(outputs[1].role, OutputRole::Stage);
    QVERIFY(!outputs[1].mediaEnabled);
    QCOMPARE(outputs[2].role, OutputRole::Audience);
    QCOMPARE(outputs[2].bibleTranslationId, QStringLiteral("acf"));
    QVERIFY(outputs[2].mediaEnabled);
}

void OutputStateModuleTest::roundTripsEveryRoleThroughTheSettingsFormat()
{
    OutputStateModule source;
    const auto roles = outputRoleNames();
    for (qsizetype index = 0; index < roles.size(); ++index) {
        const auto fingerprint = QStringLiteral("screen-%1").arg(index);
        source.manager().restore(OutputDescriptor{
            .screenId = QStringLiteral("hdmi-%1").arg(index),
            .screenFingerprint = fingerprint,
            .displayName = QStringLiteral("Saída %1").arg(index),
            .enabled = true,
        });
        QVERIFY(source.setRole(fingerprint, roles[index]));
    }

    OutputStateModule restored;
    restored.restore(source.serialize());

    QCOMPARE(restored.serialize(), source.serialize());
    QCOMPARE(restored.activeOutputs().size(), roles.size());
    for (qsizetype index = 0; index < roles.size(); ++index) {
        QCOMPARE(outputRoleName(restored.activeOutputs()[index].role), roles[index]);
    }
}

void OutputStateModuleTest::keepsOutputsWithAnUnknownRoleAsAudience()
{
    OutputStateModule module;
    QTest::ignoreMessage(QtWarningMsg,
                         "Unknown persisted output role, falling back to audience: \"projector\"");
    module.restore({legacyEntry({"tv", "hdmi-3", "TV", "", "projector", "1"})});

    QCOMPARE(module.activeOutputs().size(), 1);
    QCOMPARE(module.activeOutputs()[0].role, OutputRole::Audience);
}

void OutputStateModuleTest::rejectsUnknownRoleNamesWhenChangingAnOutput()
{
    OutputStateModule module;
    module.restore({legacyEntry({"tv", "hdmi-3", "TV", "", "audience", "1"})});

    QVERIFY(!module.setRole(QStringLiteral("hdmi-3"), QStringLiteral("projector")));
    QVERIFY(module.setRole(QStringLiteral("hdmi-3"), QStringLiteral("broadcast")));
    QCOMPARE(module.activeOutputs()[0].role, OutputRole::Broadcast);
}

void OutputStateModuleTest::describesScreensWithTheirOutputSettings()
{
    OutputStateModule module;
    const auto screens = twoScreens();
    module.applyScreens(screens);
    QVERIFY(module.enable(screens[1]).accepted);
    QVERIFY(module.setRole(QStringLiteral("dell"), QStringLiteral("broadcast")));
    QVERIFY(module.setDisplayName(QStringLiteral("dell"), QStringLiteral("Transmissão")));
    QVERIFY(module.setMediaEnabled(QStringLiteral("dell"), false));
    QVERIFY(module.setBibleTranslation(QStringLiteral("dell"), QStringLiteral("nvi")));

    const auto described = module.describeScreens(screens);
    QCOMPARE(described.size(), 2);
    const auto operatorScreen = described[0].toMap();
    QVERIFY(!operatorScreen.value(QStringLiteral("selected")).toBool());
    QVERIFY(operatorScreen.value(QStringLiteral("primary")).toBool());
    QCOMPARE(operatorScreen.value(QStringLiteral("role")).toString(),
             QStringLiteral("audience"));

    const auto broadcast = described[1].toMap();
    QVERIFY(broadcast.value(QStringLiteral("selected")).toBool());
    QCOMPARE(broadcast.value(QStringLiteral("name")).toString(), QStringLiteral("Transmissão"));
    QCOMPARE(broadcast.value(QStringLiteral("role")).toString(), QStringLiteral("broadcast"));
    QCOMPARE(broadcast.value(QStringLiteral("bibleTranslationId")).toString(),
             QStringLiteral("nvi"));
    QVERIFY(!broadcast.value(QStringLiteral("mediaEnabled")).toBool());
}

void OutputStateModuleTest::describesOneWindowPerConnectedExternalOutput()
{
    OutputStateModule module;
    const auto screens = twoScreens();
    module.applyScreens(screens);
    QVERIFY(module.enable(screens[1]).accepted);
    QVERIFY(module.setRole(QStringLiteral("dell"), QStringLiteral("confidence")));

    const auto windows = module.describeOutputWindows(screens);
    QCOMPARE(windows.size(), 1);
    const auto window = windows[0].toMap();
    QCOMPARE(window.value(QStringLiteral("identifier")).toInt(), 1);
    QCOMPARE(window.value(QStringLiteral("screenIndex")).toInt(), 1);
    QCOMPARE(window.value(QStringLiteral("screenWidth")).toInt(), 1920);
    QCOMPARE(window.value(QStringLiteral("role")).toString(), QStringLiteral("confidence"));

    module.disable(QStringLiteral("dell"));
    QVERIFY(module.describeOutputWindows(screens).isEmpty());
}

QTEST_APPLESS_MAIN(OutputStateModuleTest)
#include "OutputStateModuleTest.moc"
