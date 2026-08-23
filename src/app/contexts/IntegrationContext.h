#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace churchpresenter {

class ApplicationController;

class IntegrationContext final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList integrations READ integrations NOTIFY integrationsChanged)
    Q_PROPERTY(QVariantList integrationHistory READ integrationHistory NOTIFY integrationHistoryChanged)
    Q_PROPERTY(QString integrationStatus READ integrationStatus NOTIFY integrationStatusChanged)
    Q_PROPERTY(QStringList integrationTypes READ integrationTypes CONSTANT)
    Q_PROPERTY(QString integrationSecretBackend READ integrationSecretBackend CONSTANT)
    Q_PROPERTY(bool integrationSecretsPersistent READ integrationSecretsPersistent CONSTANT)

public:
    explicit IntegrationContext(ApplicationController &controller, QObject *parent = nullptr);

    [[nodiscard]] QVariantList integrations() const;
    [[nodiscard]] QVariantList integrationHistory() const;
    [[nodiscard]] QString integrationStatus() const;
    [[nodiscard]] QStringList integrationTypes() const;
    [[nodiscard]] QString integrationSecretBackend() const;
    [[nodiscard]] bool integrationSecretsPersistent() const;

    Q_INVOKABLE QVariantMap saveIntegration(const QVariantMap &definition);
    Q_INVOKABLE bool removeIntegration(const QString &integrationId);
    Q_INVOKABLE QString duplicateIntegration(const QString &integrationId);
    Q_INVOKABLE bool setIntegrationEnabled(const QString &integrationId, bool enabled);
    Q_INVOKABLE QString setIntegrationSecret(const QString &integrationId, const QString &field,
                                             const QString &secret);
    Q_INVOKABLE bool testIntegration(const QString &integrationId);
    Q_INVOKABLE bool executeIntegration(const QString &integrationId, const QString &operation,
                                        const QVariantMap &payload = {});
    Q_INVOKABLE QVariantMap integrationDefinition(const QString &integrationId) const;
    Q_INVOKABLE QStringList midiOutputPorts() const;
    Q_INVOKABLE QStringList integrationOperations(const QString &type) const;

signals:
    void integrationsChanged();
    void integrationHistoryChanged();
    void integrationStatusChanged();

private:
    ApplicationController &m_controller;
};

} // namespace churchpresenter
