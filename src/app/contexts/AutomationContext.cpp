#include "app/contexts/AutomationContext.h"

#include "app/ApplicationController.h"

namespace churchpresenter {

AutomationContext::AutomationContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    connect(&controller, &ApplicationController::automationsChanged,
            this, &AutomationContext::automationsChanged);
    connect(&controller, &ApplicationController::automationRunsChanged,
            this, &AutomationContext::automationRunsChanged);
    connect(&controller, &ApplicationController::automationStatusChanged,
            this, &AutomationContext::automationStatusChanged);
    connect(&controller, &ApplicationController::authorizedExecutablesChanged,
            this, &AutomationContext::authorizedExecutablesChanged);
}

QVariantList AutomationContext::automations() const { return m_controller.automations(); }
QVariantList AutomationContext::automationRuns() const { return m_controller.automationRuns(); }
QString AutomationContext::automationStatus() const { return m_controller.automationStatus(); }
bool AutomationContext::automationsEnabled() const { return m_controller.automationsEnabled(); }
void AutomationContext::setAutomationsEnabled(bool enabled) { m_controller.setAutomationsEnabled(enabled); }
bool AutomationContext::processActionsEnabled() const { return m_controller.processActionsEnabled(); }
void AutomationContext::setProcessActionsEnabled(bool enabled) { m_controller.setProcessActionsEnabled(enabled); }
QVariantList AutomationContext::authorizedExecutables() const { return m_controller.authorizedExecutables(); }
QStringList AutomationContext::automationTriggerTypes() const { return m_controller.automationTriggerTypeList(); }
QStringList AutomationContext::automationActionTypes() const { return m_controller.automationActionTypeList(); }
QStringList AutomationContext::automationConditionOperations() const { return m_controller.automationConditionOperationList(); }
QVariantMap AutomationContext::saveAutomation(const QVariantMap &value) { return m_controller.saveAutomation(value); }
bool AutomationContext::removeAutomation(const QString &id) { return m_controller.removeAutomation(id); }
bool AutomationContext::setAutomationEnabled(const QString &id, bool enabled) { return m_controller.setAutomationEnabled(id, enabled); }
bool AutomationContext::resumeAutomation(const QString &id) { return m_controller.resumeAutomation(id); }
QVariantMap AutomationContext::dryRunAutomation(const QString &id, const QVariantMap &payload) { return m_controller.dryRunAutomation(id, payload); }
QVariantMap AutomationContext::exportAutomations(const QUrl &destination) { return m_controller.exportAutomations(destination); }
QVariantMap AutomationContext::importAutomations(const QUrl &source) { return m_controller.importAutomations(source); }
QVariantMap AutomationContext::authorizeExecutable(const QString &path, const QString &label) { return m_controller.authorizeExecutable(path, label); }
bool AutomationContext::revokeExecutable(const QString &path) { return m_controller.revokeExecutable(path); }

} // namespace churchpresenter
