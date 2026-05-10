#pragma once

#include "EditorSceneSnapshot.h"

#include <deque>

class UndoRedoManager
{
public:
    explicit UndoRedoManager(size_t maxSnapshots = 12);

    void pushUndo(EditorSceneSnapshot snapshot);

    bool canUndo() const;
    bool canRedo() const;

    bool undo(EditorSceneSnapshot currentSnapshot, EditorSceneSnapshot& outSnapshotToRestore);
    bool redo(EditorSceneSnapshot currentSnapshot, EditorSceneSnapshot& outSnapshotToRestore);

    void clear();

private:
    size_t m_maxSnapshots;
    std::deque<EditorSceneSnapshot> m_undoStack;
    std::deque<EditorSceneSnapshot> m_redoStack;

    void trim(std::deque<EditorSceneSnapshot>& stack);
};