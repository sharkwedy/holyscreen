#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include <functional>

namespace churchpresenter {

struct UndoResult {
    bool success = false;
    QString label;
    QString error;
};

class UndoManager final : public QObject {
    Q_OBJECT

public:
    using Action = std::function<bool()>;

    explicit UndoManager(QObject *parent = nullptr);

    void setLimit(int limit);
    [[nodiscard]] int limit() const;
    bool record(QString label, Action undo, Action redo);
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    [[nodiscard]] QString undoLabel() const;
    [[nodiscard]] QString redoLabel() const;
    UndoResult undo();
    UndoResult redo();
    void clear();

signals:
    void stateChanged();

private:
    struct Entry {
        QString label;
        Action undo;
        Action redo;
    };

    int m_limit = 100;
    QVector<Entry> m_undoStack;
    QVector<Entry> m_redoStack;
};

} // namespace churchpresenter
