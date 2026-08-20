#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/ApplicationController.h"

#include <QGuiApplication>
#include <QTemporaryDir>

using namespace churchpresenter;

class ApplicationCommandBridgeTest final : public QObject {
    Q_OBJECT

private slots:
    void operatorBlackoutUsesCommandAndEventBuses();
    void operatorOverlayUsesCommandAndEventBuses();
    void operatorMediaUsesCommandAndEventBuses();
};

void ApplicationCommandBridgeTest::operatorBlackoutUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QCOMPARE(controller.diagnostics().value(QStringLiteral("schemaVersion")).toInt(), 1);
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.setBlackout(true);

    QVERIFY(controller.blackout());
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);

    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto result = qvariant_cast<CommandResult>(commandSpy.first().at(1));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("presentation.blackout.set"));
    QCOMPARE(command.source, QStringLiteral("operator"));
    QVERIFY(result.accepted);
    QCOMPARE(event.type, QStringLiteral("presentation.blackout.changed"));
    QCOMPARE(event.correlationId, command.id);
}

void ApplicationCommandBridgeTest::operatorOverlayUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.setAudienceMessage(QStringLiteral("Aviso importante"));

    QCOMPARE(controller.audienceMessage(), QStringLiteral("Aviso importante"));
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);
    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("overlay.audience-message.set"));
    QCOMPARE(event.type, QStringLiteral("overlay.state.changed"));
    QCOMPARE(event.correlationId, command.id);
}

void ApplicationCommandBridgeTest::operatorMediaUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.stopMedia();

    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);
    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("media.stop"));
    QCOMPARE(event.type, QStringLiteral("media.state.changed"));
    QCOMPARE(event.correlationId, command.id);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    QGuiApplication app(argc, argv);
    QTemporaryDir dataDirectory;
    if (!dataDirectory.isValid()) return 2;
    qputenv("HOLYSCREEN_DATA_DIR", dataDirectory.path().toUtf8());
    ApplicationCommandBridgeTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "ApplicationCommandBridgeTest.moc"
