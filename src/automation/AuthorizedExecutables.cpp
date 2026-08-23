#include "automation/AuthorizedExecutables.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace churchpresenter {
namespace {

QString canonical(const QString &path)
{
    const QFileInfo info(path);
    // canonicalFilePath resolve symlinks e `..`; caminho inexistente devolve
    // string vazia, o que já reprova a autorização.
    return info.canonicalFilePath();
}

void fail(QString *errorCode, QString *message, const QString &code, const QString &text)
{
    if (errorCode) *errorCode = code;
    if (message) *message = text;
}

} // namespace

void AuthorizedExecutables::setEnabled(bool enabled) { m_enabled = enabled; }
bool AuthorizedExecutables::isEnabled() const { return m_enabled; }

bool AuthorizedExecutables::authorize(const QString &path, const QString &label, QString *error)
{
    const auto resolved = canonical(path);
    if (resolved.isEmpty()) {
        if (error) *error = QStringLiteral("O caminho não existe.");
        return false;
    }
    const QFileInfo info(resolved);
    if (!info.isFile()) {
        if (error) *error = QStringLiteral("O caminho não é um arquivo.");
        return false;
    }
    if (!info.isExecutable()) {
        if (error) *error = QStringLiteral("O arquivo não é executável.");
        return false;
    }
    if (isAuthorized(resolved)) return true;
    m_entries.append(Entry{.canonicalPath = resolved,
                           .label = label.trimmed().isEmpty() ? info.fileName() : label.trimmed(),
                           .authorizedAt = QDateTime::currentDateTimeUtc()});
    return true;
}

bool AuthorizedExecutables::revoke(const QString &canonicalPath)
{
    return m_entries.removeIf([&canonicalPath](const Entry &entry) {
        return entry.canonicalPath == canonicalPath;
    }) > 0;
}

QList<AuthorizedExecutables::Entry> AuthorizedExecutables::entries() const { return m_entries; }

void AuthorizedExecutables::restore(const QList<Entry> &entries) { m_entries = entries; }

bool AuthorizedExecutables::isAuthorized(const QString &path) const
{
    const auto resolved = path.isEmpty() ? QString{} : canonical(path);
    if (resolved.isEmpty()) return false;
    return std::any_of(m_entries.cbegin(), m_entries.cend(), [&resolved](const Entry &entry) {
        return entry.canonicalPath == resolved;
    });
}

std::optional<ProcessRequest> AuthorizedExecutables::validate(const QVariantMap &parameters,
                                                              QString *errorCode,
                                                              QString *message) const
{
    if (!m_enabled) {
        fail(errorCode, message, QStringLiteral("process_disabled"),
             QStringLiteral("Processos externos estão desativados nas configurações."));
        return std::nullopt;
    }

    const auto requested = parameters.value(QStringLiteral("executable")).toString().trimmed();
    if (requested.isEmpty()) {
        fail(errorCode, message, QStringLiteral("invalid_payload"),
             QStringLiteral("Informe o executável."));
        return std::nullopt;
    }
    if (QFileInfo(requested).isRelative()) {
        fail(errorCode, message, QStringLiteral("invalid_payload"),
             QStringLiteral("Use o caminho absoluto do executável."));
        return std::nullopt;
    }
    const auto resolved = canonical(requested);
    if (resolved.isEmpty() || !isAuthorized(resolved)) {
        fail(errorCode, message, QStringLiteral("process_not_authorized"),
             QStringLiteral("O executável não está na lista autorizada."));
        return std::nullopt;
    }

    const auto argumentsValue = parameters.value(QStringLiteral("arguments"));
    if (argumentsValue.isValid() && argumentsValue.typeId() != QMetaType::QVariantList
        && argumentsValue.typeId() != QMetaType::QStringList) {
        // Uma string única viraria concatenação e abriria espaço para shell.
        fail(errorCode, message, QStringLiteral("invalid_payload"),
             QStringLiteral("Os argumentos precisam ser uma lista."));
        return std::nullopt;
    }
    QStringList arguments;
    for (const auto &argument : argumentsValue.toList()) {
        arguments.append(argument.toString());
    }
    if (arguments.size() > MaximumArguments) {
        fail(errorCode, message, QStringLiteral("invalid_payload"),
             QStringLiteral("No máximo %1 argumentos.").arg(MaximumArguments));
        return std::nullopt;
    }

    auto workingDirectory = parameters.value(QStringLiteral("workingDirectory"))
                                .toString().trimmed();
    if (!workingDirectory.isEmpty()) {
        const QFileInfo directory(workingDirectory);
        if (!directory.isAbsolute() || !directory.isDir()) {
            fail(errorCode, message, QStringLiteral("invalid_payload"),
                 QStringLiteral("O diretório de trabalho precisa ser uma pasta absoluta."));
            return std::nullopt;
        }
        workingDirectory = directory.canonicalFilePath();
    } else {
        workingDirectory = QFileInfo(resolved).absolutePath();
    }

    // Ambiente mínimo: só as chaves declaradas explicitamente na ação.
    QVariantMap environment;
    const auto declared = parameters.value(QStringLiteral("environment")).toMap();
    for (auto it = declared.cbegin(); it != declared.cend(); ++it) {
        if (it.key().trimmed().isEmpty()) continue;
        environment.insert(it.key(), it.value().toString());
    }

    int timeoutMs = parameters.value(QStringLiteral("timeoutMs"), 5000).toInt();
    timeoutMs = std::clamp(timeoutMs, MinimumTimeoutMs, MaximumTimeoutMs);

    return ProcessRequest{
        .executable = resolved,
        .arguments = arguments,
        .workingDirectory = workingDirectory,
        .environment = environment,
        .timeoutMs = timeoutMs,
        .maximumOutputBytes = MaximumOutputBytes,
    };
}

} // namespace churchpresenter
