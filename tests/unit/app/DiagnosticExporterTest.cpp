#include "app/DiagnosticExporter.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <miniz.h>

using namespace churchpresenter;

class DiagnosticExporterTest final : public QObject {
    Q_OBJECT

private slots:
    void exportsSanitizedJsonAndLogsWithoutSensitiveValues();
};

void DiagnosticExporterTest::exportsSanitizedJsonAndLogsWithoutSensitiveValues()
{
    QTemporaryDir directory;
    QFile log(directory.filePath(QStringLiteral("holyscreen.log")));
    QVERIFY(log.open(QIODevice::WriteOnly));
    QVERIFY(log.write("2026-08-22 INFO initialized\n") > 0);
    log.close();
    const auto zipPath = directory.filePath(QStringLiteral("diagnostics.zip"));

    QString error;
    QVERIFY2(DiagnosticExporter::exportZip({
        .destinationPath = zipPath,
        .application = {{QStringLiteral("version"), QStringLiteral("0.11.0")},
                        {QStringLiteral("schemaVersion"), 2}},
        .screens = {QVariantMap{{QStringLiteral("name"), QStringLiteral("Projetor")}}},
        .configuration = {{QStringLiteral("wallpaperFit"), QStringLiteral("cover")},
                          {QStringLiteral("remotePassword"), QStringLiteral("segredo")},
                          {QStringLiteral("apiToken"), QStringLiteral("token-secreto")}},
        .logPath = log.fileName(),
    }, &error), qPrintable(error));

    mz_zip_archive archive{};
    QVERIFY(mz_zip_reader_init_file(&archive, zipPath.toUtf8().constData(), 0));
    QVERIFY(mz_zip_reader_locate_file(&archive, "diagnostics.json", nullptr, 0) >= 0);
    QVERIFY(mz_zip_reader_locate_file(&archive, "logs/holyscreen.log", nullptr, 0) >= 0);
    size_t size = 0;
    const auto *contents = static_cast<const char *>(
        mz_zip_reader_extract_file_to_heap(&archive, "diagnostics.json", &size, 0));
    QVERIFY(contents != nullptr);
    const QByteArray json(contents, static_cast<qsizetype>(size));
    mz_free(const_cast<char *>(contents));
    mz_zip_reader_end(&archive);
    QVERIFY(json.contains("wallpaperFit"));
    QVERIFY(!json.contains("segredo"));
    QVERIFY(!json.contains("token-secreto"));
    QVERIFY(!json.contains("remotePassword"));
    QVERIFY(!json.contains("apiToken"));
}

QTEST_APPLESS_MAIN(DiagnosticExporterTest)
#include "DiagnosticExporterTest.moc"
