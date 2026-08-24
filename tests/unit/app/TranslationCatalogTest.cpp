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

QString qmlStringValue(QString value)
{
    value.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    value.replace(QStringLiteral("\\r"), QStringLiteral("\r"));
    value.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
    value.replace(QStringLiteral("\\\""), QStringLiteral("\""));
    value.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    return value;
}

} // namespace

class TranslationCatalogTest final : public QObject {
    Q_OBJECT

private slots:
    void migratedSurfacesUseCataloguedVisibleStrings();
};

void TranslationCatalogTest::migratedSurfacesUseCataloguedVisibleStrings()
{
    const QStringList qmlFiles{
        QStringLiteral("src/ui/operator/EventsArea.qml"),
        QStringLiteral("src/ui/operator/EventsDialog.qml"),
        QStringLiteral("src/ui/operator/OperatorHeader.qml"),
        QStringLiteral("src/ui/operator/MaintenanceArea.qml"),
        QStringLiteral("src/ui/operator/OnboardingDialog.qml"),
        QStringLiteral("src/ui/operator/BibleBrowser.qml"),
        QStringLiteral("src/ui/operator/QuickBibleSearch.qml"),
        QStringLiteral("src/ui/operator/BroadcastSettings.qml"),
        QStringLiteral("src/ui/output/StageView.qml"),
        QStringLiteral("src/ui/output/OutputWindow.qml"),
        QStringLiteral("src/ui/operator/IntegrationsArea.qml"),
        QStringLiteral("src/ui/operator/AutomationsArea.qml"),
        QStringLiteral("src/ui/operator/Dashboard.qml"),
        QStringLiteral("src/ui/operator/BiblePanel.qml"),
        QStringLiteral("src/ui/operator/LibraryPanel.qml"),
        QStringLiteral("src/ui/operator/PlaylistPanel.qml"),
        QStringLiteral("src/ui/operator/PlayerButton.qml"),
        QStringLiteral("src/ui/operator/PlaybackPanel.qml"),
        QStringLiteral("src/ui/operator/MediaLibraryDialog.qml"),
        QStringLiteral("src/ui/operator/BibleSettingsFlow.qml"),
        QStringLiteral("src/ui/operator/LiveCommunicationDialog.qml"),
        QStringLiteral("src/ui/operator/MaintenanceDialogs.qml"),
        QStringLiteral("src/ui/operator/MediaImportFlow.qml"),
        QStringLiteral("src/ui/operator/OperatorShortcuts.qml"),
        QStringLiteral("src/ui/operator/SettingsDialog.qml"),
        QStringLiteral("src/ui/operator/MainWindow.qml"),
        QStringLiteral("src/ui/output/AudienceView.qml"),
        QStringLiteral("src/ui/output/BroadcastPreview.qml"),
        QStringLiteral("src/ui/output/BroadcastView.qml"),
        QStringLiteral("src/ui/output/LiveOverlays.qml"),
        QStringLiteral("src/ui/output/OutputClock.qml"),
        QStringLiteral("src/ui/output/PresentationImageLayer.qml"),
        QStringLiteral("src/ui/output/PresentationTextLayer.qml"),
        QStringLiteral("src/ui/output/SimulatedOutput.qml"),
        QStringLiteral("src/ui/output/StageOutputView.qml"),
    };
    QSet<QString> sources;
    static const QRegularExpression translated(
        QStringLiteral("qsTr\\(\\\"([^\\\"]+)\\\"\\)"));
    static const QRegularExpression rawVisible(
        QStringLiteral("(?:text|title|placeholderText|Accessible\\.name|ToolTip\\.text)"
                       "\\s*:\\s*\\\"(?!https?://|#[0-9A-Fa-f]{3,8}\\\")"
                       "[^\\\";\\r\\n]*\\p{L}"));
    static const QRegularExpression legacyMediaAlias(
        QStringLiteral("controller\\.(?:songs|songSearch|importAudioFiles|importVideoFiles|"
                       "importImageFiles|selectSong|mediaPlaylist|mediaFolders|folderAudioFiles|"
                       "folderVideoFiles|folderImageFiles|favoriteMedia|audioFileSearch|"
                       "videoFileSearch|imageFileSearch|currentMediaId|currentMediaTitle|"
                       "currentMediaType|mediaState|mediaPositionMs|mediaDurationMs|"
                       "mediaVolume|mediaRepeatMode|addMediaFolder|removeMediaFolder|"
                       "rescanMediaFolders|addCatalogFileToPlaylist|isFavoriteMedia|"
                       "toggleFavoriteMedia|openFileLocation|moveMedia|removeMedia|"
                       "playMedia|toggleMediaPause|stopMedia|seekMedia|previousMedia|"
                       "nextMedia|shuffleMediaPlaylist|clearMediaPlaylist|saveMediaPlaylist)\\b"));
    static const QRegularExpression legacyOutputAlias(
        QStringLiteral("controller\\.(?:screens|outputWindows|blackout|identifyVisible|"
                       "toggleScreen|enableAllScreens|setOutputBibleTranslation|setOutputRole|"
                       "setOutputMediaEnabled|outputBroadcastProfile|setOutputBroadcastProfile|"
                       "setOutputDisplayName|identifyScreens)\\b"));
    static const QRegularExpression legacyBibleAlias(
        QStringLiteral("controller\\.(?:bibleTranslations|bibleBooks|biblePrimaryTranslationId|"
                       "bibleSecondaryTranslationId|bibleTertiaryTranslationId|bibleReferenceInput|"
                       "bibleResults|bibleImportRunning|bibleImportProgress|bibleImportMessage|"
                       "bibleImportRequiresLicenseConfirmation|bibleImportLicenseWarning|"
                       "importBibleTranslation|importBibleFolder|importBibleGit|importBibleZip|"
                       "confirmBibleImportLicenses|cancelBibleImport|updateBibleTranslationFromSource|"
                       "searchBibleReference|showBibleVerse|bibleChapterNumbers|bibleVerseNumbers|"
                       "presentBibleReference|bibleTextForSlide)\\b"));
    for (const auto &path : qmlFiles) {
        const auto contents = QString::fromUtf8(readFile(path));
        QVERIFY2(!contents.isEmpty(), qPrintable(QStringLiteral("Não foi possível ler %1").arg(path)));
        QVERIFY2(!rawVisible.match(contents).hasMatch(),
                 qPrintable(QStringLiteral("String visível sem qsTr em %1").arg(path)));
        const bool receivesMediaContext = path.endsWith(QStringLiteral("/LibraryPanel.qml"))
            || path.endsWith(QStringLiteral("/PlaylistPanel.qml"))
            || path.endsWith(QStringLiteral("/MediaLibraryDialog.qml"))
            || path.endsWith(QStringLiteral("/MediaImportFlow.qml"));
        QVERIFY2(receivesMediaContext || !legacyMediaAlias.match(contents).hasMatch(),
                 qPrintable(QStringLiteral("Alias legado de mídia usado em %1").arg(path)));
        QVERIFY2(!legacyOutputAlias.match(contents).hasMatch(),
                 qPrintable(QStringLiteral("Alias legado de saída usado em %1").arg(path)));
        const bool receivesBibleContext = path.endsWith(QStringLiteral("/BiblePanel.qml"))
            || path.endsWith(QStringLiteral("/BibleBrowser.qml"))
            || path.endsWith(QStringLiteral("/QuickBibleSearch.qml"))
            || path.endsWith(QStringLiteral("/BibleSettingsFlow.qml"));
        QVERIFY2(receivesBibleContext || !legacyBibleAlias.match(contents).hasMatch(),
                 qPrintable(QStringLiteral("Alias legado de Bíblia usado em %1").arg(path)));
        auto matches = translated.globalMatch(contents);
        while (matches.hasNext()) sources.insert(qmlStringValue(matches.next().captured(1)));
    }

    const auto controller = QString::fromUtf8(
        readFile(QStringLiteral("src/app/ApplicationController.cpp")));
    QVERIFY(!controller.isEmpty());
    static const QRegularExpression translatedController(
        QStringLiteral("(?:\\btr\\(\\\"([^\\\"]+)\\\"\\)"
                       "|QCoreApplication::translate\\(\\\"ApplicationController\\\","
                       "\\s*\\\"([^\\\"]+)\\\"\\))"));
    static const QRegularExpression rawController(
        QStringLiteral("QStringLiteral\\(\\\"[^\\\"\\r\\n]*[\\x{00C0}-\\x{017F}]"));
    QVERIFY2(!rawController.match(controller).hasMatch(),
             "String C++ visível sem tr() em ApplicationController.cpp");
    auto controllerMatches = translatedController.globalMatch(controller);
    while (controllerMatches.hasNext()) {
        const auto match = controllerMatches.next();
        sources.insert(qmlStringValue(match.captured(1).isEmpty()
                                          ? match.captured(2) : match.captured(1)));
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
