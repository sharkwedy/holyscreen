#pragma once

#include "core/CommandBus.h"
#include "core/EventBus.h"

#include <QObject>
#include <QVariantMap>

#include <functional>

namespace churchpresenter {

//! Entrada única das integrações pela CommandBus. Na 0.12 os comandos são
//! apenas de desktop: o catálogo não os libera para o controle remoto.
class IntegrationCommandModule final : public QObject {
    Q_OBJECT

public:
    struct Actions {
        //! Executa a operação e devolve o resultado sanitizado.
        std::function<QVariantMap(const QString &integrationId, const QString &operation,
                                  const QVariantMap &payload, const QString &correlationId)>
            execute;
        //! Testa a conexão sem executar ação destrutiva.
        std::function<QVariantMap(const QString &integrationId, const QString &correlationId)>
            test;
        //! Definição sanitizada, usada para validar o pedido.
        std::function<QVariantMap(const QString &integrationId)> definition;
    };

    IntegrationCommandModule(CommandBus &commandBus, EventBus &eventBus, Actions actions,
                             QObject *parent = nullptr);

    CommandResult requestTest(const QString &integrationId,
                              const QString &source = QStringLiteral("operator"));
    CommandResult requestExecute(const QString &integrationId, const QString &operation,
                                 const QVariantMap &payload,
                                 const QString &source = QStringLiteral("operator"));

private:
    CommandResult dispatch(const QString &type, const QVariantMap &payload,
                           const QString &source);
    void publishResult(const QString &integrationId, const QString &operation,
                       const QVariantMap &result, const QString &correlationId);
    [[nodiscard]] CommandResult invalidPayload(const QString &message) const;

    CommandBus &m_commandBus;
    EventBus &m_eventBus;
    Actions m_actions;
};

} // namespace churchpresenter
