#include <QtTest/QTest>

#include "core/CommandBus.h"

#include <stdexcept>

using namespace churchpresenter;

class CommandBusTest final : public QObject {
    Q_OBJECT

private slots:
    void dispatchesRegisteredCommandAndAdvancesStateRevision();
    void rejectsMalformedCommandBeforeCallingHandler();
    void convertsHandlerExceptionIntoRejectedResult();
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

void CommandBusTest::rejectsMalformedCommandBeforeCallingHandler()
{
    CommandBus bus;
    bool handled = false;
    QVERIFY(bus.registerHandler(QStringLiteral("media.stop"),
                                [&handled](const Command &) {
        handled = true;
        return CommandResult{.accepted = true};
    }));

    const auto result = bus.dispatch(Command{
        .type = QStringLiteral("media.stop"),
        .source = QStringLiteral("remote"),
    });

    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("invalid_command"));
    QVERIFY(!handled);
    QCOMPARE(bus.stateRevision(), quint64{0});
}

void CommandBusTest::convertsHandlerExceptionIntoRejectedResult()
{
    CommandBus bus;
    QVERIFY(bus.registerHandler(QStringLiteral("integration.fail"),
                                [](const Command &) -> CommandResult {
        throw std::runtime_error("adapter offline");
    }));
    const Command command{
        .id = QStringLiteral("command-failure"),
        .type = QStringLiteral("integration.fail"),
        .source = QStringLiteral("automation"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    };

    bool threw = false;
    CommandResult result;
    try {
        result = bus.dispatch(command);
    } catch (...) {
        threw = true;
    }

    QVERIFY(!threw);
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("internal_error"));
    QCOMPARE(bus.stateRevision(), quint64{0});
}

QTEST_APPLESS_MAIN(CommandBusTest)
#include "CommandBusTest.moc"
