#include "app/contexts/IntegrationContext.h"

#include "app/ApplicationController.h"

namespace churchpresenter {

IntegrationContext::IntegrationContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    connect(&controller, &ApplicationController::integrationsChanged,
            this, &IntegrationContext::integrationsChanged);
    connect(&controller, &ApplicationController::integrationHistoryChanged,
            this, &IntegrationContext::integrationHistoryChanged);
    connect(&controller, &ApplicationController::integrationStatusChanged,
            this, &IntegrationContext::integrationStatusChanged);
}

QVariantList IntegrationContext::integrations() const { return m_controller.integrations(); }
QVariantList IntegrationContext::integrationHistory() const { return m_controller.integrationHistory(); }
QString IntegrationContext::integrationStatus() const { return m_controller.integrationStatus(); }
QStringList IntegrationContext::integrationTypes() const { return m_controller.integrationTypes(); }
QString IntegrationContext::integrationSecretBackend() const { return m_controller.integrationSecretBackend(); }
bool IntegrationContext::integrationSecretsPersistent() const { return m_controller.integrationSecretsPersistent(); }
QVariantMap IntegrationContext::saveIntegration(const QVariantMap &value) { return m_controller.saveIntegration(value); }
bool IntegrationContext::removeIntegration(const QString &id) { return m_controller.removeIntegration(id); }
QString IntegrationContext::duplicateIntegration(const QString &id) { return m_controller.duplicateIntegration(id); }
bool IntegrationContext::setIntegrationEnabled(const QString &id, bool enabled) { return m_controller.setIntegrationEnabled(id, enabled); }
QString IntegrationContext::setIntegrationSecret(const QString &id, const QString &field, const QString &secret) { return m_controller.setIntegrationSecret(id, field, secret); }
bool IntegrationContext::testIntegration(const QString &id) { return m_controller.testIntegration(id); }
bool IntegrationContext::executeIntegration(const QString &id, const QString &operation, const QVariantMap &payload) { return m_controller.executeIntegration(id, operation, payload); }
QVariantMap IntegrationContext::integrationDefinition(const QString &id) const { return m_controller.integrationDefinition(id); }
QStringList IntegrationContext::midiOutputPorts() const { return m_controller.midiOutputPorts(); }
QStringList IntegrationContext::integrationOperations(const QString &type) const { return m_controller.integrationOperations(type); }

} // namespace churchpresenter
