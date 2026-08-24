#pragma once

#include "integrations/IntegrationTypes.h"
#include "integrations/ports/IIntegrationAdapter.h"
#include "integrations/ports/IIntegrationRepository.h"
#include "integrations/ports/ISecretStore.h"

#include <QHash>
#include <QObject>
#include <QVector>

#include <functional>
#include <memory>

namespace churchpresenter {

//! Orquestra as integrações: escolhe o adapter, valida antes de persistir,
//! aplica timeout e repetição limitada, cancela no encerramento e publica
//! resultados já sanitizados.
class IntegrationEngine final : public QObject {
    Q_OBJECT

public:
    static constexpr int MinimumTimeoutMs = 250;
    static constexpr int MaximumTimeoutMs = 60000;
    static constexpr int MaximumAttempts = 5;
    static constexpr int DefaultHistoryRetention = 200;

    using Completion = std::function<void(const IntegrationResult &)>;

    explicit IntegrationEngine(QObject *parent = nullptr);
    ~IntegrationEngine() override;

    //! O motor não assume a posse do adapter nem do repositório, e chama os
    //! dois durante a própria destruição para cancelar chamadas pendentes e
    //! registrar o cancelamento. Ambos precisam sobreviver ao motor: quem os
    //! mantém como membros deve declarar o motor **depois** deles, já que os
    //! membros são destruídos na ordem inversa da declaração.
    void registerAdapter(IntegrationType type, IIntegrationAdapter *adapter);
    void setRepository(IIntegrationRepository *repository);
    void setSecretStore(ISecretStore *secretStore);
    //! Relógio injetável para tornar as durações determinísticas nos testes.
    void setClock(std::function<QDateTime()> clock);
    void setHistoryRetention(int maximumEntriesPerIntegration);

    //! Recarrega as definições do repositório.
    void reload();
    [[nodiscard]] QVector<IntegrationDefinition> definitions() const;
    [[nodiscard]] std::optional<IntegrationDefinition> definition(const QString &id) const;
    //! Definições sem qualquer segredo, para diagnóstico e exportação.
    [[nodiscard]] QVector<IntegrationDefinition> exportableDefinitions() const;

    [[nodiscard]] IntegrationValidation validate(const IntegrationDefinition &definition) const;
    IntegrationValidation save(const IntegrationDefinition &definition);
    bool remove(const QString &integrationId);

    void execute(const IntegrationRequest &request, Completion completion = {});
    void test(const QString &integrationId, Completion completion = {});
    void cancelAll();

    [[nodiscard]] QVector<IntegrationCall> history(const QString &integrationId = {},
                                                   int limit = 100) const;

signals:
    //! Resultado sanitizado de uma chamada aceita ou recusada.
    void callFinished(const churchpresenter::IntegrationRequest &request,
                      const churchpresenter::IntegrationResult &result);

private:
    struct PendingCall;

    void start(const IntegrationDefinition &definition, const IntegrationRequest &request,
               bool isTest, Completion completion);
    void attempt(const std::shared_ptr<PendingCall> &call);
    void finish(const std::shared_ptr<PendingCall> &call, IntegrationResult result);
    [[nodiscard]] QStringList knownSecrets(const IntegrationDefinition &definition) const;
    [[nodiscard]] QDateTime now() const;

    QHash<IntegrationType, IIntegrationAdapter *> m_adapters;
    QVector<IntegrationDefinition> m_definitions;
    IIntegrationRepository *m_repository = nullptr;
    ISecretStore *m_secretStore = nullptr;
    std::function<QDateTime()> m_clock;
    int m_historyRetention = DefaultHistoryRetention;
    QVector<std::shared_ptr<PendingCall>> m_pending;
    bool m_cancelling = false;
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::IntegrationRequest)
Q_DECLARE_METATYPE(churchpresenter::IntegrationResult)
