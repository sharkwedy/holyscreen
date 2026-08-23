#pragma once

#include "integrations/ports/ITransports.h"

#include <QDateTime>
#include <QList>
#include <QString>

#include <optional>

namespace churchpresenter {

//! Allowlist de executáveis que uma automação pode acionar. O recurso vem
//! desligado por padrão e a autorização é sempre por caminho canônico.
class AuthorizedExecutables final {
public:
    struct Entry {
        QString canonicalPath;
        QString label;
        QDateTime authorizedAt;
    };

    static constexpr int MinimumTimeoutMs = 250;
    static constexpr int MaximumTimeoutMs = 30000;
    static constexpr qint64 MaximumOutputBytes = 64 * 1024;
    static constexpr int MaximumArguments = 32;

    //! Interruptor global. Enquanto estiver desligado, nenhum processo roda.
    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const;

    //! Autoriza um executável. O caminho é resolvido para a forma canônica,
    //! então um symlink autorizado passa a valer pelo destino real.
    bool authorize(const QString &path, const QString &label, QString *error = nullptr);
    bool revoke(const QString &canonicalPath);
    [[nodiscard]] QList<Entry> entries() const;
    void restore(const QList<Entry> &entries);
    [[nodiscard]] bool isAuthorized(const QString &path) const;

    //! Valida e normaliza um pedido de processo vindo de uma automação.
    [[nodiscard]] std::optional<ProcessRequest> validate(const QVariantMap &parameters,
                                                         QString *errorCode,
                                                         QString *message) const;

private:
    QList<Entry> m_entries;
    bool m_enabled = false;
};

} // namespace churchpresenter
