#include "integrations/secrets/SecretStoreFactory.h"

#include "integrations/secrets/InMemorySecretStore.h"

#include <QDebug>

#if defined(Q_OS_MACOS)
#include "integrations/secrets/KeychainSecretStore.h"
#elif defined(Q_OS_WIN)
#include "integrations/secrets/CredentialManagerSecretStore.h"
#elif defined(HOLYSCREEN_HAS_LIBSECRET)
#include "integrations/secrets/SecretServiceSecretStore.h"
#endif

namespace churchpresenter {

std::unique_ptr<ISecretStore> SecretStoreFactory::create()
{
    // Permite rodar testes e sessões descartáveis sem tocar no cofre real do
    // usuário. O armazenamento em memória se declara não persistente, então a
    // interface continua avisando o operador.
    if (qEnvironmentVariable("HOLYSCREEN_SECRET_STORE").compare(QStringLiteral("memory"),
                                                                Qt::CaseInsensitive) == 0) {
        qInfo() << "HOLYSCREEN_SECRET_STORE=memory: secrets will only live in memory.";
        return std::make_unique<InMemorySecretStore>();
    }

#if defined(Q_OS_MACOS)
    if (KeychainSecretStore::isAvailable()) return std::make_unique<KeychainSecretStore>();
#elif defined(Q_OS_WIN)
    if (CredentialManagerSecretStore::isAvailable()) {
        return std::make_unique<CredentialManagerSecretStore>();
    }
#elif defined(HOLYSCREEN_HAS_LIBSECRET)
    if (SecretServiceSecretStore::isAvailable()) {
        return std::make_unique<SecretServiceSecretStore>();
    }
#endif
    qWarning() << "No system secret store available; secrets will only live in memory.";
    return std::make_unique<InMemorySecretStore>();
}

QString SecretStoreFactory::platformBackendName()
{
#if defined(Q_OS_MACOS)
    return QStringLiteral("Keychain do macOS");
#elif defined(Q_OS_WIN)
    return QStringLiteral("Gerenciador de Credenciais do Windows");
#elif defined(HOLYSCREEN_HAS_LIBSECRET)
    return QStringLiteral("Secret Service do Linux");
#else
    return QStringLiteral("nenhum cofre do sistema disponível nesta compilação");
#endif
}

} // namespace churchpresenter
