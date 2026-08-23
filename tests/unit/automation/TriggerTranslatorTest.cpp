#include "automation/TriggerTranslator.h"

#include <QTest>

using namespace churchpresenter;

namespace {

DomainEvent domainEvent(const QString &type, const QVariantMap &payload,
                  const QString &correlationId = QStringLiteral("cmd-1"))
{
    return DomainEvent{.type = type,
                       .payload = payload,
                       .occurredAt = QDateTime::currentDateTimeUtc(),
                       .correlationId = correlationId};
}

} // namespace

class TriggerTranslatorTest final : public QObject {
    Q_OBJECT

private slots:
    void mapsPresentationActions();
    void mapsMediaActions();
    void mapsEventActions();
    void ignoresEventsWithoutATrigger();
    void keepsPayloadAndCorrelation();
};

void TriggerTranslatorTest::mapsPresentationActions()
{
    const auto first = TriggerTranslator::translate(
        domainEvent(QStringLiteral("presentation.state.changed"),
              {{QStringLiteral("action"), QStringLiteral("slide.show")},
               {QStringLiteral("slideIndex"), 0}}));
    QVERIFY(first.has_value());
    QCOMPARE(first->triggerType, QString::fromLatin1(AutomationTrigger::PresentationStarted));

    const auto middle = TriggerTranslator::translate(
        domainEvent(QStringLiteral("presentation.state.changed"),
              {{QStringLiteral("action"), QStringLiteral("slide.show")},
               {QStringLiteral("slideIndex"), 4}}));
    QCOMPARE(middle->triggerType, QString::fromLatin1(AutomationTrigger::SlideChanged));

    const auto next = TriggerTranslator::translate(
        domainEvent(QStringLiteral("presentation.state.changed"),
              {{QStringLiteral("action"), QStringLiteral("slide.next")}}));
    QCOMPARE(next->triggerType, QString::fromLatin1(AutomationTrigger::SlideChanged));

    const auto stopped = TriggerTranslator::translate(
        domainEvent(QStringLiteral("presentation.state.changed"),
              {{QStringLiteral("action"), QStringLiteral("stop")}}));
    QCOMPARE(stopped->triggerType, QString::fromLatin1(AutomationTrigger::PresentationStopped));
}

void TriggerTranslatorTest::mapsMediaActions()
{
    QCOMPARE(TriggerTranslator::translate(
                 domainEvent(QStringLiteral("media.state.changed"),
                       {{QStringLiteral("action"), QStringLiteral("media.play")}}))->triggerType,
             QString::fromLatin1(AutomationTrigger::MediaStarted));
    QCOMPARE(TriggerTranslator::translate(
                 domainEvent(QStringLiteral("media.state.changed"),
                       {{QStringLiteral("action"), QStringLiteral("media.stop")}}))->triggerType,
             QString::fromLatin1(AutomationTrigger::MediaFinished));

    const auto resumed = TriggerTranslator::translate(
        domainEvent(QStringLiteral("media.state.changed"),
              {{QStringLiteral("action"), QStringLiteral("media.pause.toggle")},
               {QStringLiteral("state"), QStringLiteral("playing")}}));
    QCOMPARE(resumed->triggerType, QString::fromLatin1(AutomationTrigger::MediaStarted));

    const auto paused = TriggerTranslator::translate(
        domainEvent(QStringLiteral("media.state.changed"),
              {{QStringLiteral("action"), QStringLiteral("media.pause.toggle")},
               {QStringLiteral("state"), QStringLiteral("paused")}}));
    QCOMPARE(paused->triggerType, QString::fromLatin1(AutomationTrigger::MediaPaused));
}

void TriggerTranslatorTest::mapsEventActions()
{
    QCOMPARE(TriggerTranslator::translate(
                 domainEvent(QStringLiteral("event.state.changed"),
                       {{QStringLiteral("action"), QStringLiteral("select")}}))->triggerType,
             QString::fromLatin1(AutomationTrigger::EventSelected));
    QCOMPARE(TriggerTranslator::translate(
                 domainEvent(QStringLiteral("event.state.changed"),
                       {{QStringLiteral("action"), QStringLiteral("item.execute")}}))->triggerType,
             QString::fromLatin1(AutomationTrigger::EventItemExecuted));
}

void TriggerTranslatorTest::ignoresEventsWithoutATrigger()
{
    QCOMPARE(TriggerTranslator::translate(
                 domainEvent(QStringLiteral("system.undo-state.changed"), {})),
             std::nullopt);
    QCOMPARE(TriggerTranslator::translate(
                 domainEvent(QStringLiteral("media.state.changed"),
                       {{QStringLiteral("action"), QStringLiteral("media.seek")}})),
             std::nullopt);
    // Um resultado de integração nunca vira gatilho, para não realimentar o laço.
    QCOMPARE(TriggerTranslator::translate(
                 domainEvent(QStringLiteral("integration.call.finished"), {})),
             std::nullopt);
}

void TriggerTranslatorTest::keepsPayloadAndCorrelation()
{
    const auto match = TriggerTranslator::translate(
        domainEvent(QStringLiteral("presentation.state.changed"),
              {{QStringLiteral("action"), QStringLiteral("slide.next")},
               {QStringLiteral("title"), QStringLiteral("Grande é o Senhor")}},
              QStringLiteral("origem")));
    QVERIFY(match.has_value());
    QCOMPARE(match->correlationId, QStringLiteral("origem"));
    QCOMPARE(match->payload.value(QStringLiteral("title")).toString(),
             QStringLiteral("Grande é o Senhor"));
}

QTEST_APPLESS_MAIN(TriggerTranslatorTest)
#include "TriggerTranslatorTest.moc"
