#include <QtTest/QTest>

#include "screens/OutputRouting.h"

using namespace churchpresenter;

class OutputRoutingTest final : public QObject {
    Q_OBJECT

private slots:
    void routesOnlySelectedExternalScreensUsingPhysicalGeometry();
    void preservesTheConfiguredRoleForEachOutput();
};

void OutputRoutingTest::routesOnlySelectedExternalScreensUsingPhysicalGeometry()
{
    const QVector<ScreenDescriptor> screens{
        {.id = "integrated", .fingerprint = "internal", .geometry = {0, 0, 1470, 956},
         .primary = true, .connected = true},
        {.id = "ultrawide", .fingerprint = "lg", .geometry = {-2560, 0, 2560, 1080},
         .connected = true},
        {.id = "projector", .fingerprint = "dell", .geometry = {1470, 0, 1920, 1080},
         .connected = true},
    };
    const QVector<OutputDescriptor> outputs{
        {.screenId = "integrated", .screenFingerprint = "internal", .displayName = "Operador",
         .enabled = true, .connected = true},
        {.screenId = "ultrawide", .screenFingerprint = "lg", .displayName = "LG",
         .enabled = true, .connected = true, .bibleTranslationId = "nvi"},
        {.screenId = "projector", .screenFingerprint = "dell", .displayName = "DELL",
         .enabled = true, .connected = true, .bibleTranslationId = "naa"},
    };

    const auto placements = routeAudienceOutputs(outputs, screens);

    QCOMPARE(placements.size(), 2);
    QCOMPARE(placements[0].screenFingerprint, QStringLiteral("lg"));
    QCOMPARE(placements[0].screenIndex, 1);
    QCOMPARE(placements[0].geometry, QRect(-2560, 0, 2560, 1080));
    QCOMPARE(placements[0].bibleTranslationId, QStringLiteral("nvi"));
    QCOMPARE(placements[1].screenFingerprint, QStringLiteral("dell"));
    QCOMPARE(placements[1].screenIndex, 2);
    QCOMPARE(placements[1].geometry, QRect(1470, 0, 1920, 1080));
    QCOMPARE(placements[1].bibleTranslationId, QStringLiteral("naa"));
}

void OutputRoutingTest::preservesTheConfiguredRoleForEachOutput()
{
    const QVector<ScreenDescriptor> screens{
        {.id = "stage", .fingerprint = "lg", .geometry = {0, 0, 1920, 1080}, .connected = true},
        {.id = "audience", .fingerprint = "dell", .geometry = {1920, 0, 1920, 1080}, .connected = true},
    };
    const QVector<OutputDescriptor> outputs{
        {.screenId = "stage", .screenFingerprint = "lg", .displayName = "Palco",
         .enabled = true, .connected = true, .role = OutputRole::Stage},
        {.screenId = "audience", .screenFingerprint = "dell", .displayName = "Público",
         .enabled = true, .connected = true, .role = OutputRole::Audience},
    };

    const auto placements = routeOutputs(outputs, screens);

    QCOMPARE(placements.size(), 2);
    QCOMPARE(placements[0].role, OutputRole::Stage);
    QCOMPARE(placements[1].role, OutputRole::Audience);
}

QTEST_APPLESS_MAIN(OutputRoutingTest)
#include "OutputRoutingTest.moc"
