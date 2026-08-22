#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/ApplicationController.h"
#include "library/PresentationRepository.h"
#include "core/CommandCatalog.h"

#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using namespace churchpresenter;

class ApplicationCommandBridgeTest final : public QObject {
    Q_OBJECT

private slots:
    void operatorBlackoutUsesCommandAndEventBuses();
    void registersEveryCatalogCommand();
    void operatorOverlayUsesCommandAndEventBuses();
    void operatorMediaUsesCommandAndEventBuses();
    void operatorPresentationNavigationUsesCommandAndEventBuses();
    void undoAndRedoUseCommandBus();
    void autosaveDebouncesPresentationEdits();
    void presentationEditsSupportUndoAndRedo();
    void canonicalBibleImportRequiresLicenseThenCompletesAsynchronously();
};

void ApplicationCommandBridgeTest::operatorBlackoutUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QCOMPARE(controller.diagnostics().value(QStringLiteral("schemaVersion")).toInt(), 2);
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

void ApplicationCommandBridgeTest::registersEveryCatalogCommand()
{
    ApplicationController controller;
    for (const auto &descriptor : CommandCatalog::descriptors()) {
        QVERIFY2(controller.commandBus().hasHandler(descriptor.type),
                 qPrintable(QStringLiteral("Handler ausente: %1").arg(descriptor.type)));
    }
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

void ApplicationCommandBridgeTest::operatorPresentationNavigationUsesCommandAndEventBuses()
{
    qRegisterMetaType<Command>();
    qRegisterMetaType<CommandResult>();
    qRegisterMetaType<DomainEvent>();

    ApplicationController controller;
    QVERIFY(!controller.createTextPresentation(QStringLiteral("Avisos")).isEmpty());
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);
    QSignalSpy eventSpy(&controller.eventBus(), &EventBus::eventPublished);

    controller.showTextSlide(0);

    QVERIFY(controller.textVisible());
    QCOMPARE(commandSpy.count(), 1);
    QCOMPARE(eventSpy.count(), 1);
    const auto command = qvariant_cast<Command>(commandSpy.first().at(0));
    const auto event = qvariant_cast<DomainEvent>(eventSpy.first().at(0));
    QCOMPARE(command.type, QStringLiteral("presentation.slide.show"));
    QCOMPARE(command.payload.value(QStringLiteral("index")).toInt(), 0);
    QCOMPARE(event.type, QStringLiteral("presentation.state.changed"));
    QCOMPARE(event.payload.value(QStringLiteral("slideIndex")).toInt(), 0);
    QCOMPARE(event.correlationId, command.id);
}

void ApplicationCommandBridgeTest::undoAndRedoUseCommandBus()
{
    qRegisterMetaType<Command>();
    ApplicationController controller;
    QSignalSpy commandSpy(&controller.commandBus(), &CommandBus::commandDispatched);

    controller.setBlackout(true);
    QVERIFY(controller.blackout());
    QVERIFY(controller.canUndo());

    controller.undo();
    QVERIFY(!controller.blackout());
    QVERIFY(controller.canRedo());
    QCOMPARE(qvariant_cast<Command>(commandSpy.last().at(0)).type,
             QStringLiteral("system.undo"));

    controller.redo();
    QVERIFY(controller.blackout());
    QVERIFY(controller.canUndo());
    QCOMPARE(qvariant_cast<Command>(commandSpy.last().at(0)).type,
             QStringLiteral("system.redo"));
}

void ApplicationCommandBridgeTest::autosaveDebouncesPresentationEdits()
{
    ApplicationController controller;
    const auto presentationId = controller.createTextPresentation(QStringLiteral("Avisos"));
    QVERIFY(!presentationId.isEmpty());
    const auto slideId = controller.currentSlideId();
    QVERIFY(!slideId.isEmpty());

    controller.updateTextSlide(slideId, QStringLiteral("1"), QStringLiteral("Primeiro texto"));
    controller.updateTextSlide(slideId, QStringLiteral("1"), QStringLiteral("Texto final"));

    QVERIFY(controller.autosavePending());
    QTRY_VERIFY_WITH_TIMEOUT(!controller.autosavePending(), 2000);

    PresentationRepository persisted(
        qEnvironmentVariable("HOLYSCREEN_DATA_DIR") + QStringLiteral("/presenter.db"));
    QVERIFY(persisted.open());
    const auto reloaded = persisted.presentation(presentationId);
    QCOMPARE(reloaded.slides.size(), 1);
    QCOMPARE(reloaded.slides.front().text, QStringLiteral("Texto final"));
}

void ApplicationCommandBridgeTest::presentationEditsSupportUndoAndRedo()
{
    ApplicationController controller;
    QVERIFY(!controller.createTextPresentation(QStringLiteral("Avisos")).isEmpty());
    const auto slideId = controller.currentSlideId();
    controller.updateTextSlide(slideId, QStringLiteral("1"), QStringLiteral("Texto alterado"));
    QCOMPARE(controller.currentSlideText(), QStringLiteral("Texto alterado"));

    QVERIFY(controller.canUndo());
    controller.undo();
    QCOMPARE(controller.currentSlideText(), QString{});
    QVERIFY(controller.canRedo());
    controller.redo();
    QCOMPARE(controller.currentSlideText(), QStringLiteral("Texto alterado"));
}

void ApplicationCommandBridgeTest::canonicalBibleImportRequiresLicenseThenCompletesAsynchronously()
{
    QTemporaryDir source;
    const auto translation = source.filePath(QStringLiteral("data/canonical/DEMO"));
    QVERIFY(QDir().mkpath(translation));
    QFile metadata(QDir(translation).filePath(QStringLiteral("meta.json")));
    QVERIFY(metadata.open(QIODevice::WriteOnly));
    QVERIFY(metadata.write(
        R"JSON({"code":"DEMO","name":"Demonstração","license":"copyright"})JSON") > 0);
    metadata.close();
    QFile book(QDir(translation).filePath(QStringLiteral("JHN.json")));
    QVERIFY(book.open(QIODevice::WriteOnly));
    QVERIFY(book.write(
        R"JSON({"id":43,"chapters":[{"number":3,"verses":[{"number":16,"text":"Texto de demonstração"}]}]})JSON") > 0);
    book.close();

    ApplicationController controller;
    QSignalSpy stateSpy(&controller, &ApplicationController::bibleImportStateChanged);
    QVERIFY(controller.importBibleFolder(QUrl::fromLocalFile(source.path())));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.bibleImportRunning(), 3000);
    QVERIFY(controller.bibleImportRequiresLicenseConfirmation());
    QVERIFY(controller.bibleTranslations().isEmpty());

    QVERIFY(controller.confirmBibleImportLicenses());
    QTRY_VERIFY_WITH_TIMEOUT(!controller.bibleImportRunning(), 3000);
    QVERIFY(!controller.bibleImportRequiresLicenseConfirmation());
    QCOMPARE(controller.bibleTranslations().size(), 1);
    QCOMPARE(controller.bibleImportProgress(), 100);
    QVERIFY(controller.bibleImportMessage().contains(QStringLiteral("1 tradução")));
    QVERIFY(stateSpy.count() > 2);
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
