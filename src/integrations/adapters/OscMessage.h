#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantList>

#include <optional>

namespace churchpresenter {

//! Codificador OSC 1.0 sobre UDP. Aceita int32, float32, string e bool, que
//! viram os tipos `i`, `f`, `s`, `T` e `F`.
class OscMessage final {
public:
    static constexpr int MaximumDatagramBytes = 8192;

    //! Um endereço OSC começa com `/` e não usa espaços nem os caracteres
    //! reservados `#`, `,` e os curingas de padrão.
    [[nodiscard]] static bool isValidAddress(const QString &address);

    //! Codifica o datagrama; devolve `nullopt` quando o endereço é inválido,
    //! um argumento não é suportado ou o datagrama excederia o limite.
    [[nodiscard]] static std::optional<QByteArray> encode(const QString &address,
                                                          const QVariantList &arguments);

    //! Alinhamento OSC: toda parte é múltipla de quatro bytes.
    [[nodiscard]] static QByteArray padded(const QByteArray &value);
};

} // namespace churchpresenter
