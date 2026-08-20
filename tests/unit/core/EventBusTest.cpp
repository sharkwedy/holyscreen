#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "core/EventBus.h"

using namespace churchpresenter;

class EventBusTest final : public QObject {
    Q_OBJECT

private slots:
    void publishesDomainEventToSubscribers();
};

void EventBusTest::publishesDomainEventToSubscribers()
{
    qRegisterMetaType<DomainEvent>();
    EventBus bus;
    QSignalSpy spy(&bus, &EventBus::eventPublished);

    const DomainEvent event{
        .type = QStringLiteral("presentation.blackout.changed"),
        .payload = {{QStringLiteral("enabled"), true}},
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .correlationId = QStringLiteral("command-1"),
    };

    QVERIFY(bus.publish(event));
    QCOMPARE(spy.count(), 1);
    const auto received = qvariant_cast<DomainEvent>(spy.takeFirst().at(0));
    QCOMPARE(received.type, event.type);
    QCOMPARE(received.payload, event.payload);
    QCOMPARE(received.correlationId, event.correlationId);
}

QTEST_APPLESS_MAIN(EventBusTest)
#include "EventBusTest.moc"
