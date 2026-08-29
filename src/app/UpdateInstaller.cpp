#include "app/UpdateInstaller.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

#include <utility>

namespace churchpresenter {

namespace {
bool platformSupportsAutomaticInstallation()
{
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

bool launchDetached(const UpdateInstallRequest &request)
{
#if defined(Q_OS_WIN)
    const auto applicationDirectory = QCoreApplication::applicationDirPath();
    const auto bundledHelper = QDir(applicationDirectory).filePath(
        QStringLiteral("holyscreen-update-helper.exe"));
    const QFileInfo helperInformation(bundledHelper);
    if (!helperInformation.exists() || !helperInformation.isFile()
        || helperInformation.isSymLink()) {
        return false;
    }

    const auto temporaryRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (temporaryRoot.isEmpty()) return false;
    const auto helperDirectory = QDir(temporaryRoot).filePath(QStringLiteral("HolyScreen"));
    if (!QDir().mkpath(helperDirectory)) return false;
    const auto temporaryHelper = QDir(helperDirectory).filePath(
        QStringLiteral("holyscreen-update-helper.exe"));
    if (QFile::exists(temporaryHelper) && !QFile::remove(temporaryHelper)) return false;
    if (!QFile::copy(bundledHelper, temporaryHelper)) return false;

    const QStringList arguments{
        QStringLiteral("--installer"), request.installerPath,
        QStringLiteral("--sha256"), request.expectedSha256,
        QStringLiteral("--size"), QString::number(request.expectedSize),
        QStringLiteral("--application"), QCoreApplication::applicationFilePath(),
        QStringLiteral("--wait-pid"), QString::number(QCoreApplication::applicationPid()),
    };
    return QProcess::startDetached(temporaryHelper, arguments, applicationDirectory);
#else
    Q_UNUSED(request);
    return false;
#endif
}

void assignError(QString *destination, const QString &message)
{
    if (destination) *destination = message;
}
} // namespace

UpdateInstaller::UpdateInstaller(QObject *parent)
    : UpdateInstaller(&launchDetached, platformSupportsAutomaticInstallation(), parent)
{
}

UpdateInstaller::UpdateInstaller(Launcher launcher, bool supported, QObject *parent)
    : QObject(parent)
    , m_launcher(std::move(launcher))
    , m_supported(supported)
{
}

bool UpdateInstaller::canInstall(const QString &path) const
{
    if (!m_supported || path.isEmpty()) return false;
    const QFileInfo file(path);
    return file.isAbsolute() && file.exists() && file.isFile() && !file.isSymLink()
        && file.fileName().startsWith(QStringLiteral("HolyScreen-"), Qt::CaseInsensitive)
        && file.suffix().compare(QStringLiteral("exe"), Qt::CaseInsensitive) == 0;
}

bool UpdateInstaller::install(const QString &path, const QString &expectedSha256,
                              qint64 expectedSize, QString *error) const
{
    if (!canInstall(path)) {
        assignError(error, tr("Este pacote não permite instalação automática neste sistema."));
        return false;
    }

    static const QRegularExpression digestExpression(
        QStringLiteral(R"(^[0-9a-fA-F]{64}$)"));
    if (expectedSize <= 0 || !digestExpression.match(expectedSha256).hasMatch()) {
        assignError(error, tr("A atualização não possui tamanho e SHA-256 válidos."));
        return false;
    }

    const QFileInfo information(path);
    if (information.size() != expectedSize) {
        assignError(error, tr("O tamanho do instalador mudou depois do download."));
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        assignError(error, tr("Não foi possível abrir o instalador para verificá-lo novamente."));
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        assignError(error, tr("Não foi possível calcular novamente o SHA-256 do instalador."));
        return false;
    }
    const auto digest = QString::fromLatin1(hash.result().toHex());
    if (digest.compare(expectedSha256, Qt::CaseInsensitive) != 0) {
        assignError(error, tr("O SHA-256 do instalador mudou depois do download."));
        return false;
    }

    if (!m_launcher || !m_launcher({
            information.absoluteFilePath(), expectedSha256.toLower(), expectedSize})) {
        assignError(error, tr("O Windows não permitiu iniciar o instalador."));
        return false;
    }
    if (error) error->clear();
    return true;
}

} // namespace churchpresenter
