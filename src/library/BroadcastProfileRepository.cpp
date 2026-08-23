#include "library/BroadcastProfileRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace churchpresenter {
namespace {

constexpr auto SelectColumns =
    "screen_fingerprint,background_mode,chroma_color,safe_area_left,safe_area_top,"
    "safe_area_right,safe_area_bottom,aspect_preset,show_clock,show_lower_third,"
    "show_alerts,show_audience_message";

BroadcastProfile readProfile(const QSqlQuery &query)
{
    BroadcastProfile profile;
    profile.screenFingerprint = query.value(0).toString();
    if (const auto mode = broadcastBackgroundModeFromName(query.value(1).toString())) {
        profile.backgroundMode = *mode;
    } else {
        qWarning() << "Unknown persisted broadcast background mode, using chroma:"
                   << query.value(1).toString();
    }
    profile.chromaColor = query.value(2).toString();
    profile.safeArea = QMarginsF(query.value(3).toDouble(), query.value(4).toDouble(),
                                 query.value(5).toDouble(), query.value(6).toDouble());
    if (const auto preset = broadcastAspectPresetFromName(query.value(7).toString())) {
        profile.aspectPreset = *preset;
    } else {
        qWarning() << "Unknown persisted broadcast aspect preset, using 16:9:"
                   << query.value(7).toString();
    }
    profile.showClock = query.value(8).toBool();
    profile.showLowerThird = query.value(9).toBool();
    profile.showAlerts = query.value(10).toBool();
    profile.showAudienceMessage = query.value(11).toBool();
    return normalizedBroadcastProfile(profile);
}

} // namespace

BroadcastProfileRepository::BroadcastProfileRepository(QString databasePath)
    : m_databasePath(std::move(databasePath))
{
}

BroadcastProfileRepository::~BroadcastProfileRepository()
{
    if (m_connectionName.isEmpty()) return;
    {
        auto database = QSqlDatabase::database(m_connectionName, false);
        if (database.isValid()) database.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool BroadcastProfileRepository::open()
{
    if (!m_connectionName.isEmpty()
        && QSqlDatabase::database(m_connectionName, false).isOpen()) {
        return true;
    }
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) return false;
    m_connectionName = QStringLiteral("holyscreen-broadcast-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(m_databasePath);
    if (!database.open()) {
        qWarning() << "Could not open broadcast profile database:" << database.lastError().text();
        database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }
    QSqlQuery query(database);
    return query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS output_broadcast_profiles("
        "screen_fingerprint TEXT PRIMARY KEY NOT NULL,"
        "background_mode TEXT NOT NULL DEFAULT 'chroma',"
        "chroma_color TEXT NOT NULL DEFAULT '#00b140',"
        "safe_area_left REAL NOT NULL DEFAULT 5,"
        "safe_area_top REAL NOT NULL DEFAULT 5,"
        "safe_area_right REAL NOT NULL DEFAULT 5,"
        "safe_area_bottom REAL NOT NULL DEFAULT 5,"
        "aspect_preset TEXT NOT NULL DEFAULT '16:9',"
        "show_clock INTEGER NOT NULL DEFAULT 0,"
        "show_lower_third INTEGER NOT NULL DEFAULT 1,"
        "show_alerts INTEGER NOT NULL DEFAULT 1,"
        "show_audience_message INTEGER NOT NULL DEFAULT 1,"
        "updated_at TEXT NOT NULL DEFAULT '')"));
}

bool BroadcastProfileRepository::save(const BroadcastProfile &profile)
{
    if (m_connectionName.isEmpty() && !open()) return false;
    const auto normalized = normalizedBroadcastProfile(profile);
    if (normalized.screenFingerprint.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "INSERT INTO output_broadcast_profiles(screen_fingerprint,background_mode,chroma_color,"
        "safe_area_left,safe_area_top,safe_area_right,safe_area_bottom,aspect_preset,show_clock,"
        "show_lower_third,show_alerts,show_audience_message,updated_at) "
        "VALUES(:fingerprint,:mode,:chroma,:left,:top,:right,:bottom,:preset,:clock,:lower,"
        ":alerts,:audience,:updated) "
        "ON CONFLICT(screen_fingerprint) DO UPDATE SET background_mode=excluded.background_mode,"
        "chroma_color=excluded.chroma_color,safe_area_left=excluded.safe_area_left,"
        "safe_area_top=excluded.safe_area_top,safe_area_right=excluded.safe_area_right,"
        "safe_area_bottom=excluded.safe_area_bottom,aspect_preset=excluded.aspect_preset,"
        "show_clock=excluded.show_clock,show_lower_third=excluded.show_lower_third,"
        "show_alerts=excluded.show_alerts,show_audience_message=excluded.show_audience_message,"
        "updated_at=excluded.updated_at"));
    query.bindValue(QStringLiteral(":fingerprint"), normalized.screenFingerprint);
    query.bindValue(QStringLiteral(":mode"), broadcastBackgroundModeName(normalized.backgroundMode));
    query.bindValue(QStringLiteral(":chroma"), normalized.chromaColor);
    query.bindValue(QStringLiteral(":left"), normalized.safeArea.left());
    query.bindValue(QStringLiteral(":top"), normalized.safeArea.top());
    query.bindValue(QStringLiteral(":right"), normalized.safeArea.right());
    query.bindValue(QStringLiteral(":bottom"), normalized.safeArea.bottom());
    query.bindValue(QStringLiteral(":preset"), broadcastAspectPresetName(normalized.aspectPreset));
    query.bindValue(QStringLiteral(":clock"), normalized.showClock ? 1 : 0);
    query.bindValue(QStringLiteral(":lower"), normalized.showLowerThird ? 1 : 0);
    query.bindValue(QStringLiteral(":alerts"), normalized.showAlerts ? 1 : 0);
    query.bindValue(QStringLiteral(":audience"), normalized.showAudienceMessage ? 1 : 0);
    query.bindValue(QStringLiteral(":updated"),
                    QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        qWarning() << "Could not save broadcast profile:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<BroadcastProfile> BroadcastProfileRepository::find(
    const QString &screenFingerprint) const
{
    if (m_connectionName.isEmpty()) return std::nullopt;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral("SELECT %1 FROM output_broadcast_profiles "
                                 "WHERE screen_fingerprint=:fingerprint")
                      .arg(QLatin1StringView(SelectColumns)));
    query.bindValue(QStringLiteral(":fingerprint"), screenFingerprint);
    if (!query.exec() || !query.next()) return std::nullopt;
    return readProfile(query);
}

BroadcastProfile BroadcastProfileRepository::findOrDefault(const QString &screenFingerprint) const
{
    if (const auto stored = find(screenFingerprint)) return *stored;
    BroadcastProfile profile;
    profile.screenFingerprint = screenFingerprint;
    return profile;
}

QVector<BroadcastProfile> BroadcastProfileRepository::all() const
{
    QVector<BroadcastProfile> profiles;
    if (m_connectionName.isEmpty()) return profiles;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    if (!query.exec(QStringLiteral("SELECT %1 FROM output_broadcast_profiles "
                                   "ORDER BY screen_fingerprint")
                        .arg(QLatin1StringView(SelectColumns)))) {
        return profiles;
    }
    while (query.next()) profiles.append(readProfile(query));
    return profiles;
}

bool BroadcastProfileRepository::remove(const QString &screenFingerprint)
{
    if (m_connectionName.isEmpty()) return false;
    QSqlQuery query(QSqlDatabase::database(m_connectionName, false));
    query.prepare(QStringLiteral(
        "DELETE FROM output_broadcast_profiles WHERE screen_fingerprint=:fingerprint"));
    query.bindValue(QStringLiteral(":fingerprint"), screenFingerprint);
    return query.exec();
}

} // namespace churchpresenter
