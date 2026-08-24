#include "app/AppLogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QTextStream>

#include <atomic>

namespace {

QString logFilePath;
QMutex logMutex;
QtMessageHandler previousHandler = nullptr;
std::atomic_bool debugMessagesEnabled = false;
std::atomic<quint64> warningCount = 0;
std::atomic<quint64> criticalCount = 0;
std::atomic<quint64> fatalCount = 0;

void writeMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    switch (type) {
    case QtWarningMsg: warningCount.fetch_add(1, std::memory_order_relaxed); break;
    case QtCriticalMsg: criticalCount.fetch_add(1, std::memory_order_relaxed); break;
    case QtFatalMsg: fatalCount.fetch_add(1, std::memory_order_relaxed); break;
    default: break;
    }
    {
        QMutexLocker lock(&logMutex);
        if (!logFilePath.isEmpty()
            && (type != QtDebugMsg || debugMessagesEnabled.load(std::memory_order_relaxed))) {
            if (QFileInfo(logFilePath).size() > 2 * 1024 * 1024) {
                QFile::remove(logFilePath + QStringLiteral(".1"));
                QFile::rename(logFilePath, logFilePath + QStringLiteral(".1"));
            }
            QFile file(logFilePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                const char *level = type == QtDebugMsg ? "DEBUG"
                    : type == QtInfoMsg ? "INFO"
                    : type == QtWarningMsg ? "WARN"
                    : type == QtCriticalMsg ? "ERROR" : "FATAL";
                QTextStream(&file) << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                                   << ' ' << level << ' ' << message << '\n';
            }
        }
    }
    if (previousHandler) previousHandler(type, context, message);
}

} // namespace

namespace churchpresenter {

void AppLogger::install(const QString &directory)
{
    QDir().mkpath(directory + QStringLiteral("/logs"));
    logFilePath = directory + QStringLiteral("/logs/holyscreen.log");
    previousHandler = qInstallMessageHandler(writeMessage);
}

QString AppLogger::logPath() { return logFilePath; }

void AppLogger::setDebugMessagesEnabled(bool enabled)
{
    ::debugMessagesEnabled.store(enabled, std::memory_order_relaxed);
}

bool AppLogger::debugMessagesEnabled()
{
    return ::debugMessagesEnabled.load(std::memory_order_relaxed);
}

quint64 AppLogger::warningCount() { return ::warningCount.load(std::memory_order_relaxed); }

quint64 AppLogger::criticalCount() { return ::criticalCount.load(std::memory_order_relaxed); }

quint64 AppLogger::fatalCount() { return ::fatalCount.load(std::memory_order_relaxed); }

void AppLogger::resetCounters()
{
    ::warningCount.store(0, std::memory_order_relaxed);
    ::criticalCount.store(0, std::memory_order_relaxed);
    ::fatalCount.store(0, std::memory_order_relaxed);
}

} // namespace churchpresenter
