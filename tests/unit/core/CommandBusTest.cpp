#include <QtTest/QTest>

#include "core/CommandBus.h"

using namespace churchpresenter;

class CommandBusTest final : public QObject {
    Q_OBJECT

private slots:
    void dispatchesRegisteredCommandAndAdvancesStateRevision();
};

void CommandBusTest::dispatchesRegisteredCommandAndAdvancesStateRevision()
{
    CommandBus bus;
    bool handled = false;

    QVERIFY(bus.registerHandler(QStringLiteral("presentation.blackout"),
                                [&handled](const Command &command) {
        handled = command.payload.value(QStringLiteral("enabled")).toBool();
        return CommandResult{.accepted = true};
    }));

    const auto result = bus.dispatch(Command{
        .id = QStringLiteral("command-1"),
        .type = QStringLiteral("presentation.blackout"),
        .payload = {{QStringLiteral("enabled"), true}},
        .source = QStringLiteral("operator"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });

    QVERIFY(result.accepted);
    QVERIFY(handled);
    QCOMPARE(result.stateRevision, quint64{1});
    QCOMPARE(bus.stateRevision(), quint64{1});
}

QTEST_APPLESS_MAIN(CommandBusTest)
#include "CommandBusTest.moc"
