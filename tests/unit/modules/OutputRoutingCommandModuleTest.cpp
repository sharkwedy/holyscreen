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

QTEST_APPLESS_MAIN(OutputRoutingCommandModuleTest)
#include "OutputRoutingCommandModuleTest.moc"
