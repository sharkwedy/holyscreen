#include "app/ApplicationController.h"
#include "app/AppLogger.h"

#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QSettings>
#include <QTranslator>
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

    const QSettings operatorSettings;
    const auto configuredLocale = operatorSettings.value(
        QStringLiteral("operator/locale"), QStringLiteral("pt-BR")).toString();
    const auto localeName = configuredLocale == QStringLiteral("en-US")
        ? QStringLiteral("en_US") : QStringLiteral("pt_BR");
    QLocale::setDefault(QLocale(localeName));
    QTranslator translator;
    if (translator.load(QStringLiteral(":/i18n/holyscreen_%1.qm").arg(localeName)))
        app.installTranslator(&translator);

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
