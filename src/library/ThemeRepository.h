#pragma once
#include "presentation/ThemeTypes.h"
#include <QVector>
namespace churchpresenter {
class ThemeRepository final {
public:
    explicit ThemeRepository(QString databasePath); ~ThemeRepository();
    bool open(); QVector<Theme> themes() const; Theme theme(const QString &id) const;
    QString save(Theme theme); bool remove(const QString &id);
private: QString m_databasePath; QString m_connectionName;
};
}
