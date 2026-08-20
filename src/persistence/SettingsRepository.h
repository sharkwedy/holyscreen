#pragma once

#include <QString>
#include <QVariant>
#include <QObject>

namespace churchpresenter {

class SettingsRepository : public QObject {
    Q_OBJECT

public:
    explicit SettingsRepository(const QString &databasePath, QObject *parent = nullptr);
    ~SettingsRepository() override;

    bool open();
    bool setValue(const QString &key, const QVariant &value);
    [[nodiscard]] QVariant value(const QString &key, const QVariant &fallback = {}) const;

signals:
    void valueChanged(const QString &key, const QVariant &value);

private:
    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
