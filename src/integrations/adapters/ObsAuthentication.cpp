#include "integrations/adapters/ObsAuthentication.h"

#include <QCryptographicHash>

namespace churchpresenter {
namespace {

QByteArray base64Sha256(const QByteArray &value)
{
    return QCryptographicHash::hash(value, QCryptographicHash::Sha256).toBase64();
}

} // namespace

QString ObsAuthentication::response(const QString &password, const QString &salt,
                                    const QString &challenge)
{
    const auto secret = base64Sha256(password.toUtf8() + salt.toUtf8());
    return QString::fromLatin1(base64Sha256(secret + challenge.toUtf8()));
}

} // namespace churchpresenter
