#include <QtTest/QTest>

#include "screens/OutputManager.h"

using namespace churchpresenter;

class OutputManagerTest final : public QObject {
    Q_OBJECT

private slots:
    void enablesUpToFiveDistinctConnectedOutputs();
    void refusesASixthOutputWithoutChangingExistingOutputs();
    void marksAnEnabledOutputMissingInsteadOfDeletingItsConfiguration();
    void reconnectsAMissingOutputWithTheSameFingerprint();
    void enablesEveryConnectedNonPrimaryScreenAtOnce();
    void assignsABibleTranslationToOneOutput();
    void assignsAStageRoleToOneOutput();
};

void OutputManagerTest::enablesUpToFiveDistinctConnectedOutputs()
{
    OutputManager manager;
    for (int index = 1; index <= 5; ++index) {
        const auto result = manager.enable(ScreenDescriptor{
            .id = QStringLiteral("screen-%1").arg(index),
            .fingerprint = QStringLiteral("display-%1").arg(index),
            .displayName = QStringLiteral("Display %1").arg(index),
            .connected = true,
        });
        QVERIFY(result.accepted);
    }

    QCOMPARE(manager.activeOutputs().size(), qsizetype{5});
}

void OutputManagerTest::refusesASixthOutputWithoutChangingExistingOutputs()
{
    OutputManager manager;
    for (int index = 1; index <= 5; ++index) {
        QVERIFY(manager.enable(ScreenDescriptor{
            .id = QStringLiteral("screen-%1").arg(index),
            .fingerprint = QStringLiteral("display-%1").arg(index),
            .connected = true,
        }).accepted);
    }

    const auto result = manager.enable(ScreenDescriptor{
        .id = QStringLiteral("screen-6"),
        .fingerprint = QStringLiteral("display-6"),
        .connected = true,
    });

    QVERIFY(!result.accepted);
    QCOMPARE(result.reason, EnableOutputResult::LimitReached);
    QCOMPARE(manager.activeOutputs().size(), qsizetype{5});
}

void OutputManagerTest::marksAnEnabledOutputMissingInsteadOfDeletingItsConfiguration()
{
    OutputManager manager;
    const ScreenDescriptor projector{
        .id = QStringLiteral("hdmi-1"),
        .fingerprint = QStringLiteral("acme-projector-serial-42"),
        .displayName = QStringLiteral("Projetor"),
        .connected = true,
    };
    QVERIFY(manager.enable(projector).accepted);

    manager.applyScreens({});

    QCOMPARE(manager.activeOutputs().size(), qsizetype{1});
    QVERIFY(manager.activeOutputs().front().enabled);
    QVERIFY(!manager.activeOutputs().front().connected);
    QCOMPARE(manager.activeOutputs().front().state, OutputConnectionState::Missing);
}

void OutputManagerTest::reconnectsAMissingOutputWithTheSameFingerprint()
{
    OutputManager manager;
    QVERIFY(manager.enable(ScreenDescriptor{
        .id = QStringLiteral("old-port"),
        .fingerprint = QStringLiteral("acme-projector-serial-42"),
        .connected = true,
    }).accepted);
    manager.applyScreens({});

    manager.applyScreens({ScreenDescriptor{
        .id = QStringLiteral("new-port"),
        .fingerprint = QStringLiteral("acme-projector-serial-42"),
        .displayName = QStringLiteral("Projetor HDMI"),
        .connected = true,
    }});

    QCOMPARE(manager.activeOutputs().size(), qsizetype{1});
    const auto &output = manager.activeOutputs().front();
    QVERIFY(output.connected);
    QCOMPARE(output.state, OutputConnectionState::Connected);
    QCOMPARE(output.screenId, QStringLiteral("new-port"));
}

void OutputManagerTest::enablesEveryConnectedNonPrimaryScreenAtOnce()
{
    OutputManager manager;
    const QVector<ScreenDescriptor> screens{
        {.id = QStringLiteral("integrated"), .fingerprint = QStringLiteral("main"),
         .primary = true, .connected = true},
        {.id = QStringLiteral("hdmi-1"), .fingerprint = QStringLiteral("dell"),
         .primary = false, .connected = true},
        {.id = QStringLiteral("hdmi-2"), .fingerprint = QStringLiteral("lg"),
         .primary = false, .connected = true},
        {.id = QStringLiteral("missing"), .fingerprint = QStringLiteral("offline"),
         .primary = false, .connected = false},
    };

    QCOMPARE(manager.enableAllAudienceScreens(screens), qsizetype{2});
    QCOMPARE(manager.activeOutputs().size(), qsizetype{2});
    QCOMPARE(manager.activeOutputs().at(0).screenFingerprint, QStringLiteral("dell"));
    QCOMPARE(manager.activeOutputs().at(1).screenFingerprint, QStringLiteral("lg"));
}

void OutputManagerTest::assignsABibleTranslationToOneOutput()
{
    OutputManager manager;
    QVERIFY(manager.enable({.id = QStringLiteral("projector"), .fingerprint = QStringLiteral("dell"),
                            .connected = true}).accepted);
    QVERIFY(manager.setBibleTranslation(QStringLiteral("dell"), QStringLiteral("naa")));
    QCOMPARE(manager.activeOutputs().front().bibleTranslationId, QStringLiteral("naa"));
    QVERIFY(!manager.setBibleTranslation(QStringLiteral("missing"), QStringLiteral("nvi")));
}

void OutputManagerTest::assignsAStageRoleToOneOutput()
{
    OutputManager manager;
    QVERIFY(manager.enable({.id = QStringLiteral("stage"), .fingerprint = QStringLiteral("lg"),
                            .connected = true}).accepted);
    QVERIFY(manager.setRole(QStringLiteral("lg"), OutputRole::Stage));
    QCOMPARE(manager.activeOutputs().front().role, OutputRole::Stage);
    QVERIFY(!manager.setRole(QStringLiteral("missing"), OutputRole::Audience));
}

QTEST_APPLESS_MAIN(OutputManagerTest)
#include "OutputManagerTest.moc"
