#include "integrations/secrets/SecretServiceSecretStore.h"

#include <QDebug>

// O GDBus declara um campo chamado `signals`, que a macro de palavra-chave do
// Qt transformaria em `public`. As macros saem de cena só para este include.
#pragma push_macro("signals")
#pragma push_macro("slots")
#pragma push_macro("emit")
#undef signals
#undef slots
#undef emit
#include <libsecret/secret.h>
#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")

namespace churchpresenter {
namespace {

const SecretSchema *holyscreenSchema()
{
    static const SecretSchema schema = {
        "app.holyscreen.Secret",
        SECRET_SCHEMA_NONE,
        {
            {"application", SECRET_SCHEMA_ATTRIBUTE_STRING},
            {"reference", SECRET_SCHEMA_ATTRIBUTE_STRING},
            {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
        },
        0, 0, 0, 0, 0, 0, 0, 0,
    };
    return &schema;
}

} // namespace

bool SecretServiceSecretStore::store(const QString &reference, const QString &secret)
{
    if (reference.trimmed().isEmpty()) return false;
    GError *error = nullptr;
    const auto label = QStringLiteral("HolyScreen — %1").arg(reference);
    const bool stored = secret_password_store_sync(
        holyscreenSchema(), SECRET_COLLECTION_DEFAULT, label.toUtf8().constData(),
        secret.toUtf8().constData(), nullptr, &error,
        "application", ApplicationName,
        "reference", reference.toUtf8().constData(), nullptr);
    if (error) {
        qWarning() << "Secret Service rejected the secret:" << error->message;
        g_error_free(error);
        return false;
    }
    return stored;
}

std::optional<QString> SecretServiceSecretStore::retrieve(const QString &reference) const
{
    GError *error = nullptr;
    gchar *secret = secret_password_lookup_sync(
        holyscreenSchema(), nullptr, &error,
        "application", ApplicationName,
        "reference", reference.toUtf8().constData(), nullptr);
    if (error) {
        qWarning() << "Secret Service could not read the secret:" << error->message;
        g_error_free(error);
        return std::nullopt;
    }
    if (!secret) return std::nullopt;
    const auto value = QString::fromUtf8(secret);
    secret_password_free(secret);
    return value;
}

bool SecretServiceSecretStore::remove(const QString &reference)
{
    GError *error = nullptr;
    const bool removed = secret_password_clear_sync(
        holyscreenSchema(), nullptr, &error,
        "application", ApplicationName,
        "reference", reference.toUtf8().constData(), nullptr);
    if (error) {
        g_error_free(error);
        return false;
    }
    return removed;
}

QStringList SecretServiceSecretStore::references() const
{
    GHashTable *attributes = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(attributes, const_cast<char *>("application"),
                        const_cast<char *>(ApplicationName));
    GError *error = nullptr;
    GList *items = secret_service_search_sync(
        nullptr, holyscreenSchema(), attributes,
        static_cast<SecretSearchFlags>(SECRET_SEARCH_ALL | SECRET_SEARCH_UNLOCK),
        nullptr, &error);
    g_hash_table_destroy(attributes);
    if (error) {
        g_error_free(error);
        return {};
    }

    QStringList references;
    for (GList *node = items; node; node = node->next) {
        auto *item = static_cast<SecretItem *>(node->data);
        GHashTable *itemAttributes = secret_item_get_attributes(item);
        if (itemAttributes) {
            const auto *reference = static_cast<const char *>(
                g_hash_table_lookup(itemAttributes, "reference"));
            if (reference) references.append(QString::fromUtf8(reference));
            g_hash_table_unref(itemAttributes);
        }
    }
    g_list_free_full(items, g_object_unref);
    references.sort();
    return references;
}

QString SecretServiceSecretStore::backendName() const
{
    return QStringLiteral("Secret Service do Linux");
}

bool SecretServiceSecretStore::isAvailable()
{
    GError *error = nullptr;
    SecretService *service = secret_service_get_sync(SECRET_SERVICE_NONE, nullptr, &error);
    if (error) {
        g_error_free(error);
        return false;
    }
    if (!service) return false;
    g_object_unref(service);
    return true;
}

} // namespace churchpresenter
