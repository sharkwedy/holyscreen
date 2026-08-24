#include "integrations/secrets/InMemorySecretStore.h"

namespace churchpresenter {

bool InMemorySecretStore::store(const QString &reference, const QString &secret)
{
    if (reference.trimmed().isEmpty()) return false;
    m_secrets.insert(reference, secret);
    return true;
}

std::optional<QString> InMemorySecretStore::retrieve(const QString &reference) const
{
    const auto found = m_secrets.constFind(reference);
    if (found == m_secrets.cend()) return std::nullopt;
    return *found;
}

bool InMemorySecretStore::remove(const QString &reference)
{
    return m_secrets.remove(reference);
}

QStringList InMemorySecretStore::references() const
{
    auto keys = m_secrets.keys();
    keys.sort();
    return keys;
}

QString InMemorySecretStore::backendName() const
{
    return QStringLiteral("memória (somente nesta sessão)");
}

} // namespace churchpresenter
