#include <QtTest/QTest>

#include "app/ConfigurationProfileService.h"

using namespace churchpresenter;

class ConfigurationProfileServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void roundTripsACompleteSecretFreeProfile();
    void rejectsSensitiveAndUnknownFields();
    void rejectsInvalidRangesEnumsAndShortcuts();
    void rejectsMalformedUnsupportedAndOversizedDocuments();
};

void ConfigurationProfileServiceTest::roundTripsACompleteSecretFreeProfile()
{
    const QVariantMap profile{
        {QStringLiteral("locale"), QStringLiteral("pt-BR")},
        {QStringLiteral("demoMode"), true},
        {QStringLiteral("presentation"), QVariantMap{
             {QStringLiteral("wallpaperColor"), QStringLiteral("#102030")},
             {QStringLiteral("clockVisible"), true},
             {QStringLiteral("clockFontSize"), 48},
         }},
        {QStringLiteral("media"), QVariantMap{
             {QStringLiteral("volume"), 0.75},
             {QStringLiteral("repeatMode"), QStringLiteral("all")},
             {QStringLiteral("imageIntervalMs"), 5000},
         }},
        {QStringLiteral("remote"), QVariantMap{
             {QStringLiteral("interface"), QStringLiteral("127.0.0.1")},
             {QStringLiteral("port"), 43120},
         }},
        {QStringLiteral("library"), QVariantMap{
             {QStringLiteral("mediaFolders"), QStringList{QStringLiteral("D:/media")}},
             {QStringLiteral("favorites"), QStringList{}},
         }},
        {QStringLiteral("outputs"), QStringList{QStringLiteral("display-a|audience")}},
        {QStringLiteral("onboarding"), QVariantMap{
             {QStringLiteral("completed"), false},
             {QStringLiteral("skippedSteps"), QStringList{QStringLiteral("broadcast")}},
         }},
        {QStringLiteral("shortcuts"), QVariantMap{
             {QStringLiteral("blackout"), QStringLiteral("Ctrl+B")},
         }},
    };

    QStringList errors;
    const auto document = ConfigurationProfileService::serialize(profile, &errors);
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join(u'\n')));
    QVERIFY(!document.isEmpty());
    QVERIFY(!document.contains("password"));
    QVERIFY(!document.contains("secret"));

    const auto parsed = ConfigurationProfileService::parse(document);
    QVERIFY2(parsed.accepted, qPrintable(parsed.errors.join(u'\n')));
    QCOMPARE(parsed.profile.value(QStringLiteral("locale")).toString(), QStringLiteral("pt-BR"));
    QCOMPARE(parsed.profile.value(QStringLiteral("media")).toMap()
                 .value(QStringLiteral("volume")).toDouble(), 0.75);
    QCOMPARE(parsed.profile.value(QStringLiteral("library")).toMap()
                 .value(QStringLiteral("mediaFolders")).toStringList(),
             QStringList{QStringLiteral("D:/media")});
}

void ConfigurationProfileServiceTest::rejectsSensitiveAndUnknownFields()
{
    auto result = ConfigurationProfileService::validate({
        {QStringLiteral("remote"), QVariantMap{
             {QStringLiteral("port"), 43120},
             {QStringLiteral("password"), QStringLiteral("must-not-leak")},
         }},
    });
    QVERIFY(!result.accepted);
    QVERIFY(result.profile.isEmpty());
    QVERIFY(result.errors.join(u'\n').contains(QStringLiteral("sensível")));

    result = ConfigurationProfileService::validate({
        {QStringLiteral("futureField"), true},
    });
    QVERIFY(!result.accepted);
    QVERIFY(result.errors.join(u'\n').contains(QStringLiteral("desconhecido")));
}

void ConfigurationProfileServiceTest::rejectsInvalidRangesEnumsAndShortcuts()
{
    const auto result = ConfigurationProfileService::validate({
        {QStringLiteral("presentation"), QVariantMap{
             {QStringLiteral("wallpaperFit"), QStringLiteral("tile")},
             {QStringLiteral("clockFontSize"), 500},
         }},
        {QStringLiteral("media"), QVariantMap{
             {QStringLiteral("repeatMode"), QStringLiteral("forever")},
             {QStringLiteral("imageIntervalMs"), 100},
         }},
        {QStringLiteral("shortcuts"), QVariantMap{
             {QStringLiteral("blackout"), QStringLiteral("Ctrl+B")},
             {QStringLiteral("quickBible"), QStringLiteral("ctrl+b")},
             {QStringLiteral("unknownAction"), QStringLiteral("Ctrl+U")},
         }},
    });

    QVERIFY(!result.accepted);
    const auto errors = result.errors.join(u'\n');
    QVERIFY(errors.contains(QStringLiteral("wallpaperFit")));
    QVERIFY(errors.contains(QStringLiteral("clockFontSize")));
    QVERIFY(errors.contains(QStringLiteral("repeatMode")));
    QVERIFY(errors.contains(QStringLiteral("imageIntervalMs")));
    QVERIFY(errors.contains(QStringLiteral("duplicado")));
    QVERIFY(errors.contains(QStringLiteral("unknownAction")));
}

void ConfigurationProfileServiceTest::rejectsMalformedUnsupportedAndOversizedDocuments()
{
    QVERIFY(!ConfigurationProfileService::parse(QByteArrayLiteral("not-json")).accepted);
    QVERIFY(!ConfigurationProfileService::parse(QByteArrayLiteral(
        R"({"documentType":"holyscreen.configuration","schemaVersion":2,"profile":{}})"))
                 .accepted);
    const QByteArray oversized(ConfigurationProfileService::MaximumDocumentSize + 1, 'x');
    const auto result = ConfigurationProfileService::parse(oversized);
    QVERIFY(!result.accepted);
    QVERIFY(result.errors.join(u'\n').contains(QStringLiteral("1 MiB")));
}

QTEST_GUILESS_MAIN(ConfigurationProfileServiceTest)
#include "ConfigurationProfileServiceTest.moc"
