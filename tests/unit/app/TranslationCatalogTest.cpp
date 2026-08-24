#include <QtTest/QTest>

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QXmlStreamReader>

namespace {

struct CatalogMessage {
    QString translation;
    bool unfinished = false;
};

QByteArray readFile(const QString &relativePath)
{
    QFile file(QStringLiteral(HOLYSCREEN_SOURCE_DIR) + u'/' + relativePath);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

QHash<QString, CatalogMessage> readCatalog(const QString &relativePath, QStringList &errors)
{
    QHash<QString, CatalogMessage> messages;
    QXmlStreamReader xml(readFile(relativePath));
    QString source;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) continue;
        if (xml.name() == QStringLiteral("source")) {
            source = xml.readElementText();
        } else if (xml.name() == QStringLiteral("translation")) {
            const auto unfinished = xml.attributes().value(QStringLiteral("type"))
                == QStringLiteral("unfinished");
            messages.insert(source, {xml.readElementText(), unfinished});
        }
    }
    if (xml.hasError()) errors.append(xml.errorString());
    return messages;
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

class TranslationCatalogTest final : public QObject {
    Q_OBJECT

private slots:
    void migratedQmlUsesCataloguedVisibleStrings();
};

void TranslationCatalogTest::migratedQmlUsesCataloguedVisibleStrings()
{
    const QStringList qmlFiles{
        QStringLiteral("src/ui/operator/EventsArea.qml"),
        QStringLiteral("src/ui/operator/MaintenanceArea.qml"),
        QStringLiteral("src/ui/operator/OnboardingDialog.qml"),
        QStringLiteral("src/ui/operator/BibleBrowser.qml"),
        QStringLiteral("src/ui/operator/QuickBibleSearch.qml"),
        QStringLiteral("src/ui/operator/BroadcastSettings.qml"),
        QStringLiteral("src/ui/output/StageView.qml"),
        QStringLiteral("src/ui/output/OutputWindow.qml"),
        QStringLiteral("src/ui/operator/IntegrationsArea.qml"),
        QStringLiteral("src/ui/operator/AutomationsArea.qml"),
    };
    QSet<QString> sources;
    static const QRegularExpression translated(
        QStringLiteral("qsTr\\(\\\"([^\\\"]+)\\\"\\)"));
    static const QRegularExpression rawVisible(
        QStringLiteral("(?:text|placeholderText|Accessible\\.name)\\s*:\\s*\\\"[^\\\";\\r\\n]*\\p{L}"));
    for (const auto &path : qmlFiles) {
        const auto contents = QString::fromUtf8(readFile(path));
        QVERIFY2(!contents.isEmpty(), qPrintable(QStringLiteral("Não foi possível ler %1").arg(path)));
        QVERIFY2(!rawVisible.match(contents).hasMatch(),
                 qPrintable(QStringLiteral("String visível sem qsTr em %1").arg(path)));
        auto matches = translated.globalMatch(contents);
        while (matches.hasNext()) sources.insert(matches.next().captured(1));
    }
    QVERIFY(!sources.isEmpty());

    for (const auto &catalogPath : {
             QStringLiteral("translations/holyscreen_pt_BR.ts"),
             QStringLiteral("translations/holyscreen_en_US.ts")}) {
        QStringList parseErrors;
        const auto catalog = readCatalog(catalogPath, parseErrors);
        QVERIFY2(parseErrors.isEmpty(), qPrintable(parseErrors.join(u'\n')));
        for (const auto &source : sources) {
            QVERIFY2(catalog.contains(source),
                     qPrintable(QStringLiteral("%1 ausente em %2").arg(source, catalogPath)));
            const auto message = catalog.value(source);
            QVERIFY2(!message.unfinished && !message.translation.trimmed().isEmpty(),
                     qPrintable(QStringLiteral("%1 inacabada em %2").arg(source, catalogPath)));
            QCOMPARE(placeholders(message.translation), placeholders(source));
        }
    }
}

QTEST_GUILESS_MAIN(TranslationCatalogTest)
#include "TranslationCatalogTest.moc"
