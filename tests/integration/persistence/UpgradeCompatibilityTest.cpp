#include <QtTest/QTest>

#include "persistence/ApplicationDatabase.h"

#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

using namespace churchpresenter;

namespace {

QString connectionName(const QString &purpose)
{
    return QStringLiteral("upgrade-%1-%2")
        .arg(purpose, QUuid::createUuid().toString(QUuid::WithoutBraces));
}

bool executeAll(QSqlDatabase &database, const QStringList &statements, QString *error)
{
    QSqlQuery query(database);
    for (const auto &statement : statements) {
        if (!query.exec(statement)) {
            if (error) *error = query.lastError().text();
            return false;
        }
    }
    return true;
}

bool prepareHistoricalDatabase(const QString &path, int schemaVersion, QString *error)
{
    const auto baseline = ApplicationDatabase::migrate(path);
    if (!baseline.success) {
        if (error) *error = baseline.error;
        return false;
    }

    const auto connection = connectionName(QStringLiteral("prepare"));
    bool prepared = false;
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        if (!database.open()) {
            if (error) *error = database.lastError().text();
        } else {
            QStringList statements;
            if (schemaVersion < 5) {
                statements << QStringLiteral("DROP TABLE automation_runs")
                           << QStringLiteral("DROP TABLE automation_actions")
                           << QStringLiteral("DROP TABLE automation_conditions")
                           << QStringLiteral("DROP TABLE automations")
                           << QStringLiteral("DROP TABLE authorized_executables");
            }
            if (schemaVersion < 4) {
                statements << QStringLiteral("DROP TABLE integration_call_history")
                           << QStringLiteral("DROP TABLE integration_definitions");
            }
            if (schemaVersion < 3) {
                statements << QStringLiteral("DROP TABLE output_broadcast_profiles");
            }
            statements << QStringLiteral("DELETE FROM schema_version WHERE version>%1")
                              .arg(schemaVersion)
                       << QStringLiteral(
                              "INSERT INTO settings(key,value) VALUES('upgrade.marker','preserve-me')")
                       << QStringLiteral(
                              "INSERT INTO events(id,title,scheduled_at) "
                              "VALUES('upgrade-event','Culto preservado','2026-08-24')")
                       << QStringLiteral(
                              "INSERT INTO bible_translations(id,name,abbreviation,language) "
                              "VALUES('upgrade-bible','Bíblia de teste','BT','pt-BR')")
                       << QStringLiteral(
                              "INSERT INTO bible_translation_sources("
                              "translation_id,source_kind,source_location,license,imported_at) "
                              "VALUES('upgrade-bible','folder','fixture','teste','2026-08-24')");
            if (schemaVersion >= 4) {
                statements << QStringLiteral(
                    "INSERT INTO integration_definitions(id,name,type) "
                    "VALUES('upgrade-integration','OBS','obs')");
            }
            if (schemaVersion >= 5) {
                statements << QStringLiteral(
                    "INSERT INTO automations(id,name,trigger_type) "
                    "VALUES('upgrade-automation','Início','event.started')");
            }
            prepared = executeAll(database, statements, error);
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return prepared;
}

int scalarCount(QSqlDatabase &database, const QString &statement)
{
    QSqlQuery query(database);
    if (!query.exec(statement) || !query.next()) return -1;
    return query.value(0).toInt();
}

} // namespace

class UpgradeCompatibilityTest final : public QObject {
    Q_OBJECT

private slots:
    void upgradesPublishedDatabaseWithoutDataLoss_data();
    void upgradesPublishedDatabaseWithoutDataLoss();
};

void UpgradeCompatibilityTest::upgradesPublishedDatabaseWithoutDataLoss_data()
{
    QTest::addColumn<QString>("release");
    QTest::addColumn<int>("schemaVersion");

    QTest::newRow("0.11.0") << QStringLiteral("0.11.0") << 2;
    QTest::newRow("0.12.0") << QStringLiteral("0.12.0") << 4;
    QTest::newRow("0.13.0") << QStringLiteral("0.13.0") << 5;
    QTest::newRow("0.14.0") << QStringLiteral("0.14.0") << 5;
}

void UpgradeCompatibilityTest::upgradesPublishedDatabaseWithoutDataLoss()
{
    QFETCH(QString, release);
    QFETCH(int, schemaVersion);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("presenter-%1.db").arg(release));
    QString preparationError;
    QVERIFY2(prepareHistoricalDatabase(path, schemaVersion, &preparationError),
             qPrintable(preparationError));

    const auto result = ApplicationDatabase::migrate(path);

    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.previousVersion, schemaVersion);
    QCOMPARE(result.currentVersion, 5);
    QCOMPARE(!result.backupPath.isEmpty(), schemaVersion < 5);
    if (schemaVersion < 5) QVERIFY(QFileInfo::exists(result.backupPath));

    const auto connection = connectionName(QStringLiteral("verify"));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY2(database.open(), qPrintable(database.lastError().text()));
        QCOMPARE(scalarCount(database,
                             QStringLiteral("SELECT COUNT(*) FROM settings "
                                            "WHERE key='upgrade.marker' AND value='preserve-me'")), 1);
        QCOMPARE(scalarCount(database,
                             QStringLiteral("SELECT COUNT(*) FROM events "
                                            "WHERE id='upgrade-event' AND title='Culto preservado'")), 1);
        QCOMPARE(scalarCount(database,
                             QStringLiteral("SELECT COUNT(*) FROM bible_translations "
                                            "WHERE id='upgrade-bible'")), 1);
        if (schemaVersion >= 4) {
            QCOMPARE(scalarCount(database,
                                 QStringLiteral("SELECT COUNT(*) FROM integration_definitions "
                                                "WHERE id='upgrade-integration'")), 1);
        }
        if (schemaVersion >= 5) {
            QCOMPARE(scalarCount(database,
                                 QStringLiteral("SELECT COUNT(*) FROM automations "
                                                "WHERE id='upgrade-automation'")), 1);
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

QTEST_GUILESS_MAIN(UpgradeCompatibilityTest)
#include "UpgradeCompatibilityTest.moc"
