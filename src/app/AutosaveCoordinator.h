#pragma once

#include <QObject>
#include <QTimer>

#include <functional>

namespace churchpresenter {

class AutosaveCoordinator final : public QObject {
    Q_OBJECT

public:
    using SaveAction = std::function<bool()>;

    explicit AutosaveCoordinator(SaveAction saveAction, QObject *parent = nullptr);

    void setIntervalMs(int intervalMs);
    [[nodiscard]] int intervalMs() const;
    [[nodiscard]] bool dirty() const;
    void markDirty();
    bool flush();
    void discard();

signals:
    void dirtyChanged(bool dirty);
    void saved();
    void saveFailed(const QString &message);

private:
    SaveAction m_saveAction;
    QTimer m_timer;
    bool m_dirty = false;
};

} // namespace churchpresenter
