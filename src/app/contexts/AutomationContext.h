#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

namespace churchpresenter {

class ApplicationController;

// QML-facing automation facade. ApplicationController keeps compatibility
// aliases while consumers migrate to this bounded context.
class AutomationContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList automations READ automations NOTIFY automationsChanged)
    Q_PROPERTY(QVariantList automationRuns READ automationRuns NOTIFY automationRunsChanged)
    Q_PROPERTY(QString automationStatus READ automationStatus NOTIFY automationStatusChanged)
    Q_PROPERTY(bool automationsEnabled READ automationsEnabled WRITE setAutomationsEnabled NOTIFY automationsChanged)
    Q_PROPERTY(bool processActionsEnabled READ processActionsEnabled WRITE setProcessActionsEnabled NOTIFY authorizedExecutablesChanged)
    Q_PROPERTY(QVariantList authorizedExecutables READ authorizedExecutables NOTIFY authorizedExecutablesChanged)
    Q_PROPERTY(QStringList automationTriggerTypes READ automationTriggerTypes CONSTANT)
    Q_PROPERTY(QStringList automationActionTypes READ automationActionTypes CONSTANT)
    Q_PROPERTY(QStringList automationConditionOperations READ automationConditionOperations CONSTANT)

public:
    explicit AutomationContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QVariantList automations() const;
    [[nodiscard]] QVariantList automationRuns() const;
    [[nodiscard]] QString automationStatus() const;
    [[nodiscard]] bool automationsEnabled() const;
    void setAutomationsEnabled(bool enabled);
    [[nodiscard]] bool processActionsEnabled() const;
    void setProcessActionsEnabled(bool enabled);
    [[nodiscard]] QVariantList authorizedExecutables() const;
    [[nodiscard]] QStringList automationTriggerTypes() const;
    [[nodiscard]] QStringList automationActionTypes() const;
    [[nodiscard]] QStringList automationConditionOperations() const;

    Q_INVOKABLE QVariantMap saveAutomation(const QVariantMap &automation);
    Q_INVOKABLE bool removeAutomation(const QString &automationId);
    Q_INVOKABLE bool setAutomationEnabled(const QString &automationId, bool enabled);
    Q_INVOKABLE bool resumeAutomation(const QString &automationId);
    Q_INVOKABLE QVariantMap dryRunAutomation(const QString &automationId,
                                             const QVariantMap &payload = {});
    Q_INVOKABLE QVariantMap exportAutomations(const QUrl &destination);
    Q_INVOKABLE QVariantMap importAutomations(const QUrl &source);
    Q_INVOKABLE QVariantMap authorizeExecutable(const QString &path, const QString &label);
    Q_INVOKABLE bool revokeExecutable(const QString &canonicalPath);

signals:
    void automationsChanged();
    void automationRunsChanged();
    void automationStatusChanged();
    void authorizedExecutablesChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
