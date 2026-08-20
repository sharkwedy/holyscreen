#include <QtTest/QTest>

#include "persistence/SettingsRepository.h"

#include <QTemporaryDir>

using namespace churchpresenter;

class SettingsRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void savesAndReloadsPresentationSettings();
};

void SettingsRepositoryTest::savesAndReloadsPresentationSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto databasePath = directory.filePath(QStringLiteral("presenter.db"));

    {
        SettingsRepository repository(databasePath);
        QVERIFY(repository.open());
        QVERIFY(repository.setValue(QStringLiteral("presentation/wallpaperColor"), QStringLiteral("#112233")));
        QVERIFY(repository.setValue(QStringLiteral("presentation/clockVisible"), false));
    }

    SettingsRepository reopened(databasePath);
    QVERIFY(reopened.open());
    QCOMPARE(reopened.value(QStringLiteral("presentation/wallpaperColor"), QStringLiteral("#000000")).toString(), QStringLiteral("#112233"));
    QCOMPARE(reopened.value(QStringLiteral("presentation/clockVisible"), true).toBool(), false);
}

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    SettingsRepositoryTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "SettingsRepositoryTest.moc"
