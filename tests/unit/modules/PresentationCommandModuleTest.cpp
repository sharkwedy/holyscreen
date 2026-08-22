#include "modules/PresentationCommandModule.h"

#include <QSignalSpy>
#include <QTest>

using namespace churchpresenter;

class PresentationCommandModuleTest final : public QObject {
    Q_OBJECT

private slots:
    void dispatchesNavigationAndPublishesState();
    void rejectsInvalidSlideIndexes();
};

void PresentationCommandModuleTest::dispatchesNavigationAndPublishesState()
{
    CommandBus commands;
    EventBus events;
    int index = 0;
    PresentationCommandModule module(commands, events, {
        .show = [&index](int next) { index = next; return true; },
        .next = [&index] { ++index; return true; },
        .previous = [&index] { --index; return true; },
        .first = [&index] { index = 0; return true; },
        .last = [&index] { index = 9; return true; },
        .stop = [] { return true; },
        .stateSnapshot = [&index] { return QVariantMap{{QStringLiteral("slideIndex"), index}}; },
    });
    QSignalSpy eventSpy(&events, &EventBus::eventPublished);

    QVERIFY(module.requestShow(4).accepted);
    QCOMPARE(index, 4);
    QVERIFY(module.requestNext().accepted);
    QCOMPARE(index, 5);
    QCOMPARE(eventSpy.count(), 2);
    const auto event = qvariant_cast<DomainEvent>(eventSpy.last().front());
    QCOMPARE(event.type, QStringLiteral("presentation.state.changed"));
    QCOMPARE(event.payload.value(QStringLiteral("slideIndex")).toInt(), 5);
}

void PresentationCommandModuleTest::rejectsInvalidSlideIndexes()
{
    CommandBus commands;
    EventBus events;
    PresentationCommandModule module(commands, events, {});
    const auto result = module.requestShow(-1);
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("invalid_payload"));
}

QTEST_APPLESS_MAIN(PresentationCommandModuleTest)
#include "PresentationCommandModuleTest.moc"
