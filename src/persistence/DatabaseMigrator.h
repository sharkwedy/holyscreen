#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include <functional>

namespace churchpresenter {

struct MigrationResult {
    bool success = false;
    int previousVersion = 0;
    int currentVersion = 0;
    QString backupPath;
    QString error;
};

class DatabaseMigrator final {
public:
    using Migration = std::function<bool(QSqlDatabase &database, QString *error)>;
    using BackupCopier = std::function<bool(const QString &source, const QString &target)>;

    explicit DatabaseMigrator(QString databasePath, BackupCopier backupCopier = {});

    bool addMigration(int version, QString description, Migration migration);
    [[nodiscard]] MigrationResult migrate() const;

private:
    struct Entry {
        int version = 0;
        QString description;
        Migration migration;
    };

    QString m_databasePath;
    BackupCopier m_backupCopier;
    QVector<Entry> m_migrations;
};

} // namespace churchpresenter
