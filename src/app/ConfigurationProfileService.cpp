#include "app/ConfigurationProfileService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeySequence>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <cmath>

namespace churchpresenter {
namespace {

bool hasSensitiveKey(const QVariant &value, const QString &path, QStringList &errors)
{
    static const QRegularExpression sensitive(
        QStringLiteral("(^|[._/-])(password|passwd|secret|token|credential|api[_-]?key)([._/-]|$)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto map = value.toMap();
    if (!map.isEmpty() || value.metaType().id() == QMetaType::QVariantMap) {
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            const auto fieldPath = path.isEmpty() ? it.key() : path + u'.' + it.key();
            if (sensitive.match(it.key()).hasMatch()) {
                errors.append(QStringLiteral("Campo sensível não permitido: %1").arg(fieldPath));
            }
            hasSensitiveKey(it.value(), fieldPath, errors);
        }
    }
    const auto list = value.toList();
    if (!list.isEmpty() || value.metaType().id() == QMetaType::QVariantList) {
        for (qsizetype index = 0; index < list.size(); ++index)
            hasSensitiveKey(list.at(index), path + QStringLiteral("[%1]").arg(index), errors);
    }
    return errors.isEmpty();
}

bool expectType(const QVariantMap &map, const QString &key, QMetaType::Type type,
                const QString &section, QStringList &errors)
{
    if (!map.contains(key)) return true;
    if (map.value(key).metaType().id() == type) return true;
    errors.append(QStringLiteral("%1.%2 possui tipo inválido.").arg(section, key));
    return false;
}

bool expectNumber(const QVariantMap &map, const QString &key, const QString &section,
                  QStringList &errors, bool integer = false)
{
    if (!map.contains(key)) return true;
    const auto value = map.value(key);
    const auto type = value.metaType().id();
    const bool numeric = type == QMetaType::Double || type == QMetaType::Float
        || type == QMetaType::Int || type == QMetaType::UInt
        || type == QMetaType::LongLong || type == QMetaType::ULongLong;
    if (numeric && (!integer || value.toDouble() == std::floor(value.toDouble()))) return true;
    errors.append(QStringLiteral("%1.%2 possui tipo inválido.").arg(section, key));
    return false;
}

bool expectStringList(const QVariantMap &map, const QString &key, const QString &section,
                      QStringList &errors)
{
    if (!map.contains(key)) return true;
    const auto value = map.value(key);
    if (value.metaType().id() == QMetaType::QStringList) return true;
    if (value.metaType().id() == QMetaType::QVariantList) {
        const auto list = value.toList();
        if (std::all_of(list.cbegin(), list.cend(), [](const QVariant &item) {
                return item.metaType().id() == QMetaType::QString;
            })) return true;
    }
    errors.append(QStringLiteral("%1.%2 deve ser uma lista de textos.").arg(section, key));
    return false;
}

void rejectUnknown(const QVariantMap &map, const QSet<QString> &allowed,
                   const QString &section, QStringList &errors)
{
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        if (!allowed.contains(it.key()))
            errors.append(QStringLiteral("Campo desconhecido: %1.%2").arg(section, it.key()));
    }
}

void expectOneOf(const QVariantMap &map, const QString &key,
                 const QSet<QString> &allowed, const QString &section,
                 QStringList &errors)
{
    if (!map.contains(key) || map.value(key).metaType().id() != QMetaType::QString) return;
    const auto value = map.value(key).toString();
    if (!allowed.contains(value))
        errors.append(QStringLiteral("%1.%2 possui valor inválido: %3")
                          .arg(section, key, value));
}

QVariantMap section(const QVariantMap &profile, const QString &name, QStringList &errors)
{
    if (!profile.contains(name)) return {};
    if (profile.value(name).metaType().id() != QMetaType::QVariantMap) {
        errors.append(QStringLiteral("%1 deve ser um objeto.").arg(name));
        return {};
    }
    return profile.value(name).toMap();
}

} // namespace

ConfigurationProfileResult ConfigurationProfileService::validate(const QVariantMap &profile)
{
    ConfigurationProfileResult result;
    result.profile = profile;
    rejectUnknown(profile,
                  {QStringLiteral("locale"), QStringLiteral("demoMode"),
                   QStringLiteral("interfaceScale"),
                   QStringLiteral("presentation"), QStringLiteral("media"),
                   QStringLiteral("bible"), QStringLiteral("remote"),
                   QStringLiteral("library"), QStringLiteral("outputs"),
                   QStringLiteral("onboarding"), QStringLiteral("shortcuts")},
                  QStringLiteral("profile"), result.errors);
    hasSensitiveKey(profile, {}, result.errors);

    expectType(profile, QStringLiteral("locale"), QMetaType::QString,
               QStringLiteral("profile"), result.errors);
    expectType(profile, QStringLiteral("demoMode"), QMetaType::Bool,
               QStringLiteral("profile"), result.errors);
    if (profile.contains(QStringLiteral("interfaceScale"))) {
        const auto value = profile.value(QStringLiteral("interfaceScale"));
        const auto type = value.metaType().id();
        const bool numeric = type == QMetaType::Double || type == QMetaType::Int
            || type == QMetaType::LongLong || type == QMetaType::UInt
            || type == QMetaType::ULongLong;
        const auto scale = value.toDouble();
        if (!numeric || (scale != 1.0 && scale != 1.5 && scale != 2.0)) {
            result.errors.append(QStringLiteral(
                "profile.interfaceScale deve ser 1.0, 1.5 ou 2.0."));
        }
    }
    if (profile.contains(QStringLiteral("locale"))) {
        const auto locale = profile.value(QStringLiteral("locale")).toString();
        if (locale != QStringLiteral("pt-BR") && locale != QStringLiteral("en-US"))
            result.errors.append(QStringLiteral("Locale não suportado: %1").arg(locale));
    }

    const auto presentation = section(profile, QStringLiteral("presentation"), result.errors);
    rejectUnknown(presentation,
                  {QStringLiteral("wallpaperColor"), QStringLiteral("wallpaperSource"),
                   QStringLiteral("wallpaperFit"), QStringLiteral("clockVisible"),
                   QStringLiteral("clockPosition"), QStringLiteral("clockFormat"),
                   QStringLiteral("clockFontFamily"), QStringLiteral("clockFontSize"),
                   QStringLiteral("clockColor")},
                  QStringLiteral("presentation"), result.errors);
    for (const auto &key : {"wallpaperColor", "wallpaperSource", "wallpaperFit", "clockPosition",
                            "clockFormat", "clockFontFamily", "clockColor"})
        expectType(presentation, QString::fromLatin1(key), QMetaType::QString,
                   QStringLiteral("presentation"), result.errors);
    expectType(presentation, QStringLiteral("clockVisible"), QMetaType::Bool,
               QStringLiteral("presentation"), result.errors);
    expectNumber(presentation, QStringLiteral("clockFontSize"),
                 QStringLiteral("presentation"), result.errors, true);
    expectOneOf(presentation, QStringLiteral("wallpaperFit"),
                {QStringLiteral("cover"), QStringLiteral("contain"),
                 QStringLiteral("stretch"), QStringLiteral("center")},
                QStringLiteral("presentation"), result.errors);
    expectOneOf(presentation, QStringLiteral("clockPosition"),
                {QStringLiteral("topLeft"), QStringLiteral("topCenter"),
                 QStringLiteral("topRight"), QStringLiteral("centerLeft"),
                 QStringLiteral("center"), QStringLiteral("centerRight"),
                 QStringLiteral("bottomLeft"), QStringLiteral("bottomCenter"),
                 QStringLiteral("bottomRight")},
                QStringLiteral("presentation"), result.errors);
    if (presentation.contains(QStringLiteral("clockFontSize"))) {
        const auto size = presentation.value(QStringLiteral("clockFontSize")).toInt();
        if (size < 16 || size > 240)
            result.errors.append(QStringLiteral("presentation.clockFontSize deve estar entre 16 e 240."));
    }

    const auto media = section(profile, QStringLiteral("media"), result.errors);
    rejectUnknown(media,
                  {QStringLiteral("volume"), QStringLiteral("audioOutputId"),
                   QStringLiteral("repeatMode"),
                   QStringLiteral("imageFit"), QStringLiteral("imageTransition"),
                   QStringLiteral("imageAutoplay"), QStringLiteral("imageIntervalMs")},
                  QStringLiteral("media"), result.errors);
    expectNumber(media, QStringLiteral("volume"), QStringLiteral("media"), result.errors);
    expectType(media, QStringLiteral("audioOutputId"), QMetaType::QString,
               QStringLiteral("media"), result.errors);
    expectType(media, QStringLiteral("repeatMode"), QMetaType::QString,
               QStringLiteral("media"), result.errors);
    expectType(media, QStringLiteral("imageFit"), QMetaType::QString,
               QStringLiteral("media"), result.errors);
    expectType(media, QStringLiteral("imageTransition"), QMetaType::QString,
               QStringLiteral("media"), result.errors);
    expectType(media, QStringLiteral("imageAutoplay"), QMetaType::Bool,
               QStringLiteral("media"), result.errors);
    expectNumber(media, QStringLiteral("imageIntervalMs"), QStringLiteral("media"),
                 result.errors, true);
    if (media.contains(QStringLiteral("volume"))) {
        const auto volume = media.value(QStringLiteral("volume")).toDouble();
        if (!std::isfinite(volume) || volume < 0.0 || volume > 1.0)
            result.errors.append(QStringLiteral("media.volume deve estar entre 0 e 1."));
    }
    expectOneOf(media, QStringLiteral("repeatMode"),
                {QStringLiteral("off"), QStringLiteral("one"), QStringLiteral("all")},
                QStringLiteral("media"), result.errors);
    expectOneOf(media, QStringLiteral("imageFit"),
                {QStringLiteral("contain"), QStringLiteral("cover"),
                 QStringLiteral("stretch"), QStringLiteral("center")},
                QStringLiteral("media"), result.errors);
    expectOneOf(media, QStringLiteral("imageTransition"),
                {QStringLiteral("none"), QStringLiteral("fade")},
                QStringLiteral("media"), result.errors);
    if (media.contains(QStringLiteral("imageIntervalMs"))) {
        const auto interval = media.value(QStringLiteral("imageIntervalMs")).toInt();
        if (interval < 250 || interval > 3'600'000)
            result.errors.append(QStringLiteral("media.imageIntervalMs deve estar entre 250 e 3600000."));
    }

    const auto bible = section(profile, QStringLiteral("bible"), result.errors);
    rejectUnknown(bible,
                  {QStringLiteral("primaryTranslationId"),
                   QStringLiteral("secondaryTranslationId"),
                   QStringLiteral("tertiaryTranslationId")},
                  QStringLiteral("bible"), result.errors);
    for (const auto &key : {"primaryTranslationId", "secondaryTranslationId",
                            "tertiaryTranslationId"})
        expectType(bible, QString::fromLatin1(key), QMetaType::QString,
                   QStringLiteral("bible"), result.errors);

    const auto remote = section(profile, QStringLiteral("remote"), result.errors);
    rejectUnknown(remote, {QStringLiteral("interface"), QStringLiteral("port")},
                  QStringLiteral("remote"), result.errors);
    expectType(remote, QStringLiteral("interface"), QMetaType::QString,
               QStringLiteral("remote"), result.errors);
    expectNumber(remote, QStringLiteral("port"), QStringLiteral("remote"),
                 result.errors, true);
    if (remote.contains(QStringLiteral("port"))) {
        const auto port = remote.value(QStringLiteral("port")).toInt();
        if (port < 1024 || port > 65535)
            result.errors.append(QStringLiteral("remote.port deve estar entre 1024 e 65535."));
    }

    const auto library = section(profile, QStringLiteral("library"), result.errors);
    rejectUnknown(library, {QStringLiteral("mediaFolders"), QStringLiteral("favorites")},
                  QStringLiteral("library"), result.errors);
    expectStringList(library, QStringLiteral("mediaFolders"),
                     QStringLiteral("library"), result.errors);
    expectStringList(library, QStringLiteral("favorites"),
                     QStringLiteral("library"), result.errors);

    expectStringList(profile, QStringLiteral("outputs"),
                     QStringLiteral("profile"), result.errors);
    const auto shortcuts = section(profile, QStringLiteral("shortcuts"), result.errors);
    rejectUnknown(shortcuts,
                  {QStringLiteral("blackout"), QStringLiteral("next"),
                   QStringLiteral("previous"), QStringLiteral("stop"),
                   QStringLiteral("quickBible")},
                  QStringLiteral("shortcuts"), result.errors);
    QSet<QString> shortcutSequences;
    for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
        if (it.value().metaType().id() != QMetaType::QString) {
            result.errors.append(QStringLiteral("shortcuts.%1 deve ser um texto.").arg(it.key()));
            continue;
        }
        const auto sequence = it.value().toString().trimmed();
        if (sequence.isEmpty()) continue;
        if (sequence.size() > 64
            || QKeySequence::fromString(sequence, QKeySequence::PortableText).isEmpty()) {
            result.errors.append(QStringLiteral("Atalho inválido em shortcuts.%1.").arg(it.key()));
            continue;
        }
        const auto normalized = sequence.toCaseFolded();
        if (shortcutSequences.contains(normalized))
            result.errors.append(QStringLiteral("Atalho duplicado: %1").arg(sequence));
        shortcutSequences.insert(normalized);
    }
    const auto onboarding = section(profile, QStringLiteral("onboarding"), result.errors);
    rejectUnknown(onboarding, {QStringLiteral("completed"), QStringLiteral("skippedSteps")},
                  QStringLiteral("onboarding"), result.errors);
    expectType(onboarding, QStringLiteral("completed"), QMetaType::Bool,
               QStringLiteral("onboarding"), result.errors);
    expectStringList(onboarding, QStringLiteral("skippedSteps"),
                     QStringLiteral("onboarding"), result.errors);

    result.accepted = result.errors.isEmpty();
    if (!result.accepted) result.profile.clear();
    return result;
}

ConfigurationProfileResult ConfigurationProfileService::parse(const QByteArray &document)
{
    ConfigurationProfileResult result;
    if (document.size() > MaximumDocumentSize) {
        result.errors.append(QStringLiteral("O perfil excede o limite de 1 MiB."));
        return result;
    }
    QJsonParseError parseError;
    const auto json = QJsonDocument::fromJson(document, &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        result.errors.append(QStringLiteral("JSON inválido: %1").arg(parseError.errorString()));
        return result;
    }
    const auto root = json.object();
    if (root.value(QStringLiteral("documentType")).toString()
        != QStringLiteral("holyscreen.configuration")) {
        result.errors.append(QStringLiteral("Tipo de documento incompatível."));
        return result;
    }
    if (root.value(QStringLiteral("schemaVersion")).toInt(-1) != SchemaVersion) {
        result.errors.append(QStringLiteral("Versão de schema não suportada."));
        return result;
    }
    if (!root.value(QStringLiteral("profile")).isObject()) {
        result.errors.append(QStringLiteral("O campo profile deve ser um objeto."));
        return result;
    }
    return validate(root.value(QStringLiteral("profile")).toObject().toVariantMap());
}

QByteArray ConfigurationProfileService::serialize(const QVariantMap &profile, QStringList *errors)
{
    const auto validation = validate(profile);
    if (errors) *errors = validation.errors;
    if (!validation.accepted) return {};
    const QJsonObject root{
        {QStringLiteral("documentType"), QStringLiteral("holyscreen.configuration")},
        {QStringLiteral("schemaVersion"), SchemaVersion},
        {QStringLiteral("profile"), QJsonObject::fromVariantMap(profile)},
    };
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

} // namespace churchpresenter
