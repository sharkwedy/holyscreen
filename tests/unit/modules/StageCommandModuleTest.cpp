#include "modules/StageCommandModule.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class StageCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void updatesStageThroughCommandAndSupportsUndo();
};

void StageCommandModuleTest::updatesStageThroughCommandAndSupportsUndo()
{
    qRegisterMetaType<DomainEvent>();
    CommandBus commands;
    EventBus events;
    UndoManager undo;
    QString message = QStringLiteral("Inicial");
    StageCommandModule module(commands, events, {
        .message = [&message] { return message; },
        .setMessage = [&message](const QString &next) { message = next; return true; },
    }, &undo);
    QSignalSpy eventSpy(&events, &EventBus::eventPublished);

    const auto result = module.requestMessage(QStringLiteral("  Próxima música  "));

    QVERIFY(result.accepted);
    QCOMPARE(message, QStringLiteral("Próxima música"));
    QCOMPARE(eventSpy.count(), 1);
    QCOMPARE(qvariant_cast<DomainEvent>(eventSpy.first().front()).type,
             QStringLiteral("stage.state.changed"));
    QVERIFY(undo.undo().success);
    QCOMPARE(message, QStringLiteral("Inicial"));
    QVERIFY(undo.redo().success);
    QCOMPARE(message, QStringLiteral("Próxima música"));
}

QTEST_APPLESS_MAIN(StageCommandModuleTest)
#include "StageCommandModuleTest.moc"
