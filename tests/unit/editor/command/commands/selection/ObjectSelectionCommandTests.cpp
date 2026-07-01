/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/selection/ClearObjectSelectionCommand.h"
#include "editor/command/selection/ObjectSelectionSnapshot.h"
#include "editor/command/selection/SelectObjectCommand.h"

namespace {

[[nodiscard]] bool has_all_flags(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flags)
{
    return (mask & flags) == flags;
}

} // namespace

namespace locus::tests {

TestResult run_object_selection_command_tests()
{
    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    const editor::SceneNodeId first = editor.scene().create_empty("First");
    const editor::SceneNodeId second = editor.scene().create_empty("Second");
    const editor::SceneNodeId locked = editor.scene().create_empty("Locked");
    editor.scene().find_node(locked)->metadata().locked = true;

    editor.selection().objects().set({ first, second }, first);
    editor.selection().objects().set_hovered(second);

    editor::ObjectSelectionSnapshot snapshot;
    if (snapshot.is_valid()) {
        return TestResult::fail("ObjectSelectionSnapshot should start invalid");
    }

    snapshot.capture(editor.selection());
    editor.selection().objects().clear();
    snapshot.restore(editor.selection());

    if (!snapshot.is_valid() ||
        editor.selection().objects().active() != first ||
        editor.selection().objects().hovered() != second ||
        !editor.selection().objects().contains(second)) {
        return TestResult::fail("ObjectSelectionSnapshot should capture and restore object selection");
    }

    editor::SelectObjectCommand invalidSelect{ editor::SceneNodeId{} };
    if (invalidSelect.execute(dispatcher.context())) {
        return TestResult::fail("SelectObjectCommand should reject invalid object ids");
    }

    editor::SelectObjectCommand lockedSelect{ locked };
    if (lockedSelect.execute(dispatcher.context())) {
        return TestResult::fail("SelectObjectCommand should reject unselectable objects");
    }

    editor::SelectObjectCommand selectSecond{ second };
    if (selectSecond.name() != "Select Object") {
        return TestResult::fail("SelectObjectCommand should expose a stable command name");
    }

    editor.clear_dirty();
    const editor::CommandResult selectResult = dispatcher.execute(selectSecond);
    if (!selectResult ||
        editor.selection().objects().size() != 1 ||
        editor.selection().objects().active() != second ||
        !editor.selection().objects().contains(second) ||
        !has_all_flags(
            selectResult.dirtyFlags,
            editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking)) {
        return TestResult::fail("SelectObjectCommand should select one valid object and report dirty flags");
    }

    const editor::CommandResult selectUndo = dispatcher.undo(selectSecond);
    if (!selectUndo ||
        editor.selection().objects().active() != first ||
        editor.selection().objects().hovered() != second ||
        !editor.selection().objects().contains(first) ||
        !editor.selection().objects().contains(second)) {
        return TestResult::fail("SelectObjectCommand undo should restore previous object selection");
    }

    editor::SelectObjectCommand notExecutedSelect{ first };
    if (notExecutedSelect.undo(dispatcher.context())) {
        return TestResult::fail("SelectObjectCommand undo should fail before successful execution");
    }

    editor.selection().objects().set({ first, second }, second);
    editor.selection().objects().set_hovered(first);

    editor::ClearObjectSelectionCommand clearSelection;
    if (clearSelection.name() != "Clear Object Selection") {
        return TestResult::fail("ClearObjectSelectionCommand should expose a stable command name");
    }

    editor.clear_dirty();
    const editor::CommandResult clearResult = dispatcher.execute(clearSelection);
    if (!clearResult ||
        !editor.selection().objects().empty() ||
        editor.selection().objects().active().is_valid() ||
        editor.selection().objects().hovered().is_valid() ||
        !has_all_flags(
            clearResult.dirtyFlags,
            editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking)) {
        return TestResult::fail("ClearObjectSelectionCommand should clear object selection and report dirty flags");
    }

    const editor::CommandResult clearUndo = dispatcher.undo(clearSelection);
    if (!clearUndo ||
        editor.selection().objects().active() != second ||
        editor.selection().objects().hovered() != first ||
        !editor.selection().objects().contains(first) ||
        !editor.selection().objects().contains(second)) {
        return TestResult::fail("ClearObjectSelectionCommand undo should restore previous selection");
    }

    editor::ClearObjectSelectionCommand notExecutedClear;
    if (notExecutedClear.undo(dispatcher.context())) {
        return TestResult::fail("ClearObjectSelectionCommand undo should fail before execution");
    }

    return TestResult::pass();
}

} // namespace locus::tests
