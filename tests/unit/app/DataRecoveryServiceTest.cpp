#include "app/DataRecoveryService.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

static void writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(contents);
}

class DataRecoveryServiceTest final : public QObject {
    Q_OBJECT
private slots:
    void createsBackupAndAppliesPendingRestore();
    void detectsUncleanSessionAndCreatesRecoverySnapshot();
};

void DataRecoveryServiceTest::createsBackupAndAppliesPendingRestore()
{
    QTemporaryDir directory;
    const QByteArray current("SQLite format 3\0current", 23);
    const QByteArray restored("SQLite format 3\0restored", 24);
    writeFile(directory.filePath(QStringLiteral("presenter.db")), current);
    DataRecoveryService service(directory.path());
    const auto backup = service.createBackup();
    QVERIFY(QFile::exists(backup));
    writeFile(directory.filePath(QStringLiteral("incoming.db")), restored);
    QVERIFY(service.scheduleRestore(directory.filePath(QStringLiteral("incoming.db"))));
    QVERIFY(service.applyPendingRestore());
    QFile file(directory.filePath(QStringLiteral("presenter.db")));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), restored);
}

void DataRecoveryServiceTest::detectsUncleanSessionAndCreatesRecoverySnapshot()
{
    QTemporaryDir directory;
    writeFile(directory.filePath(QStringLiteral("presenter.db")), "database");
    writeFile(directory.filePath(QStringLiteral("session.lock")), "stale");
    DataRecoveryService service(directory.path());
    QVERIFY(service.beginSession());
    QVERIFY(service.recoveredFromCrash());
    QVERIFY(QFile::exists(service.recoverySnapshotPath()));
    QVERIFY(service.endSession());
    QVERIFY(!QFile::exists(directory.filePath(QStringLiteral("session.lock"))));
}

QTEST_MAIN(DataRecoveryServiceTest)
#include "DataRecoveryServiceTest.moc"
