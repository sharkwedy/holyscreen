#include "library/EventRepository.h"

#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

class EventRepositoryTest final : public QObject {
    Q_OBJECT
private slots:
    void persistsOrderedMixedPlaylist()
    {
        QTemporaryDir directory;
        EventRepository repository(directory.filePath("presenter.db"));
        QVERIFY(repository.open());
        const auto id = repository.saveEvent(Event{.title="Culto de domingo", .scheduledAt="2026-08-16T19:00:00"});
        QVERIFY(!id.isEmpty());
        QVERIFY(repository.addItem(id, PlaylistItem{.type=PlaylistItemType::Song,.referenceId="song",.title="Louvor",.durationMs=180000}));
        QVERIFY(repository.addItem(id, PlaylistItem{.type=PlaylistItemType::Video,.referenceId="video",.title="Aviso",.durationMs=30000}));
        auto items=repository.items(id);
        QCOMPARE(items.size(),2);
        QCOMPARE(repository.totalDurationMs(id),210000);
        QVERIFY(repository.moveItem(items[1].id,0));
        items=repository.items(id);
        QCOMPARE(items[0].referenceId,QStringLiteral("video"));
        QVERIFY(repository.removeItem(items[1].id));
        QCOMPARE(repository.items(id).size(),1);
        QCOMPARE(repository.events().size(),1);
        QVERIFY(repository.removeEvent(id));
        QVERIFY(repository.events().isEmpty());
    }
};

QTEST_MAIN(EventRepositoryTest)
#include "EventRepositoryTest.moc"
