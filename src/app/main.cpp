#include "app/ApplicationController.h"
#include "app/AppLogger.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QTimer>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("HolyScreen"));
    QCoreApplication::setApplicationName(QStringLiteral("HolyScreen"));
    QCoreApplication::setApplicationVersion(QStringLiteral(HOLYSCREEN_VERSION));

    QTemporaryDir smokeData;
    if (app.arguments().contains(QStringLiteral("--quit-after-startup"))) {
        qputenv("HOLYSCREEN_DATA_DIR", smokeData.path().toUtf8());
    }
    const auto configuredData=qEnvironmentVariable("HOLYSCREEN_DATA_DIR");
    const auto dataDirectory=configuredData.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::AppDataLocation):configuredData;
    QDir().mkpath(dataDirectory);
    churchpresenter::AppLogger::install(dataDirectory);
    qInfo()<<"HolyScreen starting"<<QCoreApplication::applicationVersion();

    churchpresenter::ApplicationController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("presentationController"), &controller);
    engine.loadFromModule("ChurchPresenter", "MainWindow");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    if (app.arguments().contains(QStringLiteral("--quit-after-startup"))) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
