#pragma once

#include "integrations/ports/ISecretStore.h"

#include <memory>

namespace churchpresenter {

//! Escolhe o cofre do sistema operacional; quando nenhum estiver disponível,
//! devolve o cofre em memória, que se declara não persistente para que a
//! interface avise o operador.
class SecretStoreFactory final {
public:
    [[nodiscard]] static std::unique_ptr<ISecretStore> create();
    //! Nome do backend nativo desta plataforma, mesmo quando indisponível.
    [[nodiscard]] static QString platformBackendName();
};

} // namespace churchpresenter
