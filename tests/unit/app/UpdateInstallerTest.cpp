#include "app/UpdateInstaller.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

using namespace churchpresenter;

namespace {
QString digestOf(const QByteArray &payload)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

QString writeInstaller(QTemporaryDir &directory, const QByteArray &payload)
{
    const auto path = directory.filePath(QStringLiteral("HolyScreen-1.2.1-win64.exe"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(payload) != payload.size()) return {};
    file.close();
    return path;
}
} // namespace

class UpdateInstallerTest final : public QObject {
    Q_OBJECT

private slots:
    void revalidatesAndLaunchesTheWindowsInstallerSilently();
    void refusesAPackageChangedAfterDownload();
    void refusesAutomaticInstallationOnUnsupportedSystems();
    void reportsWhenTheInstallerCannotBeStarted();
};

void UpdateInstallerTest::revalidatesAndLaunchesTheWindowsInstallerSilently()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray payload("verified installer payload");
    const auto path = writeInstaller(directory, payload);
    QVERIFY(!path.isEmpty());

    QString launchedProgram;
    QString launchedDigest;
    qint64 launchedSize = 0;
    UpdateInstaller installer(
        [&](const UpdateInstallRequest &request) {
            launchedProgram = request.installerPath;
            launchedDigest = request.expectedSha256;
            launchedSize = request.expectedSize;
            return true;
        },
        true);

    QString error;
    QVERIFY2(installer.install(path, digestOf(payload), payload.size(), &error),
             qPrintable(error));
    QCOMPARE(launchedProgram, path);
    QCOMPARE(launchedDigest, digestOf(payload));
    QCOMPARE(launchedSize, payload.size());
    QVERIFY(error.isEmpty());
}

void UpdateInstallerTest::refusesAPackageChangedAfterDownload()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray trustedPayload("trusted installer");
    const auto path = writeInstaller(directory, QByteArray("altered installer"));
    QVERIFY(!path.isEmpty());

    bool launched = false;
    UpdateInstaller installer(
        [&](const UpdateInstallRequest &) {
            launched = true;
            return true;
        },
        true);

    QString error;
    QVERIFY(!installer.install(path, digestOf(trustedPayload),
                               QFileInfo(path).size(), &error));
    QVERIFY(!launched);
    QVERIFY(error.contains(QStringLiteral("SHA-256")));
}

void UpdateInstallerTest::refusesAutomaticInstallationOnUnsupportedSystems()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray payload("installer");
    const auto path = writeInstaller(directory, payload);
    QVERIFY(!path.isEmpty());

    UpdateInstaller installer({}, false);
    QString error;
    QVERIFY(!installer.install(path, digestOf(payload), payload.size(), &error));
    QVERIFY(!error.isEmpty());
}

void UpdateInstallerTest::reportsWhenTheInstallerCannotBeStarted()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray payload("installer");
    const auto path = writeInstaller(directory, payload);
    QVERIFY(!path.isEmpty());

    UpdateInstaller installer(
        [](const UpdateInstallRequest &) { return false; }, true);
    QString error;
    QVERIFY(!installer.install(path, digestOf(payload), payload.size(), &error));
    QVERIFY(error.contains(QStringLiteral("Windows")));
}

QTEST_GUILESS_MAIN(UpdateInstallerTest)

#include "UpdateInstallerTest.moc"
