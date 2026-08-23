#include "integrations/secrets/KeychainSecretStore.h"

#include <QDebug>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

namespace churchpresenter {
namespace {

struct CfReleaser {
    CFTypeRef ref = nullptr;
    ~CfReleaser() { if (ref) CFRelease(ref); }
};

CFStringRef cfString(const QByteArray &utf8)
{
    return CFStringCreateWithBytes(kCFAllocatorDefault,
                                   reinterpret_cast<const UInt8 *>(utf8.constData()),
                                   utf8.size(), kCFStringEncodingUTF8, false);
}

CFMutableDictionaryRef baseQuery(const QString &reference)
{
    auto *query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                            &kCFTypeDictionaryKeyCallBacks,
                                            &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    auto *service = cfString(QByteArray(KeychainSecretStore::ServiceName));
    CFDictionarySetValue(query, kSecAttrService, service);
    CFRelease(service);
    if (!reference.isEmpty()) {
        auto *account = cfString(reference.toUtf8());
        CFDictionarySetValue(query, kSecAttrAccount, account);
        CFRelease(account);
    }
    return query;
}

} // namespace

bool KeychainSecretStore::store(const QString &reference, const QString &secret)
{
    if (reference.trimmed().isEmpty()) return false;
    auto *query = baseQuery(reference);
    const CfReleaser queryReleaser{query};
    SecItemDelete(query);

    const auto data = secret.toUtf8();
    auto *value = CFDataCreate(kCFAllocatorDefault,
                               reinterpret_cast<const UInt8 *>(data.constData()), data.size());
    const CfReleaser valueReleaser{value};
    CFDictionarySetValue(query, kSecValueData, value);
    CFDictionarySetValue(query, kSecAttrAccessible, kSecAttrAccessibleWhenUnlocked);
    const auto status = SecItemAdd(query, nullptr);
    if (status != errSecSuccess) {
        qWarning() << "Keychain rejected the secret, status" << status;
        return false;
    }
    return true;
}

std::optional<QString> KeychainSecretStore::retrieve(const QString &reference) const
{
    auto *query = baseQuery(reference);
    const CfReleaser queryReleaser{query};
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);

    CFTypeRef result = nullptr;
    if (SecItemCopyMatching(query, &result) != errSecSuccess || !result) return std::nullopt;
    const CfReleaser resultReleaser{result};
    const auto *data = static_cast<CFDataRef>(result);
    return QString::fromUtf8(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                             static_cast<qsizetype>(CFDataGetLength(data)));
}

bool KeychainSecretStore::remove(const QString &reference)
{
    auto *query = baseQuery(reference);
    const CfReleaser queryReleaser{query};
    return SecItemDelete(query) == errSecSuccess;
}

QStringList KeychainSecretStore::references() const
{
    auto *query = baseQuery({});
    const CfReleaser queryReleaser{query};
    CFDictionarySetValue(query, kSecReturnAttributes, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitAll);

    CFTypeRef result = nullptr;
    if (SecItemCopyMatching(query, &result) != errSecSuccess || !result) return {};
    const CfReleaser resultReleaser{result};

    QStringList references;
    const auto *items = static_cast<CFArrayRef>(result);
    for (CFIndex index = 0; index < CFArrayGetCount(items); ++index) {
        const auto *item = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(items, index));
        const auto *account = static_cast<CFStringRef>(
            CFDictionaryGetValue(item, kSecAttrAccount));
        if (!account) continue;
        const auto length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(account),
                                                              kCFStringEncodingUTF8) + 1;
        QByteArray buffer(static_cast<qsizetype>(length), '\0');
        if (CFStringGetCString(account, buffer.data(), length, kCFStringEncodingUTF8)) {
            references.append(QString::fromUtf8(buffer.constData()));
        }
    }
    references.sort();
    return references;
}

QString KeychainSecretStore::backendName() const
{
    return QStringLiteral("Keychain do macOS");
}

bool KeychainSecretStore::isAvailable()
{
    auto *query = baseQuery(QStringLiteral("holyscreen/availability-probe"));
    const CfReleaser queryReleaser{query};
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);
    const auto status = SecItemCopyMatching(query, nullptr);
    // Item ausente é a resposta esperada: o cofre respondeu.
    return status == errSecSuccess || status == errSecItemNotFound;
}

} // namespace churchpresenter
