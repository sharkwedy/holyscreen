#include "automation/ConditionEvaluator.h"

#include <QTest>

using namespace churchpresenter;

namespace {

ConditionEvaluator::Context context()
{
    return {
        .event = {{QStringLiteral("title"), QStringLiteral("Grande é o Senhor")},
                  {QStringLiteral("slideIndex"), 3},
                  {QStringLiteral("media"), QVariantMap{{QStringLiteral("type"),
                                                         QStringLiteral("video")}}}},
        .state = {{QStringLiteral("eventTitle"), QStringLiteral("Culto de domingo")},
                  {QStringLiteral("activeOutputs"), 2},
                  {QStringLiteral("blackout"), false}},
        .localTime = QTime(19, 30),
    };
}

Condition condition(const QString &field, const QString &operation, const QVariant &expected = {})
{
    return {.field = field, .operation = operation, .expected = expected};
}

} // namespace

class ConditionEvaluatorTest final : public QObject {
    Q_OBJECT

private slots:
    void resolvesEventStateAndNestedFields();
    void comparesTextWithEqualityAndContains();
    void comparesNumbersAndRanges();
    void matchesTimeWindowsIncludingMidnight();
    void checksEmptiness();
    void combinesConditionsWithAllAndAny();
    void unknownOperationsNeverMatch();
};

void ConditionEvaluatorTest::resolvesEventStateAndNestedFields()
{
    const auto ctx = context();
    QCOMPARE(ConditionEvaluator::valueOf(QStringLiteral("event.title"), ctx).toString(),
             QStringLiteral("Grande é o Senhor"));
    QCOMPARE(ConditionEvaluator::valueOf(QStringLiteral("state.activeOutputs"), ctx).toInt(), 2);
    QCOMPARE(ConditionEvaluator::valueOf(QStringLiteral("event.media.type"), ctx).toString(),
             QStringLiteral("video"));
    // Sem prefixo, o evento tem prioridade e o estado é a segunda opção.
    QCOMPARE(ConditionEvaluator::valueOf(QStringLiteral("title"), ctx).toString(),
             QStringLiteral("Grande é o Senhor"));
    QCOMPARE(ConditionEvaluator::valueOf(QStringLiteral("eventTitle"), ctx).toString(),
             QStringLiteral("Culto de domingo"));
    QVERIFY(!ConditionEvaluator::valueOf(QStringLiteral("event.inexistente"), ctx).isValid());
}

void ConditionEvaluatorTest::comparesTextWithEqualityAndContains()
{
    const auto ctx = context();
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QStringLiteral("equals"),
                  QStringLiteral("grande é o senhor")), ctx));
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QStringLiteral("notEquals"),
                  QStringLiteral("Grande é o Senhor")), ctx));
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QStringLiteral("contains"),
                  QStringLiteral("senhor")), ctx));
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QStringLiteral("notContains"),
                  QStringLiteral("oferta")), ctx));
}

void ConditionEvaluatorTest::comparesNumbersAndRanges()
{
    const auto ctx = context();
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.slideIndex"), QStringLiteral("greaterThan"), 2), ctx));
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("event.slideIndex"), QStringLiteral("greaterThan"), 3), ctx));
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.slideIndex"), QStringLiteral("lessThan"), 4), ctx));
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.slideIndex"), QStringLiteral("between"),
                  QVariantList{1, 5}), ctx));
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("event.slideIndex"), QStringLiteral("between"),
                  QVariantList{10, 20}), ctx));
    // Texto não numérico nunca satisfaz comparação numérica.
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QStringLiteral("greaterThan"), 1), ctx));
}

void ConditionEvaluatorTest::matchesTimeWindowsIncludingMidnight()
{
    auto ctx = context();
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("time"), QStringLiteral("timeBetween"),
                  QVariantList{QStringLiteral("19:00"), QStringLiteral("21:00")}), ctx));
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("time"), QStringLiteral("timeBetween"),
                  QVariantList{QStringLiteral("08:00"), QStringLiteral("12:00")}), ctx));

    // Janela que cruza a meia-noite.
    ctx.localTime = QTime(23, 45);
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("time"), QStringLiteral("timeBetween"),
                  QVariantList{QStringLiteral("22:00"), QStringLiteral("02:00")}), ctx));
    ctx.localTime = QTime(1, 15);
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("time"), QStringLiteral("timeBetween"),
                  QVariantList{QStringLiteral("22:00"), QStringLiteral("02:00")}), ctx));
    ctx.localTime = QTime(12, 0);
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("time"), QStringLiteral("timeBetween"),
                  QVariantList{QStringLiteral("22:00"), QStringLiteral("02:00")}), ctx));
}

void ConditionEvaluatorTest::checksEmptiness()
{
    const auto ctx = context();
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.subtitle"), QStringLiteral("isEmpty")), ctx));
    QVERIFY(ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QStringLiteral("isNotEmpty")), ctx));
}

void ConditionEvaluatorTest::combinesConditionsWithAllAndAny()
{
    const auto ctx = context();
    const QList<Condition> conditions{
        condition(QStringLiteral("event.title"), QStringLiteral("contains"),
                  QStringLiteral("Senhor")),
        condition(QStringLiteral("state.activeOutputs"), QStringLiteral("greaterThan"), 5),
    };

    QVERIFY(!ConditionEvaluator::matches(conditions, ConditionGroup::All, ctx));
    QVERIFY(ConditionEvaluator::matches(conditions, ConditionGroup::Any, ctx));
    // Sem condições, a automação sempre passa.
    QVERIFY(ConditionEvaluator::matches({}, ConditionGroup::All, ctx));
}

void ConditionEvaluatorTest::unknownOperationsNeverMatch()
{
    const auto ctx = context();
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QStringLiteral("regex"),
                  QStringLiteral(".*")), ctx));
    QVERIFY(!ConditionEvaluator::matches(
        condition(QStringLiteral("event.title"), QString{}, QStringLiteral("x")), ctx));
}

QTEST_APPLESS_MAIN(ConditionEvaluatorTest)
#include "ConditionEvaluatorTest.moc"
