#pragma once

#include <QList>
#include <QString>

namespace churchpresenter {

struct CommandDescriptor {
    QString type;
    bool remoteAllowed = false;
    bool undoable = false;
};

class CommandCatalog final {
public:
    [[nodiscard]] static QList<CommandDescriptor> descriptors();
    [[nodiscard]] static bool contains(const QString &type);
    [[nodiscard]] static bool isRemoteAllowed(const QString &type);
};

} // namespace churchpresenter
