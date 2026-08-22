#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

#include "bible/BibleSourceStager.h"
#include "bible/CanonicalBibleImporter.h"

#include <algorithm>

using namespace churchpresenter;

namespace {

bool writeFile(const QString &path, const QByteArray &contents)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return false;
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
        && file.write(contents) == contents.size();
}

bool run(const QString &program, const QStringList &arguments, const QString &workingDirectory)
{
    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.start(program, arguments);
    return process.waitForFinished(30000) && process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0;
}

} // namespace

class BibleSourceStagerTest final : public QObject {
    Q_OBJECT

private slots:
    void rejectsUnsafeZipEntryNames();
    void downloadsAndExtractsLocalZipFixture();
    void clonesRepositoryWithLibgit2AndCapturesRevision();
    void rejectsNonHttpsProductionUrls();
    void clonesCanonicalPublicRepositoryWhenNetworkTestsAreEnabled();
    void downloadsCanonicalPublicZipWhenNetworkTestsAreEnabled();
};

void BibleSourceStagerTest::rejectsUnsafeZipEntryNames()
{
    QVERIFY(ZipArchiveExtractor::isSafeEntryName(QStringLiteral("data/canonical/TB/meta.json")));
    QVERIFY(!ZipArchiveExtractor::isSafeEntryName(QStringLiteral("../escape.json")));
    QVERIFY(!ZipArchiveExtractor::isSafeEntryName(QStringLiteral("data/../../escape.json")));
    QVERIFY(!ZipArchiveExtractor::isSafeEntryName(QStringLiteral("/absolute.json")));
    QVERIFY(!ZipArchiveExtractor::isSafeEntryName(QStringLiteral("C:\\escape.json")));
    QVERIFY(!ZipArchiveExtractor::isSafeEntryName(QStringLiteral("data\\..\\escape.json")));
}

void BibleSourceStagerTest::downloadsAndExtractsLocalZipFixture()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto input = directory.filePath(QStringLiteral("input"));
    QVERIFY(writeFile(input + QStringLiteral("/data/canonical/TB/meta.json"),
                      R"JSON({"code":"TB","name":"Tradução Brasileira","license":"public-domain"})JSON"));
    const auto archive = directory.filePath(QStringLiteral("bibles.zip"));
    QVERIFY(run(QStringLiteral("cmake"),
                {QStringLiteral("-E"), QStringLiteral("tar"), QStringLiteral("cf"), archive,
                 QStringLiteral("--format=zip"), QStringLiteral("data")}, input));

    QTemporaryDir staging;
    QVERIFY(staging.isValid());
    ZipBibleSourceStager stager(/*allowLocalFiles=*/true);
    const auto result = stager.stage(
        {.kind = BibleSourceKind::ZipUrl, .location = QUrl::fromLocalFile(archive).toString()},
        staging.path(), {}, [] { return false; });

    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.source.kind, BibleSourceKind::ZipUrl);
    QCOMPARE(result.source.location, QUrl::fromLocalFile(archive).toString());
    QVERIFY(QFileInfo(QDir(result.localPath).filePath(
        QStringLiteral("data/canonical/TB/meta.json"))).isFile());
}

void BibleSourceStagerTest::clonesRepositoryWithLibgit2AndCapturesRevision()
{
    QTemporaryDir repository;
    QVERIFY(repository.isValid());
    QVERIFY(run(QStringLiteral("git"), {QStringLiteral("init"), QStringLiteral("--quiet")}, repository.path()));
    QVERIFY(writeFile(repository.filePath(QStringLiteral("data/canonical/TB/meta.json")),
                      R"JSON({"code":"TB","name":"Tradução Brasileira","license":"public-domain"})JSON"));
    QVERIFY(run(QStringLiteral("git"), {QStringLiteral("add"), QStringLiteral(".")}, repository.path()));
    QVERIFY(run(QStringLiteral("git"),
                {QStringLiteral("-c"), QStringLiteral("user.name=HolyScreen Test"),
                 QStringLiteral("-c"), QStringLiteral("user.email=test@holyscreen.local"),
                 QStringLiteral("commit"), QStringLiteral("--quiet"), QStringLiteral("-m"),
                 QStringLiteral("fixture")}, repository.path()));

    QTemporaryDir staging;
    GitBibleSourceStager stager(/*allowLocalRepositories=*/true);
    const auto result = stager.stage(
        {.kind = BibleSourceKind::GitHttps,
         .location = QUrl::fromLocalFile(repository.path()).toString()},
        staging.filePath(QStringLiteral("clone")), {}, [] { return false; });

    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.source.revision.size(), 40);
    QVERIFY(QFileInfo(QDir(result.localPath).filePath(
        QStringLiteral("data/canonical/TB/meta.json"))).isFile());
}

void BibleSourceStagerTest::rejectsNonHttpsProductionUrls()
{
    QTemporaryDir staging;
    const auto gitResult = GitBibleSourceStager{}.stage(
        {.kind = BibleSourceKind::GitHttps, .location = QStringLiteral("http://example.test/repo.git")},
        staging.filePath(QStringLiteral("git")), {}, [] { return false; });
    QVERIFY(!gitResult.success);

    const auto zipResult = ZipBibleSourceStager{}.stage(
        {.kind = BibleSourceKind::ZipUrl, .location = QStringLiteral("file:///tmp/archive.zip")},
        staging.filePath(QStringLiteral("zip")), {}, [] { return false; });
    QVERIFY(!zipResult.success);
}

void BibleSourceStagerTest::clonesCanonicalPublicRepositoryWhenNetworkTestsAreEnabled()
{
    if (qEnvironmentVariable("HOLYSCREEN_NETWORK_TESTS") != QStringLiteral("1")) {
        QSKIP("Defina HOLYSCREEN_NETWORK_TESTS=1 para executar o teste HTTPS real.");
    }
    QTemporaryDir staging;
    const auto result = GitBibleSourceStager{}.stage(
        {.kind = BibleSourceKind::GitHttps,
         .location = QStringLiteral("https://github.com/damarals/biblias.git")},
        staging.filePath(QStringLiteral("biblias")), {}, [] { return false; });
    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.source.revision.size(), 40);
    QVERIFY(QFileInfo(QDir(result.localPath).filePath(
        QStringLiteral("data/canonical/TB/meta.json"))).isFile());
    const auto plan = CanonicalBibleImporter{}.inspect(result.source, result.localPath, {}, {});
    QVERIFY2(plan.isValid(), qPrintable(plan.errorSummary()));
    QVERIFY(plan.translations.size() >= 10);
    auto tb = std::find_if(plan.translations.cbegin(), plan.translations.cend(),
                           [](const PlannedBibleTranslation &translation) {
        return translation.translation.abbreviation == QStringLiteral("TB");
    });
    QVERIFY(tb != plan.translations.cend());
    QVERIFY(!tb->verses.isEmpty());
    QVERIFY(!tb->requiresLicenseConfirmation);
}

void BibleSourceStagerTest::downloadsCanonicalPublicZipWhenNetworkTestsAreEnabled()
{
    if (qEnvironmentVariable("HOLYSCREEN_NETWORK_TESTS") != QStringLiteral("1")) {
        QSKIP("Defina HOLYSCREEN_NETWORK_TESTS=1 para executar o teste HTTPS real.");
    }
    QTemporaryDir staging;
    const auto result = ZipBibleSourceStager{}.stage(
        {.kind = BibleSourceKind::ZipUrl,
         .location = QStringLiteral(
             "https://github.com/damarals/biblias/archive/refs/heads/main.zip")},
        staging.filePath(QStringLiteral("biblias-zip")), {}, [] { return false; });
    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.source.revision.size(), 64);
    const auto plan = CanonicalBibleImporter{}.inspect(result.source, result.localPath, {}, {});
    QVERIFY2(plan.isValid(), qPrintable(plan.errorSummary()));
    QVERIFY(plan.translations.size() >= 10);
}

QTEST_GUILESS_MAIN(BibleSourceStagerTest)
#include "BibleSourceStagerTest.moc"
