#include "library/HistoryRepository.h"

#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

class HistoryRepositoryTest final : public QObject {
    Q_OBJECT
private slots:
    void recordsAndReportsExecutions();
};

void HistoryRepositoryTest::recordsAndReportsExecutions()
{
    QTemporaryDir directory;
    HistoryRepository repository(directory.filePath(QStringLiteral("presenter.db")));
    QVERIFY(repository.open());
    QVERIFY(repository.record(HistoryEntry{.itemType = "song", .referenceId = "s1",
                                           .title = "Louvor", .eventId = "e1",
                                           .executedAt = "2026-08-13T19:00:00"}));
    QVERIFY(repository.record(HistoryEntry{.itemType = "video", .referenceId = "v1",
                                           .title = "Aviso", .eventId = "e1",
                                           .executedAt = "2026-08-13T19:05:00"}));
    QVERIFY(repository.record(HistoryEntry{.itemType = "song", .referenceId = "s1",
                                           .title = "Louvor", .eventId = "e2",
                                           .executedAt = "2026-08-14T19:00:00"}));

    QCOMPARE(repository.entries().size(), 3);
    const auto report = repository.report();
    QCOMPARE(report.totalExecutions, 3);
    QCOMPARE(report.byType.value(QStringLiteral("song")), 2);
    QCOMPARE(report.mostExecutedTitle, QStringLiteral("Louvor"));
    QVERIFY(repository.clear());
    QVERIFY(repository.entries().isEmpty());
}

QTEST_MAIN(HistoryRepositoryTest)
#include "HistoryRepositoryTest.moc"
