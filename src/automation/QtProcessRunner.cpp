#include "automation/QtProcessRunner.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>

namespace churchpresenter {

QtProcessRunner::QtProcessRunner(QObject *parent)
    : QObject(parent)
{
}

QtProcessRunner::~QtProcessRunner()
{
    cancelAll();
}

void QtProcessRunner::run(const ProcessRequest &request, Completion completion)
{
    auto *process = new QProcess(this);
    process->setProgram(request.executable);
    process->setArguments(request.arguments);
    if (!request.workingDirectory.isEmpty()) {
        process->setWorkingDirectory(request.workingDirectory);
    }

    // Ambiente mínimo: apenas o que a ação declarou, mais o PATH do sistema
    // quando ele for pedido explicitamente.
    QProcessEnvironment environment;
    for (auto it = request.environment.cbegin(); it != request.environment.cend(); ++it) {
        environment.insert(it.key(), it.value().toString());
    }
    process->setProcessEnvironment(environment);

    auto *timeout = new QTimer(this);
    timeout->setSingleShot(true);
    timeout->setInterval(request.timeoutMs);

    const auto maximumBytes = request.maximumOutputBytes;
    connect(timeout, &QTimer::timeout, process, [process] {
        process->kill();
    });
    connect(process, &QProcess::finished, this,
            [this, process, timeout, completion, maximumBytes](int exitCode,
                                                               QProcess::ExitStatus status) {
        timeout->stop();
        timeout->deleteLater();
        m_running.removeAll(process);
        ProcessResult result;
        result.started = true;
        result.finished = status == QProcess::NormalExit;
        result.exitCode = exitCode;
        result.standardOutput = process->readAllStandardOutput().left(maximumBytes);
        result.standardError = process->readAllStandardError().left(maximumBytes);
        if (!result.finished) {
            result.errorCode = QStringLiteral("timeout");
            result.message = QStringLiteral("O processo foi encerrado por tempo ou sinal.");
        } else if (exitCode != 0) {
            result.errorCode = QStringLiteral("process_failed");
            result.message = QStringLiteral("O processo terminou com código %1.").arg(exitCode);
        }
        process->deleteLater();
        if (completion) completion(result);
    });
    connect(process, &QProcess::errorOccurred, this,
            [this, process, timeout, completion](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        timeout->stop();
        timeout->deleteLater();
        m_running.removeAll(process);
        const auto message = process->errorString();
        process->deleteLater();
        if (completion) {
            completion(ProcessResult{.started = false,
                                     .errorCode = QStringLiteral("process_failed"),
                                     .message = message});
        }
    });

    m_running.append(process);
    timeout->start();
    process->start();
}

void QtProcessRunner::cancelAll()
{
    const auto running = m_running;
    m_running.clear();
    for (const auto &process : running) {
        if (process) process->kill();
    }
}

} // namespace churchpresenter
