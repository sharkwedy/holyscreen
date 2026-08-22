#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "bible/CanonicalBibleImporter.h"

using namespace churchpresenter;

namespace {

bool writeFile(const QString &path, const QByteArray &contents)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
        && file.write(contents) == contents.size();
}

} // namespace

class CanonicalBibleImporterTest final : public QObject {
    Q_OBJECT

private slots:
    void discoversRepositoryRootAndParsesTranslations();
    void acceptsCanonicalRootAndSingleTranslationDirectory();
    void rejectsMalformedBooksWithoutReturningPartialTranslation();
    void keepsOtherTranslationsAndSummarizesInvalidOnes();
};

void CanonicalBibleImporterTest::discoversRepositoryRootAndParsesTranslations()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto canonical = directory.filePath(QStringLiteral("data/canonical"));
    QVERIFY(writeFile(canonical + QStringLiteral("/TB/meta.json"), R"JSON({
        "code":"TB","name":"Tradução Brasileira","year":2010,
        "publisher":"SBB","license":"public-domain","scope":"full",
        "source":"openlp_sqlite","language":"pt-BR"
    })JSON"));
    QVERIFY(writeFile(canonical + QStringLiteral("/TB/JHN.json"), R"JSON({
        "id":43,"code":"JHN","name":"João","abbrev":"Jo",
        "chapters":[{"number":1,"verses":[
            {"number":1,"text":"No princípio era o Verbo."},
            {"number":2,"text":"Ele estava no princípio com Deus."}
        ]}]
    })JSON"));
    QVERIFY(writeFile(canonical + QStringLiteral("/ACF/meta.json"), R"JSON({
        "code":"ACF","name":"Almeida Corrigida e Fiel","year":1994,
        "publisher":"SBTB","license":"copyright","scope":"full",
        "source":"openlp_sqlite"
    })JSON"));
    QVERIFY(writeFile(canonical + QStringLiteral("/ACF/GEN.json"), R"JSON({
        "id":1,"code":"GEN","name":"Gênesis","abbrev":"Gn",
        "chapters":[{"number":1,"verses":[{"number":1,"text":"No princípio criou Deus."}]}]
    })JSON"));

    BibleSource source{
        .kind = BibleSourceKind::LocalFolder,
        .location = directory.path(),
        .revision = QStringLiteral("fixture-revision"),
    };
    const auto plan = CanonicalBibleImporter{}.inspect(source);

    QVERIFY2(plan.isValid(), qPrintable(plan.errorSummary()));
    QCOMPARE(plan.translations.size(), 2);
    QCOMPARE(plan.translations.at(0).translation.abbreviation, QStringLiteral("ACF"));
    QCOMPARE(plan.translations.at(0).translation.id, QStringLiteral("canonical:ACF"));
    QCOMPARE(plan.translations.at(0).translation.language, QStringLiteral("pt-BR"));
    QCOMPARE(plan.translations.at(0).source.license, QStringLiteral("copyright"));
    QCOMPARE(plan.translations.at(0).source.publisher, QStringLiteral("SBTB"));
    QCOMPARE(plan.translations.at(0).source.revision, QStringLiteral("fixture-revision"));
    QVERIFY(plan.translations.at(0).requiresLicenseConfirmation);
    QCOMPARE(plan.translations.at(1).verses.size(), 2);
    QCOMPARE(plan.translations.at(1).verses.at(0).book, BibleBook::John);
    QVERIFY(!plan.translations.at(1).requiresLicenseConfirmation);
}

void CanonicalBibleImporterTest::acceptsCanonicalRootAndSingleTranslationDirectory()
{
    QTemporaryDir directory;
    const auto canonical = directory.filePath(QStringLiteral("data/canonical"));
    QVERIFY(writeFile(canonical + QStringLiteral("/TB/meta.json"),
                      R"JSON({"code":"TB","name":"Tradução Brasileira","license":"public-domain"})JSON"));
    QVERIFY(writeFile(canonical + QStringLiteral("/TB/PSA.json"),
                      R"JSON({"id":19,"code":"PSA","chapters":[{"number":23,"verses":[{"number":1,"text":"O Senhor é meu pastor."}]}]})JSON"));

    const auto fromCanonical = CanonicalBibleImporter{}.inspect(
        {.kind = BibleSourceKind::LocalFolder, .location = canonical});
    QVERIFY2(fromCanonical.isValid(), qPrintable(fromCanonical.errorSummary()));
    QCOMPARE(fromCanonical.translations.size(), 1);

    const auto fromTranslation = CanonicalBibleImporter{}.inspect(
        {.kind = BibleSourceKind::LocalFolder, .location = canonical + QStringLiteral("/TB")});
    QVERIFY2(fromTranslation.isValid(), qPrintable(fromTranslation.errorSummary()));
    QCOMPARE(fromTranslation.translations.size(), 1);
    QCOMPARE(fromTranslation.translations.front().verses.front().book, BibleBook::Psalms);

    const auto wrappedRoot = directory.filePath(QStringLiteral("download/biblias-main"));
    QVERIFY(QDir().mkpath(wrappedRoot));
    QVERIFY(QFile::copy(canonical + QStringLiteral("/TB/meta.json"),
                        wrappedRoot + QStringLiteral("/data/canonical/TB/meta.json"))
            || (QDir().mkpath(wrappedRoot + QStringLiteral("/data/canonical/TB"))
                && QFile::copy(canonical + QStringLiteral("/TB/meta.json"),
                               wrappedRoot + QStringLiteral("/data/canonical/TB/meta.json"))));
    QVERIFY(QFile::copy(canonical + QStringLiteral("/TB/PSA.json"),
                        wrappedRoot + QStringLiteral("/data/canonical/TB/PSA.json")));
    const auto fromArchiveWrapper = CanonicalBibleImporter{}.inspect(
        {.kind = BibleSourceKind::ZipUrl, .location = directory.filePath(QStringLiteral("download"))});
    QVERIFY2(fromArchiveWrapper.isValid(), qPrintable(fromArchiveWrapper.errorSummary()));
    QCOMPARE(fromArchiveWrapper.translations.size(), 1);
}

void CanonicalBibleImporterTest::rejectsMalformedBooksWithoutReturningPartialTranslation()
{
    QTemporaryDir directory;
    const auto translation = directory.filePath(QStringLiteral("data/canonical/BAD"));
    QVERIFY(writeFile(translation + QStringLiteral("/meta.json"),
                      R"JSON({"code":"BAD","name":"Inválida","license":"public-domain"})JSON"));
    QVERIFY(writeFile(translation + QStringLiteral("/GEN.json"),
                      R"JSON({"id":67,"chapters":[{"number":1,"verses":[{"number":1,"text":"texto"}]}]})JSON"));

    const auto plan = CanonicalBibleImporter{}.inspect(
        {.kind = BibleSourceKind::LocalFolder, .location = directory.path()});
    QVERIFY(!plan.isValid());
    QVERIFY(plan.translations.isEmpty());
    QVERIFY(plan.errorSummary().contains(QStringLiteral("GEN.json")));
}

void CanonicalBibleImporterTest::keepsOtherTranslationsAndSummarizesInvalidOnes()
{
    QTemporaryDir directory;
    const auto canonical = directory.filePath(QStringLiteral("data/canonical"));
    QVERIFY(writeFile(canonical + QStringLiteral("/GOOD/meta.json"),
                      R"JSON({"code":"GOOD","name":"Válida","license":"public-domain"})JSON"));
    QVERIFY(writeFile(canonical + QStringLiteral("/GOOD/GEN.json"),
                      R"JSON({"id":1,"chapters":[{"number":1,"verses":[{"number":1,"text":"texto"}]}]})JSON"));
    QVERIFY(writeFile(canonical + QStringLiteral("/BAD/meta.json"),
                      R"JSON({"code":"BAD","name":"Inválida","license":"public-domain"})JSON"));
    QVERIFY(writeFile(canonical + QStringLiteral("/BAD/GEN.json"),
                      R"JSON({"id":1,"chapters":[{"number":1,"verses":[{"number":1,"text":"a"},{"number":1,"text":"b"}]}]})JSON"));

    const auto plan = CanonicalBibleImporter{}.inspect(
        {.kind = BibleSourceKind::LocalFolder, .location = directory.path()});
    QVERIFY(plan.isValid());
    QCOMPARE(plan.translations.size(), 1);
    QCOMPARE(plan.translations.front().translation.abbreviation, QStringLiteral("GOOD"));
    QCOMPARE(plan.errors.size(), 1);
    QVERIFY(plan.errorSummary().contains(QStringLiteral("BAD"))
            || plan.errorSummary().contains(QStringLiteral("GEN.json")));
}

QTEST_APPLESS_MAIN(CanonicalBibleImporterTest)
#include "CanonicalBibleImporterTest.moc"
