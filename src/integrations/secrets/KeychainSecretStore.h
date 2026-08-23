#pragma once

#include "integrations/ports/ISecretStore.h"

namespace churchpresenter {

//! Cofre do macOS (Keychain Services), usando itens de senha genérica com o
//! serviço `HolyScreen`.
class KeychainSecretStore final : public ISecretStore {
public:
    static constexpr auto ServiceName = "HolyScreen";

    bool store(const QString &reference, const QString &secret) override;
    [[nodiscard]] std::optional<QString> retrieve(const QString &reference) const override;
    bool remove(const QString &reference) override;
    [[nodiscard]] QStringList references() const override;
    [[nodiscard]] bool isPersistent() const override { return true; }
    [[nodiscard]] QString backendName() const override;

    //! Verdadeiro quando o Keychain responde a uma consulta simples.
    [[nodiscard]] static bool isAvailable();
};

} // namespace churchpresenter
