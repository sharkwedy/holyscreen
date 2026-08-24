#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "app/AppLogger.h"
#include "app/ApplicationController.h"
#include "app/EnduranceRunner.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

using namespace churchpresenter;

class EnduranceRunnerTest final : public QObject {
    Q_OBJECT

private slots:
    void shortSessionProducesAnApprovedReport();
    void unusableMediaDirectoryBlocksTheSession();
};

void EnduranceRunnerTest::shortSessionProducesAnApprovedReport()
{
    QTemporaryDir reportDirectory;
    QVERIFY(reportDirectory.isValid());
    const auto reportPath = reportDirectory.filePath(QStringLiteral("endurance.json"));

    ApplicationController controller;
    EnduranceOptions options;
    options.durationSeconds = 3;
    options.actionsPerMinute = 600;
    options.reportPath = reportPath;
    EnduranceRunner runner(controller, options);

    QSignalSpy completed(&runner, &EnduranceRunner::completed);
    runner.start();
    QVERIFY(completed.wait(15000));
    QVERIFY(runner.isFinished());

    const auto report = runner.report();
    QCOMPARE(report.value(QStringLiteral("schema")).toString(),
             QStringLiteral("holyscreen.endurance/1"));

    const auto run = report.value(QStringLiteral("run")).toObject();
    QVERIFY(run.value(QStringLiteral("actions")).toDouble() > 0.0);
    QCOMPARE(run.value(QStringLiteral("plannedSeconds")).toInt(), 3);
    QVERIFY(run.value(QStringLiteral("elapsedSeconds")).toDouble() >= 2.5);
    QVERIFY(!run.value(QStringLiteral("actionsByType")).toObject().isEmpty());
    QCOMPARE(run.value(QStringLiteral("slides")).toInt(), 12);

    const auto metrics = report.value(QStringLiteral("metrics")).toObject();
    QVERIFY(metrics.value(QStringLiteral("samples")).toInt() > 0);
    QCOMPARE(metrics.value(QStringLiteral("commandsRejected")).toDouble(), 0.0);
    QCOMPARE(metrics.value(QStringLiteral("logCriticals")).toDouble(), 0.0);
    QVERIFY(metrics.contains(QStringLiteral("eventLoopLagMsP95")));
    QVERIFY(metrics.contains(QStringLiteral("residentBytesPeak")));

    const auto environment = report.value(QStringLiteral("environment")).toObject();
    QVERIFY(!environment.value(QStringLiteral("operatingSystem")).toString().isEmpty());
    QVERIFY(environment.value(QStringLiteral("logicalCores")).toInt() > 0);

    const auto verdict = report.value(QStringLiteral("verdict")).toObject();
    QVERIFY2(verdict.value(QStringLiteral("passed")).toBool(),
             qPrintable(QJsonDocument(verdict.value(QStringLiteral("blockers")).toArray())
                            .toJson(QJsonDocument::Compact)));
    QVERIFY(runner.passed());

    QString error;
    QVERIFY2(runner.writeReport(&error), qPrintable(error));
    QFile written(reportPath);
    QVERIFY(written.open(QIODevice::ReadOnly));
    const auto parsed = QJsonDocument::fromJson(written.readAll());
    QVERIFY(parsed.isObject());
    QCOMPARE(parsed.object().value(QStringLiteral("schema")).toString(),
             QStringLiteral("holyscreen.endurance/1"));
}

void EnduranceRunnerTest::unusableMediaDirectoryBlocksTheSession()
{
    QTemporaryDir reportDirectory;
    QVERIFY(reportDirectory.isValid());

    ApplicationController controller;
    EnduranceOptions options;
    options.durationSeconds = 1;
    options.actionsPerMinute = 600;
    options.reportPath = reportDirectory.filePath(QStringLiteral("blocked.json"));
    options.mediaDirectory = reportDirectory.filePath(QStringLiteral("inexistente"));
    EnduranceRunner runner(controller, options);

    QSignalSpy completed(&runner, &EnduranceRunner::completed);
    runner.start();
    QVERIFY(completed.wait(15000));

    QVERIFY(!runner.passed());
    const auto verdict = runner.report().value(QStringLiteral("verdict")).toObject();
    QVERIFY(!verdict.value(QStringLiteral("passed")).toBool());
    QVERIFY(!verdict.value(QStringLiteral("blockers")).toArray().isEmpty());
    const auto failures = runner.report().value(QStringLiteral("failures")).toArray();
    QVERIFY(!failures.isEmpty());
    QCOMPARE(failures.first().toObject().value(QStringLiteral("category")).toString(),
             QStringLiteral("setup"));
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    // A sessão nunca pode escrever no cofre real da máquina que roda a suíte.
    qputenv("HOLYSCREEN_SECRET_STORE", QByteArrayLiteral("memory"));
    QGuiApplication app(argc, argv);
    QTemporaryDir dataDirectory;
    if (!dataDirectory.isValid()) return 2;
    qputenv("HOLYSCREEN_DATA_DIR", dataDirectory.path().toUtf8());
    AppLogger::install(dataDirectory.path());
    EnduranceRunnerTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "EnduranceRunnerTest.moc"
