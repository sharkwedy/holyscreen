#pragma once

#include "presentation/PresentationTypes.h"

#include <QString>

namespace churchpresenter {

class PresentationRepository final {
public:
    explicit PresentationRepository(QString databasePath);
    ~PresentationRepository();
    bool open();
    [[nodiscard]] QVector<Presentation> presentations(PresentationType type) const;
    [[nodiscard]] Presentation presentation(const QString &id) const;
    QString save(Presentation item);
    bool remove(const QString &id);

private:
    QString m_databasePath;
    QString m_connectionName;
};

} // namespace churchpresenter
