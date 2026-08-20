#include <QTest>

#include "bible/BibleJsonImporter.h"

using namespace churchpresenter;

class BibleJsonImporterTest final : public QObject {
    Q_OBJECT

private slots:
    void importsTranslationMetadataAndVerses();
    void rejectsMalformedOrIncompleteDocuments_data();
    void rejectsMalformedOrIncompleteDocuments();
};

void BibleJsonImporterTest::importsTranslationMetadataAndVerses()
{
    const auto result = BibleJsonImporter{}.parse(R"JSON({
        "translation": {"name":"Português Demo","abbreviation":"PTD","language":"pt-BR"},
        "verses": [
            {"book":"João","chapter":3,"verse":16,"text":"Texto dezesseis"},
            {"book":43,"chapter":3,"verse":17,"text":"Texto dezessete"}
        ]
    })JSON");

    QVERIFY2(result.isValid(), qPrintable(result.error));
    QCOMPARE(result.translation.name, QStringLiteral("Português Demo"));
    QCOMPARE(result.translation.abbreviation, QStringLiteral("PTD"));
    QCOMPARE(result.verses.size(), 2);
    QCOMPARE(result.verses.at(0).book, BibleBook::John);
    QCOMPARE(result.verses.at(1).book, BibleBook::John);
}

void BibleJsonImporterTest::rejectsMalformedOrIncompleteDocuments_data()
{
    QTest::addColumn<QByteArray>("json");
    QTest::newRow("invalid-json") << QByteArray("{");
    QTest::newRow("missing-translation") << QByteArray(R"({"verses":[]})");
    QTest::newRow("unknown-book") << QByteArray(R"({
        "translation":{"name":"Demo","abbreviation":"D","language":"pt"},
        "verses":[{"book":"Desconhecido","chapter":1,"verse":1,"text":"x"}]
    })");
    QTest::newRow("empty-text") << QByteArray(R"({
        "translation":{"name":"Demo","abbreviation":"D","language":"pt"},
        "verses":[{"book":"João","chapter":1,"verse":1,"text":""}]
    })");
}

void BibleJsonImporterTest::rejectsMalformedOrIncompleteDocuments()
{
    QFETCH(QByteArray, json);
    const auto result = BibleJsonImporter{}.parse(json);
    QVERIFY(!result.isValid());
    QVERIFY(!result.error.isEmpty());
}

QTEST_APPLESS_MAIN(BibleJsonImporterTest)
#include "BibleJsonImporterTest.moc"
