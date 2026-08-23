#include "automation/AuthorizedExecutables.h"
#include "automation/QtProcessRunner.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

using namespace churchpresenter;

namespace {

QString helperPath()
{
    return QFileInfo(QString::fromUtf8(TEST_PROCESS_HELPER_PATH)).canonicalFilePath();
}

QString copyHelper(const QTemporaryDir &directory, const QString &name)
{
    const auto path = directory.filePath(name);
    if (!QFile::copy(helperPath(), path)) return {};
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return QFileInfo(path).canonicalFilePath();
}

} // namespace

class AuthorizedExecutablesTest final : public QObject {
    Q_OBJECT

private slots:
    void refusesEverythingWhileTheFeatureIsOff();
    void authorizesOnlyExistingExecutableFiles();
    void resolvesSymlinksToTheRealTarget();
    void rejectsRelativePathsStringArgumentsAndBadDirectories();
    void normalizesTheRequestWithLimits();
    void runsAnAuthorizedScriptAndCapturesItsOutput();
    void capsProcessOutput();
    void killsAProcessThatExceedsTheTimeout();
    void cancelAllStopsRunningProcesses();
};

void AuthorizedExecutablesTest::refusesEverythingWhileTheFeatureIsOff()
{
    const auto script = helperPath();
    QVERIFY(!script.isEmpty());
    AuthorizedExecutables allowlist;
    QVERIFY(!allowlist.isEnabled());
    QVERIFY(allowlist.authorize(script, QStringLiteral("Script")));

    QString code;
    QString message;
    QCOMPARE(allowlist.validate({{QStringLiteral("executable"), script}}, &code, &message),
             std::nullopt);
    QCOMPARE(code, QStringLiteral("process_disabled"));
}

void AuthorizedExecutablesTest::authorizesOnlyExistingExecutableFiles()
{
    QTemporaryDir directory;
    AuthorizedExecutables allowlist;
    QString error;

    QVERIFY(!allowlist.authorize(directory.filePath(QStringLiteral("ausente.sh")),
                                 QStringLiteral("Ausente"), &error));
    QVERIFY(error.contains(QStringLiteral("não existe")));

    QVERIFY(!allowlist.authorize(directory.path(), QStringLiteral("Pasta"), &error));
    QVERIFY(error.contains(QStringLiteral("não é um arquivo")));

    const auto plain = directory.filePath(QStringLiteral("dados.txt"));
    QFile file(plain);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("x");
    file.close();
    QFile::setPermissions(plain, QFile::ReadOwner | QFile::WriteOwner);
    QVERIFY(!allowlist.authorize(plain, QStringLiteral("Texto"), &error));
    QVERIFY(error.contains(QStringLiteral("executável")));

    const auto script = helperPath();
    QVERIFY(!script.isEmpty());
    QVERIFY(allowlist.authorize(script, QString{}, &error));
    QCOMPARE(allowlist.entries().size(), 1);
    QCOMPARE(allowlist.entries().first().label, QFileInfo(script).fileName());
    // Autorizar duas vezes não duplica.
    QVERIFY(allowlist.authorize(script, QStringLiteral("Outra"), &error));
    QCOMPARE(allowlist.entries().size(), 1);
    QVERIFY(allowlist.revoke(allowlist.entries().first().canonicalPath));
    QVERIFY(allowlist.entries().isEmpty());
}

void AuthorizedExecutablesTest::resolvesSymlinksToTheRealTarget()
{
#ifdef Q_OS_WIN
    QSKIP("Windows runners do not permit unprivileged filesystem symlinks.");
#else
    QTemporaryDir directory;
    const auto real = copyHelper(directory, QStringLiteral("real-helper"));
    const auto other = copyHelper(directory, QStringLiteral("other-helper"));
    const auto link = directory.filePath(QStringLiteral("atalho.sh"));
    QVERIFY(QFile::link(real, link));

    AuthorizedExecutables allowlist;
    allowlist.setEnabled(true);
    QVERIFY(allowlist.authorize(link, QStringLiteral("Atalho")));
    // A autorização vale pelo destino real, não pelo nome do link.
    QCOMPARE(allowlist.entries().first().canonicalPath, real);
    QVERIFY(allowlist.isAuthorized(real));
    QVERIFY(allowlist.isAuthorized(link));
    QVERIFY(!allowlist.isAuthorized(other));

    // Trocar o alvo do symlink para um executável não autorizado não passa.
    QVERIFY(QFile::remove(link));
    QVERIFY(QFile::link(other, link));
    QString code;
    QString message;
    QCOMPARE(allowlist.validate({{QStringLiteral("executable"), link}}, &code, &message),
             std::nullopt);
    QCOMPARE(code, QStringLiteral("process_not_authorized"));
#endif
}

void AuthorizedExecutablesTest::rejectsRelativePathsStringArgumentsAndBadDirectories()
{
    QTemporaryDir directory;
    const auto script = helperPath();
    QVERIFY(!script.isEmpty());
    AuthorizedExecutables allowlist;
    allowlist.setEnabled(true);
    QVERIFY(allowlist.authorize(script, QStringLiteral("Script")));

    QString code;
    QString message;
    QCOMPARE(allowlist.validate({{QStringLiteral("executable"), QStringLiteral("ok.sh")}},
                                &code, &message),
             std::nullopt);
    QCOMPARE(code, QStringLiteral("invalid_payload"));

    // Argumento como string única abriria espaço para injeção de shell.
    QCOMPARE(allowlist.validate({{QStringLiteral("executable"), script},
                                 {QStringLiteral("arguments"),
                                  QStringLiteral("--flag; rm -rf /")}}, &code, &message),
             std::nullopt);
    QCOMPARE(code, QStringLiteral("invalid_payload"));

    QCOMPARE(allowlist.validate({{QStringLiteral("executable"), script},
                                 {QStringLiteral("workingDirectory"),
                                  QStringLiteral("relativo")}}, &code, &message),
             std::nullopt);
    QCOMPARE(code, QStringLiteral("invalid_payload"));

    QVariantList many;
    for (int index = 0; index < AuthorizedExecutables::MaximumArguments + 1; ++index) {
        many.append(QStringLiteral("-a"));
    }
    QCOMPARE(allowlist.validate({{QStringLiteral("executable"), script},
                                 {QStringLiteral("arguments"), many}}, &code, &message),
             std::nullopt);
    QCOMPARE(code, QStringLiteral("invalid_payload"));
}

void AuthorizedExecutablesTest::normalizesTheRequestWithLimits()
{
    QTemporaryDir directory;
    const auto script = helperPath();
    QVERIFY(!script.isEmpty());
    AuthorizedExecutables allowlist;
    allowlist.setEnabled(true);
    QVERIFY(allowlist.authorize(script, QStringLiteral("Script")));

    const auto request = allowlist.validate({
        {QStringLiteral("executable"), script},
        {QStringLiteral("arguments"), QVariantList{QStringLiteral("--slide"), 3}},
        {QStringLiteral("timeoutMs"), 900000},
        {QStringLiteral("environment"),
         QVariantMap{{QStringLiteral("HOLYSCREEN_SLIDE"), 3}, {QString{}, QStringLiteral("x")}}},
    }, nullptr, nullptr);

    QVERIFY(request.has_value());
    QCOMPARE(request->executable, script);
    QCOMPARE(request->arguments, QStringList({QStringLiteral("--slide"), QStringLiteral("3")}));
    QCOMPARE(request->timeoutMs, AuthorizedExecutables::MaximumTimeoutMs);
    QCOMPARE(request->maximumOutputBytes, AuthorizedExecutables::MaximumOutputBytes);
    QCOMPARE(request->environment.size(), 1);
    QCOMPARE(request->workingDirectory, QFileInfo(script).absolutePath());
}

void AuthorizedExecutablesTest::runsAnAuthorizedScriptAndCapturesItsOutput()
{
    QTemporaryDir directory;
    const auto script = helperPath();
    QVERIFY(!script.isEmpty());
    AuthorizedExecutables allowlist;
    allowlist.setEnabled(true);
    QVERIFY(allowlist.authorize(script, QStringLiteral("Saída")));

    const auto request = allowlist.validate({
        {QStringLiteral("executable"), script},
        {QStringLiteral("arguments"),
         QVariantList{QStringLiteral("--echo"), QStringLiteral("7")}},
    }, nullptr, nullptr);
    QVERIFY(request.has_value());

    QtProcessRunner runner;
    ProcessResult result;
    bool finished = false;
    runner.run(*request, [&result, &finished](const ProcessResult &value) {
        result = value;
        finished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(finished, 5000);
    QVERIFY(result.started);
    QVERIFY(result.finished);
    QCOMPARE(result.exitCode, 0);
    QVERIFY(result.standardOutput.contains(QByteArrayLiteral("slide 7")));
    QVERIFY(result.standardError.contains(QByteArrayLiteral("erro")));
}

void AuthorizedExecutablesTest::killsAProcessThatExceedsTheTimeout()
{
    QTemporaryDir directory;
    const auto script = helperPath();
    QVERIFY(!script.isEmpty());
    AuthorizedExecutables allowlist;
    allowlist.setEnabled(true);
    QVERIFY(allowlist.authorize(script, QStringLiteral("Lento")));

    auto request = allowlist.validate(
        {{QStringLiteral("executable"), script},
         {QStringLiteral("arguments"),
          QVariantList{QStringLiteral("--sleep"), QStringLiteral("30000")}},
         {QStringLiteral("timeoutMs"), 300}}, nullptr, nullptr);
    QVERIFY(request.has_value());

    QtProcessRunner runner;
    ProcessResult result;
    bool finished = false;
    runner.run(*request, [&result, &finished](const ProcessResult &value) {
        result = value;
        finished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(finished, 8000);
    QVERIFY(!result.finished);
    QCOMPARE(result.errorCode, QStringLiteral("timeout"));
}

void AuthorizedExecutablesTest::capsProcessOutput()
{
    AuthorizedExecutables allowlist;
    allowlist.setEnabled(true);
    const auto executable = helperPath();
    QVERIFY(allowlist.authorize(executable, QStringLiteral("Saída grande")));
    const auto request = allowlist.validate({
        {QStringLiteral("executable"), executable},
        {QStringLiteral("arguments"),
         QVariantList{QStringLiteral("--large-output"), QStringLiteral("70000")}},
    }, nullptr, nullptr);
    QVERIFY(request.has_value());

    QtProcessRunner runner;
    ProcessResult result;
    bool finished = false;
    runner.run(*request, [&result, &finished](const ProcessResult &value) {
        result = value;
        finished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(finished, 5000);
    QCOMPARE(result.standardOutput.size(), AuthorizedExecutables::MaximumOutputBytes);
    QCOMPARE(result.standardError.size(), AuthorizedExecutables::MaximumOutputBytes);
}

void AuthorizedExecutablesTest::cancelAllStopsRunningProcesses()
{
    AuthorizedExecutables allowlist;
    allowlist.setEnabled(true);
    const auto executable = helperPath();
    QVERIFY(allowlist.authorize(executable, QStringLiteral("Cancelável")));
    const auto request = allowlist.validate({
        {QStringLiteral("executable"), executable},
        {QStringLiteral("arguments"),
         QVariantList{QStringLiteral("--sleep"), QStringLiteral("30000")}},
    }, nullptr, nullptr);
    QVERIFY(request.has_value());

    QtProcessRunner runner;
    ProcessResult result;
    bool finished = false;
    runner.run(*request, [&result, &finished](const ProcessResult &value) {
        result = value;
        finished = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(!runner.findChildren<QProcess *>().isEmpty(), 1000);
    runner.cancelAll();
    QTRY_VERIFY_WITH_TIMEOUT(finished, 5000);
    QVERIFY(!result.finished);
    QCOMPARE(result.errorCode, QStringLiteral("timeout"));
}

QTEST_MAIN(AuthorizedExecutablesTest)
#include "AuthorizedExecutablesTest.moc"
