#pragma once
#include <QString>
namespace churchpresenter {
class DataRecoveryService final{
public:explicit DataRecoveryService(QString dataDirectory);QString createBackup();bool scheduleRestore(const QString&source);bool applyPendingRestore();bool beginSession();bool endSession();bool recoveredFromCrash()const;QString recoverySnapshotPath()const;QString dataDirectory()const;
private:QString timestampedPath(const QString&prefix)const;QString m_dataDirectory;bool m_recovered=false;QString m_recoverySnapshot;};
}
