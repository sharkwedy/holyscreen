#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "bible/BibleImportService.h"
#include "bible/BibleRepository.h"

using namespace churchpresenter;

namespace {

bool writeFile(const QString &path, const QByteArray &contents)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
        && file.write(contents) == contents.size();
}

bool writeTranslation(const QString &root, const QString &code,
                      const QString &license, int bookId)
{
    const auto directory = QDir(root).filePath(QStringLiteral("data/canonical/%1").arg(code));
    const auto metadata = QStringLiteral(
        R"JSON({"code":"%1","name":"Tradução %1","license":"%2","publisher":"Editora"})JSON")
                              .arg(code, license).toUtf8();
    const auto book = QStringLiteral(
        R"JSON({"id":%1,"code":"BOOK","chapters":[{"number":1,"verses":[{"number":1,"text":"Texto %2"}]}]})JSON")
                          .arg(bookId).arg(code).toUtf8();
    return writeFile(QDir(directory).filePath(QStringLiteral("meta.json")), metadata)
        && writeFile(QDir(directory).filePath(QStringLiteral("BOOK.json")), book);
}

} // namespace

class BibleImportServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void requiresConfirmationBeforePersistingRestrictedTranslations();
    void importsIdempotentlyAndReportsProgress();
    void cancelsAndRollsBackCurrentTranslation();
    void importsCanonicalPublicGitRepositoryWhenNetworkTestsAreEnabled();
};

void BibleImportServiceTest::requiresConfirmationBeforePersistingRestrictedTranslations()
{
    QTemporaryDir source;
    QTemporaryDir data;
    QVERIFY(writeTranslation(source.path(), QStringLiteral("FREE"),
                             QStringLiteral("public-domain"), 1));
    QVERIFY(writeTranslation(source.path(), QStringLiteral("COPY"),
                             QStringLiteral("copyright"), 43));

    const auto databasePath = data.filePath(QStringLiteral("presenter.db"));
    const auto result = BibleImportService(databasePath).run(
        {.kind = BibleSourceKind::LocalFolder, .location = source.path()});

    QVERIFY(!result.success);
    QVERIFY(result.requiresLicenseConfirmation);
    QCOMPARE(result.restrictedTranslations.size(), 1);
    BibleRepository repository(databasePath);
    QVERIFY(repository.open());
    QVERIFY(repository.translations().isEmpty());
}

void BibleImportServiceTest::importsIdempotentlyAndReportsProgress()
{
    QTemporaryDir source;
    QTemporaryDir data;
    QVERIFY(writeTranslation(source.path(), QStringLiteral("FREE"),
                             QStringLiteral("public-domain"), 1));
    QVERIFY(writeTranslation(source.path(), QStringLiteral("COPY"),
                             QStringLiteral("copyright"), 43));
    QVector<BibleImportPhase> phases;
    const auto progress = [&phases](const BibleImportProgress &value) {
        phases.append(value.phase);
    };
    BibleImportService service(data.filePath(QStringLiteral("presenter.db")));
    const BibleSource bibleSource{.kind = BibleSourceKind::LocalFolder,
                                  .location = source.path(),
                                  .revision = QStringLiteral("local-v1")};
    const auto first = service.run(bibleSource, {.confirmRestrictedLicenses = true}, progress);
    QVERIFY2(first.success, qPrintable(first.errorSummary()));
    QCOMPARE(first.importedTranslations, 2);
    QCOMPARE(first.importedVerses, 2);
    QVERIFY(phases.contains(BibleImportPhase::Inspecting));
    QVERIFY(phases.contains(BibleImportPhase::Saving));
    QCOMPARE(phases.back(), BibleImportPhase::Completed);

    const auto second = service.run(bibleSource, {.confirmRestrictedLicenses = true});
    QVERIFY(second.success);
    BibleRepository repository(data.filePath(QStringLiteral("presenter.db")));
    QVERIFY(repository.open());
    QCOMPARE(repository.translations().size(), 2);
    QCOMPARE(repository.verses(QStringLiteral("canonical:FREE"),
                               {BibleBook::Genesis, 1, 1, 1}).size(), 1);
}

void BibleImportServiceTest::cancelsAndRollsBackCurrentTranslation()
{
    QTemporaryDir source;
    QTemporaryDir data;
    QVERIFY(writeTranslation(source.path(), QStringLiteral("AAA"),
                             QStringLiteral("public-domain"), 1));
    QVERIFY(writeTranslation(source.path(), QStringLiteral("BBB"),
                             QStringLiteral("public-domain"), 2));
    bool cancel = false;
    const auto progress = [&cancel](const BibleImportProgress &value) {
        if (value.phase == BibleImportPhase::Saving && value.current == 1) cancel = true;
    };
    const auto result = BibleImportService(data.filePath(QStringLiteral("presenter.db"))).run(
        {.kind = BibleSourceKind::LocalFolder, .location = source.path()}, {}, progress,
        [&cancel] { return cancel; });

    QVERIFY(result.cancelled);
    QVERIFY(!result.success);
    QCOMPARE(result.importedTranslations, 0);
    BibleRepository repository(data.filePath(QStringLiteral("presenter.db")));
    QVERIFY(repository.open());
    QCOMPARE(repository.translations().size(), 0);
}

void BibleImportServiceTest::importsCanonicalPublicGitRepositoryWhenNetworkTestsAreEnabled()
{
    if (qEnvironmentVariable("HOLYSCREEN_NETWORK_TESTS") != QStringLiteral("1")) {
        QSKIP("Defina HOLYSCREEN_NETWORK_TESTS=1 para executar o teste HTTPS real.");
    }
    QTemporaryDir data;
    const auto databasePath = data.filePath(QStringLiteral("presenter.db"));
    const auto result = BibleImportService(databasePath).run(
        {.kind = BibleSourceKind::GitHttps,
         .location = QStringLiteral("https://github.com/damarals/biblias.git")},
        {.confirmRestrictedLicenses = true});
    QVERIFY2(result.success, qPrintable(result.errorSummary()));
    QVERIFY(result.importedTranslations >= 10);
    QVERIFY(result.importedVerses > 100000);

    BibleRepository repository(databasePath);
    QVERIFY(repository.open());
    QCOMPARE(repository.translations().size(), result.importedTranslations);
    const auto source = repository.translationSource(QStringLiteral("canonical:TB"));
    QVERIFY(source.has_value());
    QCOMPARE(source->kind, BibleSourceKind::GitHttps);
    QCOMPARE(source->revision.size(), 40);
    QCOMPARE(source->license, QStringLiteral("public-domain"));
}

QTEST_GUILESS_MAIN(BibleImportServiceTest)
#include "BibleImportServiceTest.moc"
