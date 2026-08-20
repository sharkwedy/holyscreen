#pragma once
#include <QHash>
#include <QString>
#include <QVector>
namespace churchpresenter {
struct HistoryEntry{QString id;QString itemType;QString referenceId;QString title;QString eventId;QString executedAt;};
struct HistoryReport{int totalExecutions=0;QHash<QString,int>byType;QString mostExecutedTitle;};
class HistoryRepository final{
public:explicit HistoryRepository(QString databasePath);~HistoryRepository();bool open();bool record(HistoryEntry entry);QVector<HistoryEntry>entries(int limit=500)const;HistoryReport report()const;bool clear();
private:QString m_databasePath;QString m_connectionName;};
}
