#include "library/BroadcastProfileRepository.h"
#include "persistence/ApplicationDatabase.h"

#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

class BroadcastProfileRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void savesAndReloadsAProfilePerOutput();
    void returnsDefaultsForOutputsWithoutAProfile();
    void updatesAnExistingProfileWithoutDuplicatingRows();
    void refusesProfilesWithoutAnOutput();
    void removesAProfile();
};

void BroadcastProfileRepositoryTest::savesAndReloadsAProfilePerOutput()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);

    BroadcastProfileRepository repository(path);
    QVERIFY(repository.open());

    const BroadcastProfile transparent{
        .screenFingerprint = QStringLiteral("hdmi-2"),
        .backgroundMode = BroadcastBackgroundMode::Transparent,
        .chromaColor = QStringLiteral("#00ff00"),
        .safeArea = QMarginsF(3.0, 4.0, 5.0, 6.0),
        .aspectPreset = BroadcastAspectPreset::Portrait,
        .showClock = true,
        .showLowerThird = false,
        .showAlerts = true,
        .showAudienceMessage = false,
    };
    const BroadcastProfile chroma{.screenFingerprint = QStringLiteral("hdmi-3")};

    QVERIFY(repository.save(transparent));
    QVERIFY(repository.save(chroma));

    QCOMPARE(repository.find(QStringLiteral("hdmi-2")), std::optional<BroadcastProfile>{transparent});
    QCOMPARE(repository.all().size(), 2);

    BroadcastProfileRepository reopened(path);
    QVERIFY(reopened.open());
    const auto reloaded = reopened.find(QStringLiteral("hdmi-2"));
    QVERIFY(reloaded.has_value());
    QCOMPARE(*reloaded, transparent);
}

void BroadcastProfileRepositoryTest::returnsDefaultsForOutputsWithoutAProfile()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    BroadcastProfileRepository repository(path);
    QVERIFY(repository.open());

    QCOMPARE(repository.find(QStringLiteral("unknown")), std::nullopt);
    const auto fallback = repository.findOrDefault(QStringLiteral("unknown"));
    QCOMPARE(fallback.screenFingerprint, QStringLiteral("unknown"));
    QCOMPARE(fallback.backgroundMode, BroadcastBackgroundMode::Chroma);
    QCOMPARE(fallback.chromaColor, QStringLiteral("#00b140"));
    QVERIFY(fallback.showLowerThird);
    QVERIFY(!fallback.showClock);
}

void BroadcastProfileRepositoryTest::updatesAnExistingProfileWithoutDuplicatingRows()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    BroadcastProfileRepository repository(path);
    QVERIFY(repository.open());

    BroadcastProfile profile{.screenFingerprint = QStringLiteral("hdmi-2")};
    QVERIFY(repository.save(profile));
    profile.backgroundMode = BroadcastBackgroundMode::Transparent;
    profile.safeArea = QMarginsF(10.0, 10.0, 10.0, 10.0);
    QVERIFY(repository.save(profile));

    QCOMPARE(repository.all().size(), 1);
    QCOMPARE(repository.find(QStringLiteral("hdmi-2"))->backgroundMode,
             BroadcastBackgroundMode::Transparent);
}

void BroadcastProfileRepositoryTest::refusesProfilesWithoutAnOutput()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    BroadcastProfileRepository repository(path);
    QVERIFY(repository.open());

    QVERIFY(!repository.save(BroadcastProfile{}));
    QVERIFY(repository.all().isEmpty());
}

void BroadcastProfileRepositoryTest::removesAProfile()
{
    QTemporaryDir directory;
    const auto path = directory.filePath(QStringLiteral("presenter.db"));
    QVERIFY(ApplicationDatabase::migrate(path).success);
    BroadcastProfileRepository repository(path);
    QVERIFY(repository.open());

    QVERIFY(repository.save(BroadcastProfile{.screenFingerprint = QStringLiteral("hdmi-2")}));
    QVERIFY(repository.remove(QStringLiteral("hdmi-2")));
    QVERIFY(repository.all().isEmpty());
}

QTEST_MAIN(BroadcastProfileRepositoryTest)
#include "BroadcastProfileRepositoryTest.moc"
