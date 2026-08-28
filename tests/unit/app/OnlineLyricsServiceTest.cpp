#include "app/OnlineLyricsService.h"

#include <QtTest/QTest>

using churchpresenter::OnlineLyricsService;

class OnlineLyricsServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void parsesLrclibResultsAndPlainLyrics();
    void derivesPlainLyricsFromSyncedText();
    void parsesVagalumeSearchAndLyrics();
    void rejectsInvalidProviderPayloads();
    void structuresMarkedPortugueseLyrics();
    void structuresUnmarkedLyricsInFourLineSlides();
};

void OnlineLyricsServiceTest::parsesLrclibResultsAndPlainLyrics()
{
    const QByteArray payload = R"json([
        {"id":42,"trackName":"Raridade","artistName":"Anderson Freire",
         "albumName":"Raridade","duration":280,"instrumental":false,
         "plainLyrics":"Primeira linha\nSegunda linha","syncedLyrics":null},
        {"id":43,"trackName":"Instrumental","artistName":"Banda",
         "instrumental":true,"plainLyrics":null}
    ])json";
    QString error;
    const auto results = OnlineLyricsService::parseLrclibSearch(payload, &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(results.size(), 1);
    const auto result = results.first().toMap();
    QCOMPARE(result.value("key").toString(), QStringLiteral("lrclib:42"));
    QCOMPARE(result.value("title").toString(), QStringLiteral("Raridade"));
    QCOMPARE(result.value("artist").toString(), QStringLiteral("Anderson Freire"));
    QVERIFY(result.value("hasLyrics").toBool());
}

void OnlineLyricsServiceTest::derivesPlainLyricsFromSyncedText()
{
    const QByteArray payload = R"json([
        {"id":7,"trackName":"Teste","artistName":"Autor","instrumental":false,
         "plainLyrics":null,"syncedLyrics":"[00:01.20]Linha um\n[00:04.00]Linha dois"}
    ])json";
    const auto result = OnlineLyricsService::parseLrclibSearch(payload).first().toMap();
    QCOMPARE(result.value("lyrics").toString(), QStringLiteral("Linha um\nLinha dois"));
}

void OnlineLyricsServiceTest::parsesVagalumeSearchAndLyrics()
{
    const QByteArray search = R"json({"response":{"docs":[
        {"id":"M3ade68b4gdc96eda3","title":"Bondade de Deus","band":"Isaias Saad"}
    ]}})json";
    const auto result = OnlineLyricsService::parseVagalumeSearch(search).first().toMap();
    QCOMPARE(result.value("provider").toString(), QStringLiteral("Vagalume"));
    QCOMPARE(result.value("title").toString(), QStringLiteral("Bondade de Deus"));
    QVERIFY(!result.value("hasLyrics").toBool());

    QString error;
    QCOMPARE(OnlineLyricsService::parseVagalumeLyrics(
                 R"json({"mus":[{"text":"Tu és fiel\nEm todo tempo"}]})json", &error),
             QStringLiteral("Tu és fiel\nEm todo tempo"));
    QVERIFY(error.isEmpty());
}

void OnlineLyricsServiceTest::rejectsInvalidProviderPayloads()
{
    QString error;
    QVERIFY(OnlineLyricsService::parseLrclibSearch("{}", &error).isEmpty());
    QVERIFY(!error.isEmpty());
    error.clear();
    QVERIFY(OnlineLyricsService::parseVagalumeSearch("[]", &error).isEmpty());
    QVERIFY(!error.isEmpty());
}

void OnlineLyricsServiceTest::structuresMarkedPortugueseLyrics()
{
    const auto structured = OnlineLyricsService::toStructuredLyrics(
        QStringLiteral("[Verso 1]\nLinha um\nLinha dois\n\n[Refrão]\nSanto, santo\n\n[Ponte]\nAleluia"));
    QCOMPARE(structured,
             QStringLiteral("VERSO 1\nLinha um\nLinha dois\n\nREFRÃO\nSanto, santo\n\nPONTE\nAleluia"));
}

void OnlineLyricsServiceTest::structuresUnmarkedLyricsInFourLineSlides()
{
    const auto structured = OnlineLyricsService::toStructuredLyrics(
        QStringLiteral("[ar:Autor]\n[00:01]Um\n[00:02]Dois\n[00:03]Três\n[00:04]Quatro\n[00:05]Cinco"));
    QCOMPARE(structured,
             QStringLiteral("VERSO 1\nUm\nDois\nTrês\nQuatro\n\nVERSO 2\nCinco"));
}

QTEST_GUILESS_MAIN(OnlineLyricsServiceTest)
#include "OnlineLyricsServiceTest.moc"
