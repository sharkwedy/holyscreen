#pragma once

#include "persistence/DatabaseMigrator.h"

namespace churchpresenter {

class ApplicationDatabase final {
public:
    [[nodiscard]] static MigrationResult migrate(const QString &databasePath);
};

} // namespace churchpresenter
