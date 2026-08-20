#include <QtTest/QTest>

#include "persistence/DatabaseMigrator.h"

#include <QCoreApplication>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

using namespace churchpresenter;

namespace {

bool createLegacyDatabase(const QString &path)
{
    const auto connection = QStringLiteral("legacy-%1")
                                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool created = false;
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        created = database.open();
        if (created) {
            QSqlQuery query(database);
            created = query.exec(QStringLiteral("CREATE TABLE legacy_value(value TEXT NOT NULL)"))
                && query.exec(QStringLiteral("INSERT INTO legacy_value VALUES ('preservar')"));
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
    return created;
}

} // namespace

class DatabaseMigratorTest final : public QObject {
    Q_OBJECT

private slots:
    void createsBackupAndAppliesNumberedMigrationTransactionally();
    void newDatabaseDoesNotCreateMeaninglessBackup();
    void rollsBackEveryPendingMigrationWhenOneFails();
};

void DatabaseMigratorTest::createsBackupAndAppliesNumberedMigrationTransactionally()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto databasePath = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(createLegacyDatabase(databasePath));

    DatabaseMigrator migrator(databasePath);
    QVERIFY(migrator.addMigration(1, QStringLiteral("create operational events"),
                                  [](QSqlDatabase &database, QString *error) {
        QSqlQuery query(database);
        if (query.exec(QStringLiteral(
                "CREATE TABLE operational_events(id TEXT PRIMARY KEY, type TEXT NOT NULL)"))) {
            return true;
        }
        if (error) *error = query.lastError().text();
        return false;
    }));

    const auto result = migrator.migrate();

    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.previousVersion, 0);
    QCOMPARE(result.currentVersion, 1);
    QVERIFY(!result.backupPath.isEmpty());
    QVERIFY(QFile::exists(result.backupPath));
}

void DatabaseMigratorTest::newDatabaseDoesNotCreateMeaninglessBackup()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto databasePath = directory.filePath(QStringLiteral("new.db"));
    QVERIFY(!QFile::exists(databasePath));

    DatabaseMigrator migrator(databasePath);
    QVERIFY(migrator.addMigration(1, QStringLiteral("initial schema"),
                                  [](QSqlDatabase &, QString *) { return true; }));

    const auto result = migrator.migrate();

    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.currentVersion, 1);
    QVERIFY(result.backupPath.isEmpty());
}

void DatabaseMigratorTest::rollsBackEveryPendingMigrationWhenOneFails()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto databasePath = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(createLegacyDatabase(databasePath));

    DatabaseMigrator migrator(databasePath);
    QVERIFY(migrator.addMigration(1, QStringLiteral("temporary table"),
                                  [](QSqlDatabase &database, QString *error) {
        QSqlQuery query(database);
        const auto success = query.exec(QStringLiteral("CREATE TABLE must_rollback(id TEXT)"));
        if (!success && error) *error = query.lastError().text();
        return success;
    }));
    QVERIFY(migrator.addMigration(2, QStringLiteral("forced failure"),
                                  [](QSqlDatabase &, QString *error) {
        if (error) *error = QStringLiteral("falha controlada");
        return false;
    }));

    const auto result = migrator.migrate();

    QVERIFY(!result.success);
    QCOMPARE(result.previousVersion, 0);
    QCOMPARE(result.currentVersion, 0);
    QVERIFY(QFile::exists(result.backupPath));

    const auto connection = QStringLiteral("rollback-check-%1")
                                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(databasePath);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral(
            "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='must_rollback'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        QVERIFY(query.exec(QStringLiteral("SELECT value FROM legacy_value")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("preservar"));
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    DatabaseMigratorTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "DatabaseMigratorTest.moc"
