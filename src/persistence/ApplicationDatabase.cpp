#include "persistence/ApplicationDatabase.h"

#include <QSqlError>
#include <QSqlQuery>

namespace churchpresenter {

MigrationResult ApplicationDatabase::migrate(const QString &databasePath)
{
    DatabaseMigrator migrator(databasePath);
    migrator.addMigration(1, QStringLiteral("baseline HolyScreen 0.10"),
                          [](QSqlDatabase &database, QString *error) {
        const QStringList statements{
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS settings("
                "key TEXT PRIMARY KEY NOT NULL,value BLOB NOT NULL)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS media_items("
                "id TEXT PRIMARY KEY NOT NULL,type INTEGER NOT NULL,title TEXT NOT NULL,"
                "path TEXT NOT NULL,duration_ms INTEGER NOT NULL DEFAULT 0,"
                "artist TEXT NOT NULL DEFAULT '',album TEXT NOT NULL DEFAULT '',"
                "position INTEGER NOT NULL,playlist_position INTEGER NOT NULL DEFAULT 0,"
                "UNIQUE(type,path))"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS presentations("
                "id TEXT PRIMARY KEY NOT NULL,type INTEGER NOT NULL,title TEXT NOT NULL,"
                "author TEXT NOT NULL DEFAULT '',default_theme TEXT NOT NULL DEFAULT '')"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS slides("
                "id TEXT PRIMARY KEY NOT NULL,presentation_id TEXT NOT NULL,"
                "label TEXT NOT NULL DEFAULT '',text TEXT NOT NULL DEFAULT '',"
                "position INTEGER NOT NULL,theme_override TEXT NOT NULL DEFAULT '',"
                "FOREIGN KEY(presentation_id) REFERENCES presentations(id) ON DELETE CASCADE)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS presentation_sequence("
                "presentation_id TEXT NOT NULL,slide_id TEXT NOT NULL,position INTEGER NOT NULL,"
                "PRIMARY KEY(presentation_id,position),"
                "FOREIGN KEY(presentation_id) REFERENCES presentations(id) ON DELETE CASCADE)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS themes("
                "id TEXT PRIMARY KEY,name TEXT NOT NULL,background_type INTEGER NOT NULL,"
                "background_color TEXT NOT NULL,background_image TEXT NOT NULL,"
                "font_family TEXT NOT NULL,font_size INTEGER NOT NULL,"
                "minimum_font_size INTEGER NOT NULL,font_weight INTEGER NOT NULL,"
                "text_color TEXT NOT NULL,horizontal_alignment TEXT NOT NULL,"
                "vertical_alignment TEXT NOT NULL,line_spacing INTEGER NOT NULL,"
                "margin INTEGER NOT NULL,outline INTEGER NOT NULL,outline_color TEXT NOT NULL,"
                "shadow INTEGER NOT NULL,shadow_color TEXT NOT NULL,transition_name TEXT NOT NULL)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS events("
                "id TEXT PRIMARY KEY,title TEXT NOT NULL,scheduled_at TEXT NOT NULL DEFAULT '')"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS playlist_items("
                "id TEXT PRIMARY KEY,event_id TEXT NOT NULL,item_type INTEGER NOT NULL,"
                "reference_id TEXT NOT NULL,title TEXT NOT NULL,duration_ms INTEGER NOT NULL DEFAULT 0,"
                "position INTEGER NOT NULL,"
                "FOREIGN KEY(event_id) REFERENCES events(id) ON DELETE CASCADE)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS history("
                "id TEXT PRIMARY KEY,item_type TEXT NOT NULL,reference_id TEXT NOT NULL,"
                "title TEXT NOT NULL,event_id TEXT NOT NULL DEFAULT '',executed_at TEXT NOT NULL)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS bible_translations("
                "id TEXT PRIMARY KEY NOT NULL,name TEXT NOT NULL,abbreviation TEXT NOT NULL,"
                "language TEXT NOT NULL)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS bible_verses("
                "translation_id TEXT NOT NULL,book INTEGER NOT NULL,chapter INTEGER NOT NULL,"
                "verse INTEGER NOT NULL,text TEXT NOT NULL,"
                "PRIMARY KEY(translation_id,book,chapter,verse),"
                "FOREIGN KEY(translation_id) REFERENCES bible_translations(id) ON DELETE CASCADE)"),
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS bible_verses_reference_idx "
                "ON bible_verses(translation_id,book,chapter,verse)"),
        };

        QSqlQuery query(database);
        for (const auto &statement : statements) {
            if (!query.exec(statement)) {
                if (error) *error = query.lastError().text();
                return false;
            }
        }

        bool hasPlaylistPosition = false;
        if (!query.exec(QStringLiteral("PRAGMA table_info(media_items)"))) {
            if (error) *error = query.lastError().text();
            return false;
        }
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("playlist_position")) {
                hasPlaylistPosition = true;
                break;
            }
        }
        if (!hasPlaylistPosition
            && !query.exec(QStringLiteral(
                "ALTER TABLE media_items ADD COLUMN playlist_position INTEGER NOT NULL DEFAULT 0"))) {
            if (error) *error = query.lastError().text();
            return false;
        }
        if (!hasPlaylistPosition
            && !query.exec(QStringLiteral(
                "WITH ordered AS (SELECT id,ROW_NUMBER() OVER (ORDER BY rowid)-1 AS pos FROM media_items) "
                "UPDATE media_items SET playlist_position="
                "(SELECT pos FROM ordered WHERE ordered.id=media_items.id)"))) {
            if (error) *error = query.lastError().text();
            return false;
        }
        return true;
    });
    migrator.addMigration(2, QStringLiteral("Bible import source metadata"),
                          [](QSqlDatabase &database, QString *error) {
        QSqlQuery query(database);
        const bool ok = query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bible_translation_sources("
            "translation_id TEXT PRIMARY KEY NOT NULL,"
            "source_kind TEXT NOT NULL,source_location TEXT NOT NULL,"
            "source_revision TEXT NOT NULL DEFAULT '',license TEXT NOT NULL,"
            "publisher TEXT NOT NULL DEFAULT '',source_name TEXT NOT NULL DEFAULT '',"
            "source_code TEXT NOT NULL DEFAULT '',scope TEXT NOT NULL DEFAULT '',"
            "imported_at TEXT NOT NULL,content_hash TEXT NOT NULL DEFAULT '',"
            "FOREIGN KEY(translation_id) REFERENCES bible_translations(id) ON DELETE CASCADE)"));
        if (!ok && error) *error = query.lastError().text();
        return ok;
    });
    return migrator.migrate();
}

} // namespace churchpresenter
