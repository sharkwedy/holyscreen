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

} // namespace churchpresenter
