#pragma once

#include "integrations/ports/ISecretStore.h"

namespace churchpresenter {

//! Cofre do Windows (Credential Manager), com credenciais genéricas
//! prefixadas por `HolyScreen:`.
class CredentialManagerSecretStore final : public ISecretStore {
public:
    static constexpr auto TargetPrefix = "HolyScreen:";

    bool store(const QString &reference, const QString &secret) override;
    [[nodiscard]] std::optional<QString> retrieve(const QString &reference) const override;
    bool remove(const QString &reference) override;
    [[nodiscard]] QStringList references() const override;
    [[nodiscard]] bool isPersistent() const override { return true; }
    [[nodiscard]] QString backendName() const override;

    [[nodiscard]] static bool isAvailable();
};

} // namespace churchpresenter
