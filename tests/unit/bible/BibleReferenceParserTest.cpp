#include <QTest>

#include "bible/BibleReferenceParser.h"

using namespace churchpresenter;

class BibleReferenceParserTest final : public QObject {
    Q_OBJECT

private slots:
    void convergesCommonJohnFormatsToTheSameReference_data();
    void convergesCommonJohnFormatsToTheSameReference();
    void parsesNumberedBooksAndVerseRanges();
    void distinguishesJoFromAccentedJo();
    void rejectsUnknownOrIncompleteReferences_data();
    void rejectsUnknownOrIncompleteReferences();
};

void BibleReferenceParserTest::convergesCommonJohnFormatsToTheSameReference_data()
{
    QTest::addColumn<QString>("input");
    QTest::newRow("colon") << QStringLiteral("João 3:16");
    QTest::newRow("spaces") << QStringLiteral("Jo 3 16");
    QTest::newRow("dot") << QStringLiteral("João 3.16");
    QTest::newRow("case-and-padding") << QStringLiteral("  JOÃO   3 : 16  ");
}

void BibleReferenceParserTest::convergesCommonJohnFormatsToTheSameReference()
{
    QFETCH(QString, input);
    const auto parsed = BibleReferenceParser{}.parse(input);
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed.value(), BibleReference({BibleBook::John, 3, 16, 16}));
    QCOMPARE(bibleBookName(parsed->book), QStringLiteral("João"));
}

void BibleReferenceParserTest::parsesNumberedBooksAndVerseRanges()
{
    const auto parsed = BibleReferenceParser{}.parse(QStringLiteral("1 Coríntios 13:4-7"));
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed.value(), BibleReference({BibleBook::FirstCorinthians, 13, 4, 7}));
}

void BibleReferenceParserTest::distinguishesJoFromAccentedJo()
{
    const BibleReferenceParser parser;
    QCOMPARE(parser.parse(QStringLiteral("Jo 3:16"))->book, BibleBook::John);
    QCOMPARE(parser.parse(QStringLiteral("Jó 1:1"))->book, BibleBook::Job);
}

void BibleReferenceParserTest::rejectsUnknownOrIncompleteReferences_data()
{
    QTest::addColumn<QString>("input");
    QTest::newRow("empty") << QString{};
    QTest::newRow("unknown-book") << QStringLiteral("Livro 3:16");
    QTest::newRow("missing-verse") << QStringLiteral("João 3");
    QTest::newRow("zero-chapter") << QStringLiteral("João 0:16");
    QTest::newRow("reversed-range") << QStringLiteral("João 3:18-16");
}

void BibleReferenceParserTest::rejectsUnknownOrIncompleteReferences()
{
    QFETCH(QString, input);
    QVERIFY(!BibleReferenceParser{}.parse(input).has_value());
}

QTEST_APPLESS_MAIN(BibleReferenceParserTest)
#include "BibleReferenceParserTest.moc"
