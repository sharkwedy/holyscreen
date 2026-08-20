#include "app/DataRecoveryService.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSaveFile>
namespace churchpresenter {
DataRecoveryService::DataRecoveryService(QString d):m_dataDirectory(std::move(d)){QDir().mkpath(m_dataDirectory);QDir().mkpath(m_dataDirectory+"/backups");}
QString DataRecoveryService::timestampedPath(const QString&p)const{return m_dataDirectory+"/backups/"+p+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")+".db";}
QString DataRecoveryService::createBackup(){const auto source=m_dataDirectory+"/presenter.db";if(!QFile::exists(source))return{};const auto target=timestampedPath("holyscreen-");return QFile::copy(source,target)?target:QString{};}
bool DataRecoveryService::scheduleRestore(const QString&source){QFile input(source);if(!input.open(QIODevice::ReadOnly)||input.read(16)!=QByteArray("SQLite format 3\0",16))return false;input.close();const auto target=m_dataDirectory+"/presenter.db.restore-pending";QFile::remove(target);return QFile::copy(source,target);}
bool DataRecoveryService::applyPendingRestore(){const auto pending=m_dataDirectory+"/presenter.db.restore-pending";if(!QFile::exists(pending))return true;const auto current=m_dataDirectory+"/presenter.db";if(QFile::exists(current)){const auto safety=timestampedPath("pre-restore-");if(!QFile::copy(current,safety))return false;}const auto staged=m_dataDirectory+"/presenter.db.restore-staged";QFile::remove(staged);if(!QFile::copy(pending,staged))return false;QFile::remove(current);if(!QFile::rename(staged,current))return false;return QFile::remove(pending);}
bool DataRecoveryService::beginSession(){const auto marker=m_dataDirectory+"/session.lock";if(QFile::exists(marker)){m_recovered=true;const auto db=m_dataDirectory+"/presenter.db";if(QFile::exists(db)){m_recoverySnapshot=timestampedPath("crash-recovery-");QFile::copy(db,m_recoverySnapshot);}}QSaveFile f(marker);if(!f.open(QIODevice::WriteOnly))return false;f.write(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toUtf8());return f.commit();}
bool DataRecoveryService::endSession(){const auto marker=m_dataDirectory+"/session.lock";return !QFile::exists(marker)||QFile::remove(marker);}
bool DataRecoveryService::recoveredFromCrash()const{return m_recovered;}QString DataRecoveryService::recoverySnapshotPath()const{return m_recoverySnapshot;}QString DataRecoveryService::dataDirectory()const{return m_dataDirectory;}
}
