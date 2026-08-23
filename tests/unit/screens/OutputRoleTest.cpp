#include <QtTest/QTest>

#include "screens/OutputRole.h"

using namespace churchpresenter;

class OutputRoleTest final : public QObject {
    Q_OBJECT

private slots:
    void serializesEveryRoleWithARoundTrip();
    void rejectsUnknownNamesInsteadOfFallingBackSilently();
    void normalizesCaseAndSurroundingSpaces();
    void listsEveryRoleNameOnce();
    void appliesTheRequestedFallbackForLegacyValues();
};

void OutputRoleTest::serializesEveryRoleWithARoundTrip()
{
    const QVector<std::pair<OutputRole, QString>> expectations{
        {OutputRole::Audience, QStringLiteral("audience")},
        {OutputRole::Stage, QStringLiteral("stage")},
        {OutputRole::Broadcast, QStringLiteral("broadcast")},
        {OutputRole::Confidence, QStringLiteral("confidence")},
        {OutputRole::Custom, QStringLiteral("custom")},
    };

    for (const auto &[role, name] : expectations) {
        QCOMPARE(outputRoleName(role), name);
        QCOMPARE(outputRoleFromName(name), std::optional<OutputRole>{role});
    }
}

void OutputRoleTest::rejectsUnknownNamesInsteadOfFallingBackSilently()
{
    QCOMPARE(outputRoleFromName(QStringLiteral("monitor")), std::nullopt);
    QCOMPARE(outputRoleFromName(QString{}), std::nullopt);
    QVERIFY(!isOutputRoleName(QStringLiteral("monitor")));
    QVERIFY(isOutputRoleName(QStringLiteral("broadcast")));
}

void OutputRoleTest::normalizesCaseAndSurroundingSpaces()
{
    QCOMPARE(outputRoleFromName(QStringLiteral("  Broadcast ")),
             std::optional<OutputRole>{OutputRole::Broadcast});
    QCOMPARE(outputRoleFromName(QStringLiteral("CUSTOM")),
             std::optional<OutputRole>{OutputRole::Custom});
}

void OutputRoleTest::listsEveryRoleNameOnce()
{
    const auto names = outputRoleNames();
    QCOMPARE(names.size(), 5);
    QCOMPARE(names, QStringList({QStringLiteral("audience"), QStringLiteral("stage"),
                                 QStringLiteral("broadcast"), QStringLiteral("confidence"),
                                 QStringLiteral("custom")}));
    for (const auto &name : names) {
        const auto role = outputRoleFromName(name);
        QVERIFY(role.has_value());
        QCOMPARE(outputRoleName(*role), name);
    }
}

void OutputRoleTest::appliesTheRequestedFallbackForLegacyValues()
{
    QCOMPARE(outputRoleFromNameOr(QStringLiteral("stage"), OutputRole::Audience), OutputRole::Stage);
    QCOMPARE(outputRoleFromNameOr(QStringLiteral("legacy"), OutputRole::Audience),
             OutputRole::Audience);
}

QTEST_APPLESS_MAIN(OutputRoleTest)
#include "OutputRoleTest.moc"
