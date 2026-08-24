#include <QtTest/QTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

namespace {

QByteArray readWebFile(const QString &name)
{
    QFile file(QStringLiteral(HOLYSCREEN_SOURCE_DIR "/src/remote/web/") + name);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

QSet<QString> placeholders(const QString &text)
{
    QSet<QString> result;
    static const QRegularExpression expression(QStringLiteral("%[1-9][0-9]*"));
    auto matches = expression.globalMatch(text);
    while (matches.hasNext()) result.insert(matches.next().captured());
    return result;
}

} // namespace

class PwaTranslationTest final : public QObject {
    Q_OBJECT

private slots:
    void everyVisibleAndRuntimeKeyExistsInBothLocales();
    void serviceWorkerReplacesThePreviousOfflineCache();
};

void PwaTranslationTest::everyVisibleAndRuntimeKeyExistsInBothLocales()
{
    const auto htmlBytes = readWebFile(QStringLiteral("index.html"));
    QVERIFY(!htmlBytes.isEmpty());
    const auto html = QString::fromUtf8(htmlBytes);

    static const QRegularExpression catalogExpression(
        QStringLiteral(R"(<script id="translations" type="application/json">\s*([\s\S]*?)\s*</script>)"));
    const auto catalogMatch = catalogExpression.match(html);
    QVERIFY2(catalogMatch.hasMatch(), "PWA translation JSON was not found");

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(catalogMatch.captured(1).toUtf8(), &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QVERIFY(document.isObject());

    const auto catalogs = document.object();
    const auto portuguese = catalogs.value(QStringLiteral("pt-BR")).toObject();
    const auto english = catalogs.value(QStringLiteral("en-US")).toObject();
    QVERIFY(!portuguese.isEmpty());
    const auto portugueseKeys = portuguese.keys();
    const auto englishKeys = english.keys();
    QCOMPARE(QSet<QString>(portugueseKeys.cbegin(), portugueseKeys.cend()),
             QSet<QString>(englishKeys.cbegin(), englishKeys.cend()));

    QSet<QString> usedKeys;
    static const QRegularExpression attributeExpression(
        QStringLiteral("data-i18n(?:-placeholder|-aria-label)?=\"([^\"]+)\""));
    auto attributeMatches = attributeExpression.globalMatch(html);
    while (attributeMatches.hasNext()) usedKeys.insert(attributeMatches.next().captured(1));

    static const QRegularExpression runtimeExpression(
        QStringLiteral("\\bt\\('([^']+)'\\)"));
    auto runtimeMatches = runtimeExpression.globalMatch(html);
    while (runtimeMatches.hasNext()) usedKeys.insert(runtimeMatches.next().captured(1));

    QVERIFY(usedKeys.size() > 50);
    for (const auto &key : usedKeys) {
        QVERIFY2(portuguese.contains(key), qPrintable(QStringLiteral("pt-BR missing %1").arg(key)));
        QVERIFY2(english.contains(key), qPrintable(QStringLiteral("en-US missing %1").arg(key)));
        const auto ptText = portuguese.value(key).toString();
        const auto enText = english.value(key).toString();
        QVERIFY2(!ptText.trimmed().isEmpty(), qPrintable(QStringLiteral("pt-BR empty %1").arg(key)));
        QVERIFY2(!enText.trimmed().isEmpty(), qPrintable(QStringLiteral("en-US empty %1").arg(key)));
        QCOMPARE(placeholders(enText), placeholders(ptText));
    }

    QVERIFY(html.contains(QStringLiteral("localStorage.getItem('hs-locale')")));
    QVERIFY(html.contains(QStringLiteral("navigator.language")));
    QVERIFY(html.contains(QStringLiteral("document.documentElement.lang=locale")));
}

void PwaTranslationTest::serviceWorkerReplacesThePreviousOfflineCache()
{
    const auto serviceWorker = readWebFile(QStringLiteral("sw.js"));
    QVERIFY(serviceWorker.contains("holyscreen-remote-v2"));
    QVERIFY(serviceWorker.contains("caches.delete"));
    QVERIFY(!serviceWorker.contains("holyscreen-remote-v1"));
}

QTEST_GUILESS_MAIN(PwaTranslationTest)
#include "PwaTranslationTest.moc"
