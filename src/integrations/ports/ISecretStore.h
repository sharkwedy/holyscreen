#pragma once

#include <QString>
#include <QStringList>

#include <optional>

namespace churchpresenter {

//! Porta de armazenamento de segredos. Implementações persistentes usam o
//! cofre do sistema operacional; quando não houver cofre, o segredo fica
//! somente em memória e a interface avisa o operador — nunca há fallback
//! silencioso para texto puro.
class ISecretStore {
public:
    virtual ~ISecretStore() = default;

    virtual bool store(const QString &reference, const QString &secret) = 0;
    [[nodiscard]] virtual std::optional<QString> retrieve(const QString &reference) const = 0;
    virtual bool remove(const QString &reference) = 0;
    [[nodiscard]] virtual QStringList references() const = 0;

    //! Falso quando os segredos serão perdidos ao fechar o aplicativo.
    [[nodiscard]] virtual bool isPersistent() const = 0;
    //! Nome legível do backend, usado em diagnósticos.
    [[nodiscard]] virtual QString backendName() const = 0;
};

} // namespace churchpresenter
