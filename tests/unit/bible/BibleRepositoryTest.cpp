#include <QTemporaryDir>
#include <QTest>

#include "bible/BibleRepository.h"

using namespace churchpresenter;

class BibleRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void storesTranslationsAndVerseRangesIndependently();
    void searchesVerseTextWithinOneTranslation();
};

void BibleRepositoryTest::storesTranslationsAndVerseRangesIndependently()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    BibleRepository repository(directory.filePath(QStringLiteral("presenter.db")));
    QVERIFY(repository.open());

    const auto pt = repository.saveTranslation({{}, QStringLiteral("Português Demo"), QStringLiteral("PTD"), QStringLiteral("pt-BR")});
    const auto en = repository.saveTranslation({{}, QStringLiteral("English Demo"), QStringLiteral("END"), QStringLiteral("en")});
    QVERIFY(!pt.isEmpty());
    QVERIFY(!en.isEmpty());
    QCOMPARE(repository.translations().size(), 2);

    QVERIFY(repository.importVerses(pt, {
        {pt, BibleBook::John, 3, 16, QStringLiteral("Texto dezesseis")},
        {pt, BibleBook::John, 3, 17, QStringLiteral("Texto dezessete")},
        {pt, BibleBook::John, 4, 1, QStringLiteral("Texto do capítulo quatro")},
    }));
    QVERIFY(repository.importVerses(en, {
        {en, BibleBook::John, 3, 16, QStringLiteral("English sixteen")},
    }));

    const auto range = repository.verses(pt, {BibleBook::John, 3, 16, 17});
    QCOMPARE(range.size(), 2);
    QCOMPARE(range.at(0).verse, 16);
    QCOMPARE(range.at(1).verse, 17);
    QCOMPARE(repository.verses(en, {BibleBook::John, 3, 16, 16}).front().text,
             QStringLiteral("English sixteen"));
    QCOMPARE(repository.chapters(pt, BibleBook::John), QVector<int>({3, 4}));
    QCOMPARE(repository.verseNumbers(pt, BibleBook::John, 3), QVector<int>({16, 17}));
    QVERIFY(repository.verseNumbers(pt, BibleBook::John, 99).isEmpty());
}

void BibleRepositoryTest::searchesVerseTextWithinOneTranslation()
{
    QTemporaryDir directory;
    BibleRepository repository(directory.filePath(QStringLiteral("presenter.db")));
    QVERIFY(repository.open());
    const auto translation = repository.saveTranslation(
        {{}, QStringLiteral("Demo"), QStringLiteral("DMO"), QStringLiteral("pt-BR")});
    QVERIFY(repository.importVerses(translation, {
        {translation, BibleBook::Psalms, 23, 1, QStringLiteral("O Senhor é meu pastor")},
        {translation, BibleBook::Psalms, 23, 2, QStringLiteral("Deitar-me faz em verdes pastos")},
    }));

    const auto found = repository.search(translation, QStringLiteral("PASTOR"));
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.front().verse, 1);
}

QTEST_GUILESS_MAIN(BibleRepositoryTest)
#include "BibleRepositoryTest.moc"
