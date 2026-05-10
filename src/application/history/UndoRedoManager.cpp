#include "UndoRedoManager.h"

UndoRedoManager::UndoRedoManager(size_t maxSnapshots)
    : m_maxSnapshots(maxSnapshots)
{
}

void UndoRedoManager::pushUndo(EditorSceneSnapshot snapshot)
{
    m_undoStack.push_back(std::move(snapshot));
    trim(m_undoStack);
    m_redoStack.clear();
}

bool UndoRedoManager::canUndo() const
{
    return !m_undoStack.empty();
}

bool UndoRedoManager::canRedo() const
{
    return !m_redoStack.empty();
}

bool UndoRedoManager::undo(EditorSceneSnapshot currentSnapshot, EditorSceneSnapshot& outSnapshotToRestore)
{
    if (m_undoStack.empty())
    {
        return false;
    }

    m_redoStack.push_back(std::move(currentSnapshot));
    trim(m_redoStack);

    outSnapshotToRestore = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    return true;
}

bool UndoRedoManager::redo(EditorSceneSnapshot currentSnapshot, EditorSceneSnapshot& outSnapshotToRestore)
{
    if (m_redoStack.empty())
    {
        return false;
    }

    m_undoStack.push_back(std::move(currentSnapshot));
    trim(m_undoStack);

    outSnapshotToRestore = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    return true;
}

void UndoRedoManager::clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

void UndoRedoManager::trim(std::deque<EditorSceneSnapshot>& stack)
{
    while (stack.size() > m_maxSnapshots)
    {
        stack.pop_front();
    }
}