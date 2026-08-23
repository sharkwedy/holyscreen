#pragma once

#include "integrations/ports/ISecretStore.h"

#include <QHash>

namespace churchpresenter {

//! Cofre volátil usado quando o sistema não oferece armazenamento seguro. Os
//! segredos vivem apenas nesta execução e a interface precisa avisar o
//! operador; nunca há gravação em texto puro no disco.
class InMemorySecretStore final : public ISecretStore {
public:
    bool store(const QString &reference, const QString &secret) override;
    [[nodiscard]] std::optional<QString> retrieve(const QString &reference) const override;
    bool remove(const QString &reference) override;
    [[nodiscard]] QStringList references() const override;
    [[nodiscard]] bool isPersistent() const override { return false; }
    [[nodiscard]] QString backendName() const override;

private:
    QHash<QString, QString> m_secrets;
};

} // namespace churchpresenter
