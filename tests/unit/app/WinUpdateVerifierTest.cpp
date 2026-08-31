#include "app/WinUpdateVerifier.h"

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace {

QString digestOf(const QByteArray &payload)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

} // namespace

class WinUpdateVerifierTest final : public QObject {
    Q_OBJECT

private slots:
    void verifiesInstallerWithoutExhaustingTheHelperStack();
    void rejectsChangedInstaller();
};

void WinUpdateVerifierTest::verifiesInstallerWithoutExhaustingTheHelperStack()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray payload(4 * 1024 * 1024, 'H');
    const auto path = directory.filePath(QStringLiteral("HolyScreen-test-win64.exe"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(payload), payload.size());
    file.close();

    QVERIFY(holyscreen_update::verifiedInstaller(
        path.toStdWString(), digestOf(payload).toStdWString(),
        static_cast<std::uint64_t>(payload.size())));
}

void WinUpdateVerifierTest::rejectsChangedInstaller()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray payload("changed installer");
    const auto path = directory.filePath(QStringLiteral("HolyScreen-test-win64.exe"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(payload), payload.size());
    file.close();

    const std::wstring wrongDigest(64, L'0');
    QVERIFY(!holyscreen_update::verifiedInstaller(
        path.toStdWString(), wrongDigest,
        static_cast<std::uint64_t>(payload.size())));
}

QTEST_GUILESS_MAIN(WinUpdateVerifierTest)

#include "WinUpdateVerifierTest.moc"
