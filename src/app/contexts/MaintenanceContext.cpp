#include "app/contexts/MaintenanceContext.h"

#include "app/ApplicationController.h"

namespace churchpresenter {

MaintenanceContext::MaintenanceContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    connect(&controller, &ApplicationController::maintenanceChanged,
            this, &MaintenanceContext::maintenanceChanged);
    connect(&controller, &ApplicationController::diagnosticsChanged,
            this, &MaintenanceContext::diagnosticsChanged);
    connect(&controller, &ApplicationController::updateChanged,
            this, &MaintenanceContext::updateChanged);
    connect(&controller, &ApplicationController::autosaveChanged,
            this, &MaintenanceContext::autosaveChanged);
    connect(&controller, &ApplicationController::debugOptionsChanged,
            this, &MaintenanceContext::debugOptionsChanged);
    connect(&controller, &ApplicationController::updateDownloadChanged,
            this, &MaintenanceContext::updateDownloadChanged);
}

QString MaintenanceContext::lastBackupPath() const { return m_controller.lastBackupPath(); }
bool MaintenanceContext::recoveredFromCrash() const { return m_controller.recoveredFromCrash(); }
QVariantMap MaintenanceContext::diagnostics() const { return m_controller.diagnostics(); }
QString MaintenanceContext::updateStatus() const { return m_controller.updateStatus(); }
QString MaintenanceContext::updateEndpoint() const { return m_controller.updateEndpoint(); }
void MaintenanceContext::setUpdateEndpoint(const QString &value) { m_controller.setUpdateEndpoint(value); }
bool MaintenanceContext::autosavePending() const { return m_controller.autosavePending(); }
QString MaintenanceContext::autosaveStatus() const { return m_controller.autosaveStatus(); }
bool MaintenanceContext::debugEnabled() const { return m_controller.debugEnabled(); }
void MaintenanceContext::setDebugEnabled(bool enabled) { m_controller.setDebugEnabled(enabled); }
bool MaintenanceContext::debugDiagnostics() const { return m_controller.debugDiagnostics(); }
void MaintenanceContext::setDebugDiagnostics(bool enabled) { m_controller.setDebugDiagnostics(enabled); }
QString MaintenanceContext::createBackup() { return m_controller.createBackup(); }
bool MaintenanceContext::scheduleRestore(const QUrl &source) { return m_controller.scheduleRestore(source); }
bool MaintenanceContext::exportDiagnostics(const QUrl &destination) { return m_controller.exportDiagnostics(destination); }
void MaintenanceContext::runBenchmark() { m_controller.runBenchmark(); }
void MaintenanceContext::checkForUpdates() { m_controller.checkForUpdates(); }
bool MaintenanceContext::automaticUpdateChecks() const { return m_controller.automaticUpdateChecks(); }
void MaintenanceContext::setAutomaticUpdateChecks(bool enabled) { m_controller.setAutomaticUpdateChecks(enabled); }
bool MaintenanceContext::updateAvailable() const { return m_controller.updateAvailable(); }
QString MaintenanceContext::updateLatestVersion() const { return m_controller.updateLatestVersion(); }
QString MaintenanceContext::updateReleaseUrl() const { return m_controller.updateReleaseUrl(); }
QString MaintenanceContext::updateAssetName() const { return m_controller.updateAssetName(); }
bool MaintenanceContext::updateDownloadable() const { return m_controller.updateDownloadable(); }
bool MaintenanceContext::updateDownloading() const { return m_controller.updateDownloading(); }
double MaintenanceContext::updateDownloadProgress() const { return m_controller.updateDownloadProgress(); }
QString MaintenanceContext::updateDownloadedPath() const { return m_controller.updateDownloadedPath(); }
bool MaintenanceContext::updateInstallable() const { return m_controller.updateInstallable(); }
void MaintenanceContext::downloadUpdate() { m_controller.downloadUpdate(); }
void MaintenanceContext::cancelUpdateDownload() { m_controller.cancelUpdateDownload(); }
bool MaintenanceContext::installDownloadedUpdate() { return m_controller.installDownloadedUpdate(); }
bool MaintenanceContext::revealUpdateDownload() { return m_controller.revealUpdateDownload(); }

} // namespace churchpresenter
