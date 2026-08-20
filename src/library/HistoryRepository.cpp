#include "library/HistoryRepository.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>
namespace churchpresenter {
HistoryRepository::HistoryRepository(QString p):m_databasePath(std::move(p)){}
HistoryRepository::~HistoryRepository(){if(m_connectionName.isEmpty())return;{auto d=QSqlDatabase::database(m_connectionName,false);if(d.isValid())d.close();}QSqlDatabase::removeDatabase(m_connectionName);}
bool HistoryRepository::open(){if(!m_connectionName.isEmpty()&&QSqlDatabase::database(m_connectionName,false).isOpen())return true;if(!QDir().mkpath(QFileInfo(m_databasePath).absolutePath()))return false;m_connectionName="holyscreen-history-"+QUuid::createUuid().toString(QUuid::WithoutBraces);auto d=QSqlDatabase::addDatabase("QSQLITE",m_connectionName);d.setDatabaseName(m_databasePath);if(!d.open())return false;QSqlQuery q(d);return q.exec("CREATE TABLE IF NOT EXISTS history(id TEXT PRIMARY KEY,item_type TEXT NOT NULL,reference_id TEXT NOT NULL,title TEXT NOT NULL,event_id TEXT NOT NULL DEFAULT '',executed_at TEXT NOT NULL)");}
bool HistoryRepository::record(HistoryEntry e){if(m_connectionName.isEmpty()&&!open())return false;if(e.id.isEmpty())e.id=QUuid::createUuid().toString(QUuid::WithoutBraces);if(e.eventId.isNull())e.eventId="";if(e.executedAt.isEmpty())e.executedAt=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);QSqlQuery q(QSqlDatabase::database(m_connectionName,false));q.prepare("INSERT INTO history VALUES(:id,:type,:ref,:title,:event,:at)");q.bindValue(":id",e.id);q.bindValue(":type",e.itemType);q.bindValue(":ref",e.referenceId);q.bindValue(":title",e.title);q.bindValue(":event",e.eventId);q.bindValue(":at",e.executedAt);return q.exec();}
QVector<HistoryEntry>HistoryRepository::entries(int limit)const{QVector<HistoryEntry>r;QSqlQuery q(QSqlDatabase::database(m_connectionName,false));q.prepare("SELECT id,item_type,reference_id,title,event_id,executed_at FROM history ORDER BY executed_at DESC,id DESC LIMIT :limit");q.bindValue(":limit",limit);if(q.exec())while(q.next())r.append({q.value(0).toString(),q.value(1).toString(),q.value(2).toString(),q.value(3).toString(),q.value(4).toString(),q.value(5).toString()});return r;}
HistoryReport HistoryRepository::report()const{HistoryReport r;QSqlQuery q(QSqlDatabase::database(m_connectionName,false));if(q.exec("SELECT item_type,COUNT(*) FROM history GROUP BY item_type"))while(q.next()){const auto count=q.value(1).toInt();r.byType[q.value(0).toString()]=count;r.totalExecutions+=count;}if(q.exec("SELECT title,COUNT(*) AS n FROM history GROUP BY reference_id,title ORDER BY n DESC,title LIMIT 1")&&q.next())r.mostExecutedTitle=q.value(0).toString();return r;}
bool HistoryRepository::clear(){QSqlQuery q(QSqlDatabase::database(m_connectionName,false));return q.exec("DELETE FROM history");}
}
