#include "app/ApplicationController.h"
#include "app/AppLogger.h"
#include "app/EnduranceRunner.h"

#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QCoreApplication>
#include <QSettings>
#include <QTranslator>
#include <QTimer>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QDir>

namespace {

//! Lê `--opção=valor` ou `--opção valor`. Devolve string vazia quando a opção
//! não foi informada, para que o chamador aplique o próprio padrão.
QString optionValue(const QStringList &arguments, const QString &name)
{
    const auto prefix = name + QStringLiteral("=");
    for (int index = 0; index < arguments.size(); ++index) {
        const auto &argument = arguments.at(index);
        if (argument.startsWith(prefix)) return argument.mid(prefix.size());
        if (argument == name && index + 1 < arguments.size()) return arguments.at(index + 1);
    }
    return {};
}

} // namespace

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("Fusion"));

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

    const auto arguments = app.arguments();
    const bool enduranceRequested = arguments.contains(QStringLiteral("--endurance"));

    QTemporaryDir isolatedData;
    // O smoke de inicialização e o endurance nunca podem tocar na biblioteca do
    // operador: sem um diretório explícito eles trabalham em dados temporários.
    if (arguments.contains(QStringLiteral("--quit-after-startup")) || enduranceRequested) {
        if (qEnvironmentVariableIsEmpty("HOLYSCREEN_DATA_DIR"))
            qputenv("HOLYSCREEN_DATA_DIR", isolatedData.path().toUtf8());
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
    if (arguments.contains(QStringLiteral("--quit-after-startup"))) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    }

    std::unique_ptr<churchpresenter::EnduranceRunner> endurance;
    if (enduranceRequested) {
        const auto minutes =
            optionValue(arguments, QStringLiteral("--endurance-minutes")).toDouble();
        const auto reportPath = optionValue(arguments, QStringLiteral("--endurance-report"));
        churchpresenter::EnduranceOptions options;
        options.durationSeconds = minutes > 0.0
            ? static_cast<int>(minutes * 60.0) : options.durationSeconds;
        options.reportPath = reportPath.isEmpty()
            ? dataDirectory + QStringLiteral("/endurance-report.json") : reportPath;
        options.mediaDirectory = optionValue(arguments, QStringLiteral("--endurance-media"));
        endurance = std::make_unique<churchpresenter::EnduranceRunner>(controller, options);
        QObject::connect(endurance.get(), &churchpresenter::EnduranceRunner::completed,
                         &app, [&app, runner = endurance.get(), path = options.reportPath] {
            QString error;
            if (!runner->writeReport(&error))
                qWarning() << "Could not write the endurance report:" << error;
            else
                qInfo() << "Endurance report written to" << path;
            app.exit(runner->passed() ? 0 : 1);
        });
        QTimer::singleShot(0, endurance.get(), [runner = endurance.get()] { runner->start(); });
    }

    return app.exec();
}
