#pragma once

#include <QString>

namespace churchpresenter {

//! Desafio-resposta do protocolo OBS WebSocket v5.
class ObsAuthentication final {
public:
    //! `base64(sha256(base64(sha256(senha + salt)) + challenge))`.
    [[nodiscard]] static QString response(const QString &password, const QString &salt,
                                          const QString &challenge);
};

} // namespace churchpresenter
