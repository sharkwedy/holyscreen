#pragma once

#include <QString>
#include <QStringList>

#include <optional>

namespace churchpresenter {

enum class OutputRole {
    Audience,
    Stage,
    Broadcast,
    Confidence,
    Custom,
};

//! Canonical wire/persistence name of \a role. Every enumerator has its own
//! name; no role is collapsed into another one.
[[nodiscard]] QString outputRoleName(OutputRole role);

//! Parses \a name case-insensitively and trimmed. Returns \c std::nullopt for
//! unknown values so callers decide explicitly what to do instead of silently
//! degrading the role.
[[nodiscard]] std::optional<OutputRole> outputRoleFromName(const QString &name);

//! Convenience for call sites that must keep working with legacy data.
[[nodiscard]] OutputRole outputRoleFromNameOr(const QString &name, OutputRole fallback);

[[nodiscard]] bool isOutputRoleName(const QString &name);

//! Canonical names in declaration order.
[[nodiscard]] QStringList outputRoleNames();

} // namespace churchpresenter
