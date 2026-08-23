#pragma once

#include <QByteArray>
#include <QStringList>
#include <QVariantMap>

namespace churchpresenter {

struct ConfigurationProfileResult {
    bool accepted = false;
    QVariantMap profile;
    QStringList errors;
};

// Versioned, secret-free operator configuration document. Persistence is kept
// outside this service so validation always completes before any setting is
// changed.
class ConfigurationProfileService final {
public:
    static constexpr qsizetype MaximumDocumentSize = 1024 * 1024;
    static constexpr int SchemaVersion = 1;

    [[nodiscard]] static ConfigurationProfileResult validate(const QVariantMap &profile);
    [[nodiscard]] static ConfigurationProfileResult parse(const QByteArray &document);
    [[nodiscard]] static QByteArray serialize(const QVariantMap &profile,
                                              QStringList *errors = nullptr);
};

} // namespace churchpresenter
