#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>

namespace churchpresenter {

class ApplicationController;

class MaintenanceContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastBackupPath READ lastBackupPath NOTIFY maintenanceChanged)
    Q_PROPERTY(bool recoveredFromCrash READ recoveredFromCrash NOTIFY maintenanceChanged)
    Q_PROPERTY(QVariantMap diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QString updateStatus READ updateStatus NOTIFY updateChanged)
    Q_PROPERTY(QString updateEndpoint READ updateEndpoint WRITE setUpdateEndpoint NOTIFY updateChanged)
    Q_PROPERTY(bool automaticUpdateChecks READ automaticUpdateChecks WRITE setAutomaticUpdateChecks NOTIFY updateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateChanged)
    Q_PROPERTY(QString updateLatestVersion READ updateLatestVersion NOTIFY updateChanged)
    Q_PROPERTY(QString updateReleaseUrl READ updateReleaseUrl NOTIFY updateChanged)
    Q_PROPERTY(QString updateAssetName READ updateAssetName NOTIFY updateChanged)
    Q_PROPERTY(bool updateDownloadable READ updateDownloadable NOTIFY updateChanged)
    Q_PROPERTY(bool updateDownloading READ updateDownloading NOTIFY updateDownloadChanged)
    Q_PROPERTY(double updateDownloadProgress READ updateDownloadProgress NOTIFY updateDownloadChanged)
    Q_PROPERTY(QString updateDownloadedPath READ updateDownloadedPath NOTIFY updateDownloadChanged)
    Q_PROPERTY(bool autosavePending READ autosavePending NOTIFY autosaveChanged)
    Q_PROPERTY(QString autosaveStatus READ autosaveStatus NOTIFY autosaveChanged)
    Q_PROPERTY(bool debugEnabled READ debugEnabled WRITE setDebugEnabled NOTIFY debugOptionsChanged)
    Q_PROPERTY(bool debugDiagnostics READ debugDiagnostics WRITE setDebugDiagnostics NOTIFY debugOptionsChanged)

public:
    explicit MaintenanceContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QString lastBackupPath() const;
    [[nodiscard]] bool recoveredFromCrash() const;
    [[nodiscard]] QVariantMap diagnostics() const;
    [[nodiscard]] QString updateStatus() const;
    [[nodiscard]] QString updateEndpoint() const;
    void setUpdateEndpoint(const QString &endpoint);
    [[nodiscard]] bool automaticUpdateChecks() const;
    void setAutomaticUpdateChecks(bool enabled);
    [[nodiscard]] bool updateAvailable() const;
    [[nodiscard]] QString updateLatestVersion() const;
    [[nodiscard]] QString updateReleaseUrl() const;
    [[nodiscard]] QString updateAssetName() const;
    [[nodiscard]] bool updateDownloadable() const;
    [[nodiscard]] bool updateDownloading() const;
    [[nodiscard]] double updateDownloadProgress() const;
    [[nodiscard]] QString updateDownloadedPath() const;
    [[nodiscard]] bool autosavePending() const;
    [[nodiscard]] QString autosaveStatus() const;
    [[nodiscard]] bool debugEnabled() const;
    void setDebugEnabled(bool enabled);
    [[nodiscard]] bool debugDiagnostics() const;
    void setDebugDiagnostics(bool enabled);

    Q_INVOKABLE QString createBackup();
    Q_INVOKABLE bool scheduleRestore(const QUrl &source);
    Q_INVOKABLE bool exportDiagnostics(const QUrl &destination);
    Q_INVOKABLE void runBenchmark();
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadUpdate();
    Q_INVOKABLE void cancelUpdateDownload();
    Q_INVOKABLE bool revealUpdateDownload();

signals:
    void maintenanceChanged();
    void diagnosticsChanged();
    void updateChanged();
    void updateDownloadChanged();
    void autosaveChanged();
    void debugOptionsChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
