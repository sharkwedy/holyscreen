#include "modules/EventCommandModule.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class EventCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void routesEventSelectionAndExecution();
    void rejectsEmptyIdentifiers();
};

void EventCommandModuleTest::routesEventSelectionAndExecution()
{
    qRegisterMetaType<DomainEvent>();
    CommandBus commands;
    EventBus events;
    QString selected;
    QString executed;
    EventCommandModule module(commands, events, {
        .select = [&selected](const QString &id) { selected = id; return true; },
        .executeItem = [&executed](const QString &id) { executed = id; return true; },
        .stateSnapshot = [&selected, &executed] {
            return QVariantMap{{QStringLiteral("eventId"), selected},
                               {QStringLiteral("itemId"), executed}};
        },
    });
    QSignalSpy eventSpy(&events, &EventBus::eventPublished);

    QVERIFY(module.requestSelect(QStringLiteral("event-1")).accepted);
    QVERIFY(module.requestExecuteItem(QStringLiteral("item-2"), QStringLiteral("remote")).accepted);
    QCOMPARE(selected, QStringLiteral("event-1"));
    QCOMPARE(executed, QStringLiteral("item-2"));
    QCOMPARE(eventSpy.count(), 2);
    QCOMPARE(qvariant_cast<DomainEvent>(eventSpy.last().front()).type,
             QStringLiteral("event.state.changed"));
}

void EventCommandModuleTest::rejectsEmptyIdentifiers()
{
    CommandBus commands;
    EventBus events;
    EventCommandModule module(commands, events, {});
    QCOMPARE(module.requestSelect(QStringLiteral(" ")).errorCode,
             QStringLiteral("invalid_payload"));
    QCOMPARE(module.requestExecuteItem(QString{}).errorCode,
             QStringLiteral("invalid_payload"));
}

QTEST_APPLESS_MAIN(EventCommandModuleTest)
#include "EventCommandModuleTest.moc"
