#pragma once

#include "integrations/IntegrationTypes.h"

#include <functional>

namespace churchpresenter {

//! Porta implementada por cada protocolo. Os adapters são assíncronos para que
//! nenhuma chamada bloqueie a operação do culto.
class IIntegrationAdapter {
public:
    using Completion = std::function<void(const IntegrationResult &)>;

    virtual ~IIntegrationAdapter() = default;

    //! Valida a configuração antes de persistir a definição.
    [[nodiscard]] virtual IntegrationValidation validate(
        const IntegrationDefinition &definition) const = 0;

    //! Teste de conexão. Nunca pode executar ação destrutiva.
    virtual void test(const IntegrationDefinition &definition, Completion completion) = 0;

    virtual void execute(const IntegrationDefinition &definition,
                         const IntegrationRequest &request, Completion completion) = 0;

    //! Cancela chamadas em andamento no encerramento do aplicativo.
    virtual void cancelAll() = 0;

    //! Verdadeiro apenas quando reenviar a operação não causa efeito duplicado.
    [[nodiscard]] virtual bool isRetriable(const QString &operation) const
    {
        Q_UNUSED(operation);
        return false;
    }
};

} // namespace churchpresenter
