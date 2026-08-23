#include "integrations/secrets/CredentialManagerSecretStore.h"

#include <QDebug>

#include <windows.h>
#include <wincred.h>

namespace churchpresenter {
namespace {

QString targetName(const QString &reference)
{
    return QString::fromLatin1(CredentialManagerSecretStore::TargetPrefix) + reference;
}

} // namespace

bool CredentialManagerSecretStore::store(const QString &reference, const QString &secret)
{
    if (reference.trimmed().isEmpty()) return false;
    const auto target = targetName(reference);
    auto secretBytes = secret.toUtf8();

    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(target.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(secretBytes.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(secretBytes.data());
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;

    if (!CredWriteW(&credential, 0)) {
        qWarning() << "Credential Manager rejected the secret, error" << GetLastError();
        return false;
    }
    return true;
}

std::optional<QString> CredentialManagerSecretStore::retrieve(const QString &reference) const
{
    const auto target = targetName(reference);
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0, &credential)) {
        return std::nullopt;
    }
    const auto secret = QString::fromUtf8(
        reinterpret_cast<const char *>(credential->CredentialBlob),
        static_cast<qsizetype>(credential->CredentialBlobSize));
    CredFree(credential);
    return secret;
}

bool CredentialManagerSecretStore::remove(const QString &reference)
{
    const auto target = targetName(reference);
    return CredDeleteW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0);
}

QStringList CredentialManagerSecretStore::references() const
{
    const auto filter = QString::fromLatin1(TargetPrefix) + QStringLiteral("*");
    DWORD count = 0;
    PCREDENTIALW *credentials = nullptr;
    if (!CredEnumerateW(reinterpret_cast<LPCWSTR>(filter.utf16()), 0, &count, &credentials)) {
        return {};
    }
    QStringList references;
    for (DWORD index = 0; index < count; ++index) {
        const auto target = QString::fromWCharArray(credentials[index]->TargetName);
        references.append(target.mid(QString::fromLatin1(TargetPrefix).size()));
    }
    CredFree(credentials);
    references.sort();
    return references;
}

QString CredentialManagerSecretStore::backendName() const
{
    return QStringLiteral("Gerenciador de Credenciais do Windows");
}

bool CredentialManagerSecretStore::isAvailable()
{
    const auto probe = targetName(QStringLiteral("availability-probe"));
    PCREDENTIALW credential = nullptr;
    if (CredReadW(reinterpret_cast<LPCWSTR>(probe.utf16()), CRED_TYPE_GENERIC, 0, &credential)) {
        CredFree(credential);
        return true;
    }
    // Credencial ausente é a resposta esperada; qualquer outro erro indica que
    // o cofre não está acessível.
    return GetLastError() == ERROR_NOT_FOUND;
}

} // namespace churchpresenter
