#include <QtTest/QTest>

#include "persistence/ApplicationDatabase.h"

#include <QCoreApplication>
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
};

void ApplicationDatabaseTest::createsVersionedBaselineSchema()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("presenter.db"));

    const auto result = ApplicationDatabase::migrate(path);

    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.previousVersion, 0);
    QCOMPARE(result.currentVersion, 1);

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
        };
        for (const auto &table : expected) {
            QVERIFY2(tables.contains(table), qPrintable(QStringLiteral("Tabela ausente: %1").arg(table)));
        }
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    ApplicationDatabaseTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "ApplicationDatabaseTest.moc"
