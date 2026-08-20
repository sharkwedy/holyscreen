#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "modules/OutputModule.h"

using namespace churchpresenter;

class OutputModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void ownsBlackoutStateAndPublishesCorrelatedEvent();
};

void OutputModuleTest::ownsBlackoutStateAndPublishesCorrelatedEvent()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();
    CommandBus commandBus;
    EventBus eventBus;
    OutputModule module(commandBus, eventBus);
    QSignalSpy changedSpy(&module, &OutputModule::blackoutChanged);
    QSignalSpy commandSpy(&commandBus, &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&eventBus, &EventBus::eventPublished);

    const auto result = module.requestBlackout(true, QStringLiteral("remote"));

    QVERIFY(result.accepted);
    QVERIFY(module.blackout());
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);
    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.source, QStringLiteral("remote"));
    QCOMPARE(event.correlationId, command.id);
}

QTEST_APPLESS_MAIN(OutputModuleTest)
#include "OutputModuleTest.moc"
