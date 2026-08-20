#include "core/UndoManager.h"

#include <algorithm>

namespace churchpresenter {

UndoManager::UndoManager(QObject *parent)
    : QObject(parent)
{
}

void UndoManager::setLimit(int limit)
{
    m_limit = std::max(1, limit);
    while (m_undoStack.size() > m_limit) m_undoStack.removeFirst();
    while (m_redoStack.size() > m_limit) m_redoStack.removeFirst();
    emit stateChanged();
}
int UndoManager::limit() const { return m_limit; }
bool UndoManager::record(QString label, Action undo, Action redo)
{
    label = label.trimmed();
    if (label.isEmpty() || !undo || !redo) return false;
    m_undoStack.append({std::move(label), std::move(undo), std::move(redo)});
    while (m_undoStack.size() > m_limit) m_undoStack.removeFirst();
    m_redoStack.clear();
    emit stateChanged();
    return true;
}
bool UndoManager::canUndo() const { return !m_undoStack.isEmpty(); }
bool UndoManager::canRedo() const { return !m_redoStack.isEmpty(); }
QString UndoManager::undoLabel() const
{
    return canUndo() ? m_undoStack.constLast().label : QString{};
}
QString UndoManager::redoLabel() const
{
    return canRedo() ? m_redoStack.constLast().label : QString{};
}

UndoResult UndoManager::undo()
{
    if (!canUndo()) return {.error = QStringLiteral("Nenhuma ação para desfazer.")};
    auto entry = m_undoStack.takeLast();
    bool success = false;
    try {
        success = entry.undo();
    } catch (...) {
        success = false;
    }
    if (!success) {
        m_undoStack.append(std::move(entry));
        return {.error = QStringLiteral("Não foi possível desfazer a ação.")};
    }
    const auto label = entry.label;
    m_redoStack.append(std::move(entry));
    emit stateChanged();
    return {.success = true, .label = label};
}

UndoResult UndoManager::redo()
{
    if (!canRedo()) return {.error = QStringLiteral("Nenhuma ação para refazer.")};
    auto entry = m_redoStack.takeLast();
    bool success = false;
    try {
        success = entry.redo();
    } catch (...) {
        success = false;
    }
    if (!success) {
        m_redoStack.append(std::move(entry));
        return {.error = QStringLiteral("Não foi possível refazer a ação.")};
    }
    const auto label = entry.label;
    m_undoStack.append(std::move(entry));
    emit stateChanged();
    return {.success = true, .label = label};
}

void UndoManager::clear()
{
    if (m_undoStack.isEmpty() && m_redoStack.isEmpty()) return;
    m_undoStack.clear();
    m_redoStack.clear();
    emit stateChanged();
}

} // namespace churchpresenter
