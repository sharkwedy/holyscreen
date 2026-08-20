#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

namespace churchpresenter {

struct DomainEvent {
    QString type;
    QVariantMap payload;
    QDateTime occurredAt;
    QString correlationId;
};

} // namespace churchpresenter

Q_DECLARE_METATYPE(churchpresenter::DomainEvent)
