#include "app/UpdateChecker.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

using namespace churchpresenter;

class UpdateCheckerTest final : public QObject
{
    Q_OBJECT

private slots:
    void selectsVerifiedPlatformAsset();
    void ignoresUnverifiedAssets();
    void rejectsUntrustedReleaseUrl();
    void comparesReleaseCandidates();
    void selectsNewestPublishedReleaseFromList();
    void rejectsOversizedManifest();
};

namespace {
QByteArray releasePayload(const QString &tag, const QJsonArray &assets = {})
{
    return QJsonDocument(QJsonObject{
        {QStringLiteral("tag_name"), tag},
        {QStringLiteral("html_url"), QStringLiteral("https://github.com/sharkwedy/holyscreen/releases/tag/") + tag},
        {QStringLiteral("draft"), false},
        {QStringLiteral("prerelease"), tag.contains(QStringLiteral("-rc."))},
        {QStringLiteral("assets"), assets}
    }).toJson(QJsonDocument::Compact);
}

QJsonObject asset(const QString &name, const QString &digest, qint64 size = 42)
{
    return {
        {QStringLiteral("name"), name},
        {QStringLiteral("digest"), digest},
        {QStringLiteral("size"), size},
        {QStringLiteral("browser_download_url"),
         QStringLiteral("https://github.com/sharkwedy/holyscreen/releases/download/v1.0.0/") + name}
    };
}
}

void UpdateCheckerTest::selectsVerifiedPlatformAsset()
{
    const auto digest = QStringLiteral("sha256:") + QString(64, QLatin1Char('a'));
    const auto result = UpdateChecker::parseGitHubRelease(
        releasePayload(QStringLiteral("v1.0.0"), {
            asset(QStringLiteral("HolyScreen-1.0.0.zip"), digest),
            asset(QStringLiteral("HolyScreen-1.0.0.exe"), digest, 84)
        }),
        QStringLiteral("0.14.0"), QStringLiteral("windows"));

    QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
    QVERIFY(result.available);
    QCOMPARE(result.latestVersion, QStringLiteral("1.0.0"));
    QCOMPARE(result.assetName, QStringLiteral("HolyScreen-1.0.0.exe"));
    QCOMPARE(result.assetSize, 84);
    QCOMPARE(result.sha256, QString(64, QLatin1Char('a')));
}

void UpdateCheckerTest::ignoresUnverifiedAssets()
{
    const auto result = UpdateChecker::parseGitHubRelease(
        releasePayload(QStringLiteral("v1.0.0"), {
            asset(QStringLiteral("HolyScreen-1.0.0.exe"), QStringLiteral("sha256:invalid"))
        }),
        QStringLiteral("0.14.0"), QStringLiteral("windows"));

    QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
    QVERIFY(result.available);
    QVERIFY(result.downloadUrl.isEmpty());
    QVERIFY(result.sha256.isEmpty());
    QVERIFY(result.releaseUrl.isValid());
}

void UpdateCheckerTest::rejectsUntrustedReleaseUrl()
{
    auto document = QJsonDocument::fromJson(releasePayload(QStringLiteral("v1.0.0")));
    auto object = document.object();
    object.insert(QStringLiteral("html_url"), QStringLiteral("http://example.com/release"));
    const auto result = UpdateChecker::parseGitHubRelease(
        QJsonDocument(object).toJson(QJsonDocument::Compact),
        QStringLiteral("0.14.0"), QStringLiteral("linux"));

    QVERIFY(!result.error.isEmpty());
    QVERIFY(!result.available);
}

void UpdateCheckerTest::comparesReleaseCandidates()
{
    QVERIFY(UpdateChecker::parseGitHubRelease(
        releasePayload(QStringLiteral("v1.0.0-rc.2")),
        QStringLiteral("1.0.0-rc.1"), QStringLiteral("linux")).available);
    QVERIFY(UpdateChecker::parseGitHubRelease(
        releasePayload(QStringLiteral("v1.0.0")),
        QStringLiteral("1.0.0-rc.9"), QStringLiteral("linux")).available);
    QVERIFY(!UpdateChecker::parseGitHubRelease(
        releasePayload(QStringLiteral("v1.0.0-rc.1")),
        QStringLiteral("1.0.0"), QStringLiteral("linux")).available);
}

void UpdateCheckerTest::selectsNewestPublishedReleaseFromList()
{
    const auto draft = QJsonDocument::fromJson(releasePayload(QStringLiteral("v9.0.0"))).object();
    auto unpublished = draft;
    unpublished.insert(QStringLiteral("draft"), true);
    const auto older = QJsonDocument::fromJson(releasePayload(QStringLiteral("v0.14.0"))).object();
    const auto newest = QJsonDocument::fromJson(releasePayload(QStringLiteral("v1.0.0-rc.1"))).object();

    const auto result = UpdateChecker::parseGitHubRelease(
        QJsonDocument(QJsonArray{unpublished, older, newest}).toJson(QJsonDocument::Compact),
        QStringLiteral("0.13.0"), QStringLiteral("windows"));

    QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
    QCOMPARE(result.latestVersion, QStringLiteral("1.0.0-rc.1"));
    QVERIFY(result.available);
}

void UpdateCheckerTest::rejectsOversizedManifest()
{
    const auto result = UpdateChecker::parseGitHubRelease(
        QByteArray(256 * 1024 + 1, 'x'),
        QStringLiteral("0.14.0"), QStringLiteral("linux"));
    QVERIFY(!result.error.isEmpty());
}

QTEST_GUILESS_MAIN(UpdateCheckerTest)
#include "UpdateCheckerTest.moc"
