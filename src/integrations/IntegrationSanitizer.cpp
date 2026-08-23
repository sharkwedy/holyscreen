#include "integrations/IntegrationSanitizer.h"

#include <QRegularExpression>
#include <QUrl>

namespace churchpresenter {
namespace {

const QStringList &sensitiveFragments()
{
    static const QStringList fragments{
        QStringLiteral("password"), QStringLiteral("secret"), QStringLiteral("token"),
        QStringLiteral("apikey"),   QStringLiteral("api_key"), QStringLiteral("authorization"),
        QStringLiteral("credential"), QStringLiteral("passphrase"), QStringLiteral("senha"),
    };
    return fragments;
}

QString redactUrlCredentials(const QString &text)
{
    static const QRegularExpression pattern(
        QStringLiteral("([a-zA-Z][a-zA-Z0-9+.-]*://)([^/\\s:@]+):([^/\\s@]+)@"));
    QString result = text;
    return result.replace(pattern, QStringLiteral("\\1\\2:%1@").arg(QLatin1StringView(
        IntegrationSanitizer::Redacted)));
}

} // namespace

bool IntegrationSanitizer::isSensitiveKey(const QString &key)
{
    const auto normalized = key.toLower();
    for (const auto &fragment : sensitiveFragments()) {
        if (normalized.contains(fragment)) return true;
    }
    return false;
}

QVariantMap IntegrationSanitizer::sanitizedConfiguration(const IntegrationDefinition &definition)
{
    QVariantMap sanitized;
    for (auto it = definition.configuration.cbegin(); it != definition.configuration.cend(); ++it) {
        const auto sensitive = isSensitiveKey(it.key())
            || definition.secretReferences.contains(it.value().toString());
        if (sensitive) {
            sanitized.insert(it.key(), QString::fromLatin1(Redacted));
            continue;
        }
        if (it.value().typeId() == QMetaType::QVariantMap) {
            IntegrationDefinition nested = definition;
            nested.configuration = it.value().toMap();
            sanitized.insert(it.key(), sanitizedConfiguration(nested));
            continue;
        }
        if (it.value().typeId() == QMetaType::QString) {
            sanitized.insert(it.key(), sanitizedText(it.value().toString()));
            continue;
        }
        sanitized.insert(it.key(), it.value());
    }
    return sanitized;
}

IntegrationDefinition IntegrationSanitizer::sanitizedDefinition(
    const IntegrationDefinition &definition)
{
    IntegrationDefinition sanitized = definition;
    sanitized.configuration = sanitizedConfiguration(definition);
    return sanitized;
}

QVariantMap IntegrationSanitizer::sanitizedMetadata(const QVariantMap &metadata,
                                                    const QStringList &secrets)
{
    QVariantMap sanitized;
    for (auto it = metadata.cbegin(); it != metadata.cend(); ++it) {
        if (isSensitiveKey(it.key())) {
            sanitized.insert(it.key(), QString::fromLatin1(Redacted));
            continue;
        }
        if (it.value().typeId() == QMetaType::QVariantMap) {
            sanitized.insert(it.key(), sanitizedMetadata(it.value().toMap(), secrets));
            continue;
        }
        if (it.value().typeId() == QMetaType::QString) {
            sanitized.insert(it.key(), sanitizedText(it.value().toString(), secrets));
            continue;
        }
        sanitized.insert(it.key(), it.value());
    }
    return sanitized;
}

QString IntegrationSanitizer::sanitizedText(const QString &text, const QStringList &secrets)
{
    QString sanitized = redactUrlCredentials(text);
    for (const auto &secret : secrets) {
        if (secret.isEmpty()) continue;
        sanitized.replace(secret, QString::fromLatin1(Redacted));
    }
    return sanitized;
}

IntegrationResult IntegrationSanitizer::sanitizedResult(const IntegrationResult &result,
                                                        const QStringList &secrets)
{
    IntegrationResult sanitized = result;
    sanitized.message = sanitizedText(result.message, secrets);
    sanitized.responseMetadata = sanitizedMetadata(result.responseMetadata, secrets);
    return sanitized;
}

} // namespace churchpresenter
