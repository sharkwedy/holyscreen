#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace churchpresenter {

struct Command {
    QString id;
    QString type;
    QVariantMap payload;
    QString source;
    QDateTime issuedAt;
};

struct CommandResult {
    bool accepted = false;
    QString errorCode;
    QString message;
    quint64 stateRevision = 0;
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::Command)
Q_DECLARE_METATYPE(churchpresenter::CommandResult)
