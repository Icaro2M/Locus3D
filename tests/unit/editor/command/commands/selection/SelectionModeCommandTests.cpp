/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/selection/SetSelectionGranularityCommand.h"
#include "editor/command/selection/SetSelectionScopeCommand.h"
#include "editor/command/selection/ToggleObjectSelectionCommand.h"

namespace {

[[nodiscard]] bool has_all_flags(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flags)
{
    return (mask & flags) == flags;
}

} // namespace

namespace locus::tests {

TestResult run_selection_mode_command_tests()
{
    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    const editor::SceneNodeId first = editor.scene().create_empty("First");
    const editor::SceneNodeId second = editor.scene().create_empty("Second");
    const editor::SceneNodeId locked = editor.scene().create_empty("Locked");
    editor.scene().find_node(locked)->metadata().locked = true;

    editor::ToggleObjectSelectionCommand invalidToggle{ editor::SceneNodeId{} };
    if (invalidToggle.execute(dispatcher.context())) {
        return TestResult::fail("ToggleObjectSelectionCommand should reject invalid object ids");
    }

    editor::ToggleObjectSelectionCommand lockedToggle{ locked };
    if (lockedToggle.execute(dispatcher.context())) {
        return TestResult::fail("ToggleObjectSelectionCommand should reject unselectable objects");
    }

    editor.selection().objects().set(first);
    editor.selection().objects().set_hovered(second);

    editor::ToggleObjectSelectionCommand toggleSecond{ second };
    if (toggleSecond.name() != "Toggle Object Selection") {
        return TestResult::fail("ToggleObjectSelectionCommand should expose a stable command name");
    }

    editor.clear_dirty();
    const editor::CommandResult addResult = dispatcher.execute(toggleSecond);
    if (!addResult ||
        !editor.selection().objects().contains(first) ||
        !editor.selection().objects().contains(second) ||
        editor.selection().objects().active() != second ||
        !has_all_flags(
            addResult.dirtyFlags,
            editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking)) {
        return TestResult::fail("ToggleObjectSelectionCommand should add an unselected object");
    }

    const editor::CommandResult addUndo = dispatcher.undo(toggleSecond);
    if (!addUndo ||
        !editor.selection().objects().contains(first) ||
        editor.selection().objects().contains(second) ||
        editor.selection().objects().hovered() != second) {
        return TestResult::fail("ToggleObjectSelectionCommand undo should restore previous object selection");
    }

    editor.selection().objects().set({ first, second }, second);
    editor::ToggleObjectSelectionCommand removeFirst{ first };
    const editor::CommandResult removeResult = dispatcher.execute(removeFirst);
    if (!removeResult ||
        editor.selection().objects().contains(first) ||
        !editor.selection().objects().contains(second)) {
        return TestResult::fail("ToggleObjectSelectionCommand should remove a selected object");
    }

    editor::ToggleObjectSelectionCommand notExecutedToggle{ first };
    if (notExecutedToggle.undo(dispatcher.context())) {
        return TestResult::fail("ToggleObjectSelectionCommand undo should fail before execution");
    }

    editor.selection().set_scope(editor::SelectionScope::ActiveMesh);
    editor.selection().set_granularity(editor::SelectionGranularity::Face);

    editor::SetSelectionGranularityCommand setEdge{ editor::SelectionGranularity::Edge };
    if (setEdge.name() != "Set Selection Granularity") {
        return TestResult::fail("SetSelectionGranularityCommand should expose a stable command name");
    }

    const editor::CommandResult granularityResult = dispatcher.execute(setEdge);
    if (!granularityResult ||
        editor.selection().granularity() != editor::SelectionGranularity::Edge ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh ||
        !has_all_flags(
            granularityResult.dirtyFlags,
            editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking)) {
        return TestResult::fail("SetSelectionGranularityCommand should change granularity and report dirty flags");
    }

    const editor::CommandResult granularityUndo = dispatcher.undo(setEdge);
    if (!granularityUndo ||
        editor.selection().granularity() != editor::SelectionGranularity::Face ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("SetSelectionGranularityCommand undo should restore granularity and scope");
    }

    editor::SetSelectionGranularityCommand unchangedGranularity{ editor::SelectionGranularity::Face };
    const editor::CommandResult unchangedGranularityResult = dispatcher.execute(unchangedGranularity);
    if (!unchangedGranularityResult ||
        unchangedGranularityResult.dirtyFlags != editor::EditorDirtyFlags::None ||
        editor.selection().granularity() != editor::SelectionGranularity::Face) {
        return TestResult::fail("SetSelectionGranularityCommand should be clean when unchanged");
    }

    editor::SetSelectionGranularityCommand notExecutedGranularity{ editor::SelectionGranularity::Vertex };
    if (notExecutedGranularity.undo(dispatcher.context())) {
        return TestResult::fail("SetSelectionGranularityCommand undo should fail before execution");
    }

    editor.selection().set_scope(editor::SelectionScope::Scene);
    editor.selection().set_granularity(editor::SelectionGranularity::Object);

    editor::SetSelectionScopeCommand setActiveMesh{ editor::SelectionScope::ActiveMesh };
    if (setActiveMesh.name() != "Set Selection Scope") {
        return TestResult::fail("SetSelectionScopeCommand should expose a stable command name");
    }

    const editor::CommandResult scopeResult = dispatcher.execute(setActiveMesh);
    if (!scopeResult ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh ||
        editor.selection().granularity() != editor::SelectionGranularity::Object ||
        !has_all_flags(
            scopeResult.dirtyFlags,
            editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking)) {
        return TestResult::fail("SetSelectionScopeCommand should change scope and report dirty flags");
    }

    const editor::CommandResult scopeUndo = dispatcher.undo(setActiveMesh);
    if (!scopeUndo ||
        editor.selection().scope() != editor::SelectionScope::Scene ||
        editor.selection().granularity() != editor::SelectionGranularity::Object) {
        return TestResult::fail("SetSelectionScopeCommand undo should restore scope and granularity");
    }

    editor::SetSelectionScopeCommand unchangedScope{ editor::SelectionScope::Scene };
    const editor::CommandResult unchangedScopeResult = dispatcher.execute(unchangedScope);
    if (!unchangedScopeResult ||
        unchangedScopeResult.dirtyFlags != editor::EditorDirtyFlags::None ||
        editor.selection().scope() != editor::SelectionScope::Scene) {
        return TestResult::fail("SetSelectionScopeCommand should be clean when unchanged");
    }

    editor::SetSelectionScopeCommand notExecutedScope{ editor::SelectionScope::ActiveMesh };
    if (notExecutedScope.undo(dispatcher.context())) {
        return TestResult::fail("SetSelectionScopeCommand undo should fail before execution");
    }

    return TestResult::pass();
}

} // namespace locus::tests
