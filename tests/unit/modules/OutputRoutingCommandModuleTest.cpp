#include "modules/OutputRoutingCommandModule.h"

#include "screens/OutputRole.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class OutputRoutingCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void routesOutputChangesAndSupportsUndo();
    void rejectsMissingOutputsAndInvalidRoles();
    void acceptsEveryDeclaredOutputRole();
    void appliesPartialBroadcastProfileChangesWithUndo();
    void rejectsInvalidBroadcastProfileValues();
};

void OutputRoutingCommandModuleTest::routesOutputChangesAndSupportsUndo()
{
    CommandBus commands;
    EventBus events;
    UndoManager undo;
    QVariantMap output{{QStringLiteral("fingerprint"), QStringLiteral("screen-2")},
                       {QStringLiteral("enabled"), false},
                       {QStringLiteral("role"), QStringLiteral("audience")},
                       {QStringLiteral("mediaEnabled"), true}};
    OutputRoutingCommandModule module(commands, events, {
        .output = [&output](const QString &fingerprint) {
            return output.value(QStringLiteral("fingerprint")).toString() == fingerprint
                ? output : QVariantMap{};
        },
        .setEnabled = [&output](const QString &, bool enabled) {
            output.insert(QStringLiteral("enabled"), enabled); return true;
        },
        .setRole = [&output](const QString &, const QString &role) {
            output.insert(QStringLiteral("role"), role); return true;
        },
        .setMediaEnabled = [&output](const QString &, bool enabled) {
            output.insert(QStringLiteral("mediaEnabled"), enabled); return true;
        },
    }, &undo);
    QSignalSpy eventSpy(&events, &EventBus::eventPublished);

    QVERIFY(module.requestEnabled(QStringLiteral("screen-2"), true).accepted);
    QVERIFY(output.value(QStringLiteral("enabled")).toBool());
    QCOMPARE(eventSpy.count(), 1);
    QVERIFY(undo.undo().success);
    QVERIFY(!output.value(QStringLiteral("enabled")).toBool());
    QVERIFY(undo.redo().success);
    QVERIFY(output.value(QStringLiteral("enabled")).toBool());
}

void OutputRoutingCommandModuleTest::rejectsMissingOutputsAndInvalidRoles()
{
    CommandBus commands;
    EventBus events;
    OutputRoutingCommandModule module(commands, events, {});
    QCOMPARE(module.requestEnabled(QString{}, true).errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestRole(QStringLiteral("missing"), QStringLiteral("projector"))
                 .errorCode,
             QStringLiteral("invalid_payload"));
}

void OutputRoutingCommandModuleTest::acceptsEveryDeclaredOutputRole()
{
    CommandBus commands;
    EventBus events;
    QVariantMap output{{QStringLiteral("fingerprint"), QStringLiteral("screen-2")},
                       {QStringLiteral("role"), QStringLiteral("audience")}};
    OutputRoutingCommandModule module(commands, events, {
        .output = [&output](const QString &) { return output; },
        .setRole = [&output](const QString &, const QString &role) {
            output.insert(QStringLiteral("role"), role); return true;
        },
    });

    for (const auto &role : outputRoleNames()) {
        const auto result = module.requestRole(QStringLiteral("screen-2"), role);
        QVERIFY2(result.accepted, qPrintable(role));
        QCOMPARE(output.value(QStringLiteral("role")).toString(), role);
    }
}

namespace {

struct BroadcastFixture {
    QVariantMap profile{{QStringLiteral("backgroundMode"), QStringLiteral("chroma")},
                        {QStringLiteral("chromaColor"), QStringLiteral("#00b140")},
                        {QStringLiteral("showLowerThird"), true}};

    OutputRoutingCommandModule::Actions actions()
    {
        return {
            .broadcastProfile = [this](const QString &) { return profile; },
            .setBroadcastProfile = [this](const QString &, const QVariantMap &changes) {
                for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
                    profile.insert(it.key(), it.value());
                }
                return true;
            },
        };
    }
};

} // namespace

void OutputRoutingCommandModuleTest::appliesPartialBroadcastProfileChangesWithUndo()
{
    CommandBus commands;
    EventBus events;
    UndoManager undo;
    BroadcastFixture fixture;
    OutputRoutingCommandModule module(commands, events, fixture.actions(), &undo);
    QSignalSpy eventSpy(&events, &EventBus::eventPublished);

    const auto result = module.requestBroadcastProfile(
        QStringLiteral("hdmi-2"),
        {{QStringLiteral("backgroundMode"), QStringLiteral("transparent")},
         {QStringLiteral("showLowerThird"), false}});
    QVERIFY2(result.accepted, qPrintable(result.message));
    QCOMPARE(fixture.profile.value(QStringLiteral("backgroundMode")).toString(),
             QStringLiteral("transparent"));
    QVERIFY(!fixture.profile.value(QStringLiteral("showLowerThird")).toBool());
    // A cor de chroma não informada permanece intocada.
    QCOMPARE(fixture.profile.value(QStringLiteral("chromaColor")).toString(),
             QStringLiteral("#00b140"));
    QCOMPARE(eventSpy.count(), 1);

    QVERIFY(undo.undo().success);
    QCOMPARE(fixture.profile.value(QStringLiteral("backgroundMode")).toString(),
             QStringLiteral("chroma"));
    QVERIFY(fixture.profile.value(QStringLiteral("showLowerThird")).toBool());
    QVERIFY(undo.redo().success);
    QCOMPARE(fixture.profile.value(QStringLiteral("backgroundMode")).toString(),
             QStringLiteral("transparent"));
}

void OutputRoutingCommandModuleTest::rejectsInvalidBroadcastProfileValues()
{
    CommandBus commands;
    EventBus events;
    BroadcastFixture fixture;
    OutputRoutingCommandModule module(commands, events, fixture.actions());

    QCOMPARE(module.requestBroadcastProfile(QString{}, {{QStringLiteral("showAlerts"), false}})
                 .errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestBroadcastProfile(
                 QStringLiteral("hdmi-2"),
                 {{QStringLiteral("backgroundMode"), QStringLiteral("alpha")}}).errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestBroadcastProfile(
                 QStringLiteral("hdmi-2"),
                 {{QStringLiteral("aspectPreset"), QStringLiteral("4:3")}}).errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestBroadcastProfile(
                 QStringLiteral("hdmi-2"),
                 {{QStringLiteral("chromaColor"), QStringLiteral("verde")}}).errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestBroadcastProfile(QStringLiteral("hdmi-2"), {}).errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(fixture.profile.value(QStringLiteral("backgroundMode")).toString(),
             QStringLiteral("chroma"));
}

QTEST_APPLESS_MAIN(OutputRoutingCommandModuleTest)
#include "OutputRoutingCommandModuleTest.moc"
