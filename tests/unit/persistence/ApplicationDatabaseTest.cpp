#include <QtTest/QTest>

#include "persistence/ApplicationDatabase.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

using namespace churchpresenter;

class ApplicationDatabaseTest final : public QObject {
    Q_OBJECT

private slots:
    void createsVersionedBaselineSchema();
    void migratesVersionOneWithBackup();
    void addsBroadcastProfilesToAnExistingInstallation();
    void keepsTheDatabaseUntouchedWhenTheBroadcastMigrationFails();
};

void ApplicationDatabaseTest::createsVersionedBaselineSchema()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("presenter.db"));

    const auto result = ApplicationDatabase::migrate(path);

    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.previousVersion, 0);
    QCOMPARE(result.currentVersion, 3);

    const auto connection = QStringLiteral("schema-check-%1")
                                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table'")));
        QSet<QString> tables;
        while (query.next()) tables.insert(query.value(0).toString());
        const QSet<QString> expected{
            QStringLiteral("schema_version"),
            QStringLiteral("settings"),
            QStringLiteral("media_items"),
            QStringLiteral("presentations"),
            QStringLiteral("slides"),
            QStringLiteral("presentation_sequence"),
            QStringLiteral("themes"),
            QStringLiteral("events"),
            QStringLiteral("playlist_items"),
            QStringLiteral("history"),
            QStringLiteral("bible_translations"),
            QStringLiteral("bible_verses"),
            QStringLiteral("bible_translation_sources"),
            QStringLiteral("output_broadcast_profiles"),
        };
        for (const auto &table : expected) {
            QVERIFY2(tables.contains(table), qPrintable(QStringLiteral("Tabela ausente: %1").arg(table)));
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

void ApplicationDatabaseTest::migratesVersionOneWithBackup()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);

    const auto connection = QStringLiteral("schema-v1-%1")
                                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("DROP TABLE bible_translation_sources")));
        QVERIFY(query.exec(QStringLiteral("DROP TABLE output_broadcast_profiles")));
        QVERIFY(query.exec(QStringLiteral("DELETE FROM schema_version WHERE version>1")));
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);

    const auto result = ApplicationDatabase::migrate(path);
    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.previousVersion, 1);
    QCOMPARE(result.currentVersion, 3);
    QVERIFY(!result.backupPath.isEmpty());
    QVERIFY(QFileInfo::exists(result.backupPath));
}

void ApplicationDatabaseTest::addsBroadcastProfilesToAnExistingInstallation()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);

    const auto connection = QStringLiteral("schema-v2-%1")
                                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        // Volta o banco ao estado publicado na 0.11 e grava dados do usuário.
        QVERIFY(query.exec(QStringLiteral("DROP TABLE output_broadcast_profiles")));
        QVERIFY(query.exec(QStringLiteral("DELETE FROM schema_version WHERE version=3")));
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO events(id,title,scheduled_at) VALUES('e1','Culto','2026-08-23')")));
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);

    const auto result = ApplicationDatabase::migrate(path);
    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.previousVersion, 2);
    QCOMPARE(result.currentVersion, 3);

    const auto check = QStringLiteral("schema-v3-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), check);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("SELECT title FROM events WHERE id='e1'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("Culto"));
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO output_broadcast_profiles(screen_fingerprint) VALUES('hdmi-2')")));
        QVERIFY(query.exec(QStringLiteral(
            "SELECT background_mode,chroma_color,safe_area_left,aspect_preset,show_lower_third "
            "FROM output_broadcast_profiles WHERE screen_fingerprint='hdmi-2'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QStringLiteral("chroma"));
        QCOMPARE(query.value(1).toString(), QStringLiteral("#00b140"));
        QCOMPARE(query.value(2).toDouble(), 5.0);
        QCOMPARE(query.value(3).toString(), QStringLiteral("16:9"));
        QCOMPARE(query.value(4).toInt(), 1);
        database.close();
    }
    QSqlDatabase::removeDatabase(check);

    // Reaplicar a migração em um banco já atualizado não muda nada.
    const auto again = ApplicationDatabase::migrate(path);
    QVERIFY(again.success);
    QCOMPARE(again.previousVersion, 3);
    QCOMPARE(again.currentVersion, 3);
}

void ApplicationDatabaseTest::keepsTheDatabaseUntouchedWhenTheBroadcastMigrationFails()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);

    const auto connection = QStringLiteral("schema-conflict-%1")
                                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("DROP TABLE output_broadcast_profiles")));
        QVERIFY(query.exec(QStringLiteral("DELETE FROM schema_version WHERE version=3")));
        // Um objeto conflitante faz a segunda instrução da migração falhar.
        QVERIFY(query.exec(QStringLiteral(
            "CREATE VIEW output_broadcast_profiles_mode_idx AS SELECT 1")));
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);

    const auto result = ApplicationDatabase::migrate(path);
    QVERIFY(!result.success);
    QVERIFY(!result.error.isEmpty());
    QCOMPARE(result.currentVersion, 2);
    QVERIFY(QFileInfo::exists(result.backupPath));

    const auto check = QStringLiteral("schema-rollback-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), check);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral(
            "SELECT COUNT(*) FROM sqlite_master WHERE name='output_broadcast_profiles'")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 0);
        QVERIFY(query.exec(QStringLiteral("SELECT MAX(version) FROM schema_version")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 2);
        database.close();
    }
    QSqlDatabase::removeDatabase(check);
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    ApplicationDatabaseTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "ApplicationDatabaseTest.moc"
