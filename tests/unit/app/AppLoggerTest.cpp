#include <QDebug>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "app/AppLogger.h"

using namespace churchpresenter;

class AppLoggerTest final : public QObject {
    Q_OBJECT

private slots:
    void writesDebugMessagesOnlyWhenTheOptionIsEnabled();
};

void AppLoggerTest::writesDebugMessagesOnlyWhenTheOptionIsEnabled()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    AppLogger::install(directory.path());

    AppLogger::setDebugMessagesEnabled(false);
    qDebug() << "hidden-debug-message";
    qInfo() << "visible-info-message";

    QFile log(AppLogger::logPath());
    QVERIFY(log.open(QIODevice::ReadOnly | QIODevice::Text));
    auto contents = log.readAll();
    QVERIFY(!contents.contains("hidden-debug-message"));
    QVERIFY(contents.contains("visible-info-message"));
    log.close();

    AppLogger::setDebugMessagesEnabled(true);
    qDebug() << "visible-debug-message";

    QVERIFY(log.open(QIODevice::ReadOnly | QIODevice::Text));
    contents = log.readAll();
    QVERIFY(contents.contains("visible-debug-message"));
}

QTEST_APPLESS_MAIN(AppLoggerTest)
#include "AppLoggerTest.moc"
