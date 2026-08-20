#include "library/PresentationRepository.h"

#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

class PresentationRepositoryTest final : public QObject {
    Q_OBJECT
private slots:
    void persistsPresentationsAndOrderedSlides()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto database = directory.filePath("presenter.db");
        QString id;
        {
            PresentationRepository repository(database);
            QVERIFY(repository.open());
            Presentation item{.type = PresentationType::Text, .title = "Avisos", .author = "Equipe"};
            item.slides = {{.id="a",.label = "A", .text = "Um"}, {.id="b",.label = "B", .text = "Dois", .order = 1}};
            item.sequence = {"a", "b", "a"};
            id = repository.save(item);
            QVERIFY(!id.isEmpty());
        }
        PresentationRepository reopened(database);
        QVERIFY(reopened.open());
        const auto item = reopened.presentation(id);
        QCOMPARE(item.title, QStringLiteral("Avisos"));
        QCOMPARE(item.slides.size(), 2);
        QCOMPARE(item.slides[1].text, QStringLiteral("Dois"));
        QCOMPARE(item.sequence, QStringList({"a","b","a"}));
        QCOMPARE(reopened.presentations(PresentationType::Text).size(), 1);
        QVERIFY(reopened.remove(id));
        QVERIFY(reopened.presentations(PresentationType::Text).isEmpty());
    }
};

QTEST_MAIN(PresentationRepositoryTest)
#include "PresentationRepositoryTest.moc"
