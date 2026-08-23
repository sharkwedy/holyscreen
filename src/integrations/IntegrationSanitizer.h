#pragma once

#include "integrations/IntegrationTypes.h"

namespace churchpresenter {

//! Remove segredos de configurações, mensagens e metadados antes de qualquer
//! log, evento, histórico, diagnóstico ou exportação.
class IntegrationSanitizer final {
public:
    static constexpr auto Redacted = "***";

    //! Chaves consideradas sensíveis mesmo sem estarem em `secretReferences`.
    [[nodiscard]] static bool isSensitiveKey(const QString &key);

    [[nodiscard]] static QVariantMap sanitizedConfiguration(
        const IntegrationDefinition &definition);
    [[nodiscard]] static IntegrationDefinition sanitizedDefinition(
        const IntegrationDefinition &definition);
    [[nodiscard]] static QVariantMap sanitizedMetadata(const QVariantMap &metadata,
                                                       const QStringList &secrets = {});
    //! Substitui segredos conhecidos e credenciais embutidas em URLs.
    [[nodiscard]] static QString sanitizedText(const QString &text,
                                               const QStringList &secrets = {});
    [[nodiscard]] static IntegrationResult sanitizedResult(const IntegrationResult &result,
                                                           const QStringList &secrets = {});
};

} // namespace churchpresenter
