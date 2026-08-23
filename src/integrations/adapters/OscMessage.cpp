#include "integrations/adapters/OscMessage.h"

#include <QtEndian>

namespace churchpresenter {

bool OscMessage::isValidAddress(const QString &address)
{
    if (!address.startsWith(QLatin1Char('/'))) return false;
    if (address.size() > 255) return false;
    static const QString forbidden = QStringLiteral(" #*,?[]{}");
    for (const auto character : address) {
        if (forbidden.contains(character) || character.unicode() < 32
            || character.unicode() > 126) {
            return false;
        }
    }
    return true;
}

QByteArray OscMessage::padded(const QByteArray &value)
{
    QByteArray result = value;
    result.append('\0');
    while (result.size() % 4 != 0) result.append('\0');
    return result;
}

std::optional<QByteArray> OscMessage::encode(const QString &address,
                                             const QVariantList &arguments)
{
    if (!isValidAddress(address)) return std::nullopt;

    QByteArray typeTags = ",";
    QByteArray body;
    for (const auto &argument : arguments) {
        switch (argument.typeId()) {
        case QMetaType::Bool:
            typeTags.append(argument.toBool() ? 'T' : 'F');
            break;
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::UInt: {
            bool ok = false;
            const auto value = argument.toInt(&ok);
            if (!ok) return std::nullopt;
            qint32 encoded = qToBigEndian<qint32>(value);
            body.append(reinterpret_cast<const char *>(&encoded), sizeof(encoded));
            typeTags.append('i');
            break;
        }
        case QMetaType::Float:
        case QMetaType::Double: {
            bool ok = false;
            const auto value = static_cast<float>(argument.toDouble(&ok));
            if (!ok) return std::nullopt;
            quint32 bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            bits = qToBigEndian<quint32>(bits);
            body.append(reinterpret_cast<const char *>(&bits), sizeof(bits));
            typeTags.append('f');
            break;
        }
        case QMetaType::QString:
            body.append(padded(argument.toString().toUtf8()));
            typeTags.append('s');
            break;
        default:
            return std::nullopt;
        }
    }

    QByteArray datagram = padded(address.toUtf8());
    datagram.append(padded(typeTags));
    datagram.append(body);
    if (datagram.size() > MaximumDatagramBytes) return std::nullopt;
    return datagram;
}

} // namespace churchpresenter
