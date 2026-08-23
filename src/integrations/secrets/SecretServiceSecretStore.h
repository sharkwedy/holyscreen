#pragma once

#include "integrations/ports/ISecretStore.h"

namespace churchpresenter {

//! Cofre do Linux via Secret Service (libsecret). Cada segredo é gravado com
//! os atributos `application=HolyScreen` e `reference=<referência>`.
class SecretServiceSecretStore final : public ISecretStore {
public:
    static constexpr auto ApplicationName = "HolyScreen";

    bool store(const QString &reference, const QString &secret) override;
    [[nodiscard]] std::optional<QString> retrieve(const QString &reference) const override;
    bool remove(const QString &reference) override;
    [[nodiscard]] QStringList references() const override;
    [[nodiscard]] bool isPersistent() const override { return true; }
    [[nodiscard]] QString backendName() const override;

    [[nodiscard]] static bool isAvailable();
};

} // namespace churchpresenter
