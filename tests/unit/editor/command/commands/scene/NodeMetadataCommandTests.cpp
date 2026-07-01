/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/RenameNodeCommand.h"
#include "editor/command/scene/SetNodeLockCommand.h"
#include "editor/command/scene/SetNodeSelectableCommand.h"
#include "editor/command/scene/SetNodeVisibilityCommand.h"

namespace {

[[nodiscard]] bool has_all_flags(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flags)
{
    return (mask & flags) == flags;
}

[[nodiscard]] locus::tests::TestResult expect_scene_render_picking(
    const locus::editor::CommandResult& result,
    std::string message)
{
    if (!has_all_flags(
            result.dirtyFlags,
            locus::editor::EditorDirtyFlags::Scene |
                locus::editor::EditorDirtyFlags::Render |
                locus::editor::EditorDirtyFlags::Picking)) {
        return locus::tests::TestResult::fail(std::move(message));
    }

    return locus::tests::TestResult::pass();
}

} // namespace

namespace locus::tests {

TestResult run_node_metadata_command_tests()
{
    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    const editor::SceneNodeId nodeId = editor.scene().create_empty("Original");
    editor::SceneNode* node = editor.scene().find_node(nodeId);
    if (!node) {
        return TestResult::fail("test scene should contain a node");
    }

    editor::RenameNodeCommand invalidRename{ editor::SceneNodeId{}, "Name" };
    if (invalidRename.execute(dispatcher.context())) {
        return TestResult::fail("RenameNodeCommand should reject invalid ids");
    }

    editor::RenameNodeCommand emptyRename{ nodeId, "" };
    if (emptyRename.execute(dispatcher.context())) {
        return TestResult::fail("RenameNodeCommand should reject empty names");
    }

    editor::RenameNodeCommand rename{ nodeId, "Renamed" };
    if (rename.name() != "Rename Node" || rename.undo(dispatcher.context()) || rename.redo(dispatcher.context())) {
        return TestResult::fail("RenameNodeCommand should expose name and reject undo/redo before execution");
    }

    editor.clear_dirty();
    const editor::CommandResult renameResult = dispatcher.execute(rename);
    if (!renameResult || node->metadata().name != "Renamed") {
        return TestResult::fail("RenameNodeCommand should update node name");
    }

    TestResult flagsResult = expect_scene_render_picking(
        renameResult,
        "RenameNodeCommand should report scene/render/picking dirty flags");
    if (!flagsResult.success) {
        return flagsResult;
    }

    if (!editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
        return TestResult::fail("dispatcher should apply RenameNodeCommand dirty flags");
    }

    if (!dispatcher.undo(rename) || node->metadata().name != "Original") {
        return TestResult::fail("RenameNodeCommand undo should restore previous name");
    }

    if (!dispatcher.redo(rename) || node->metadata().name != "Renamed") {
        return TestResult::fail("RenameNodeCommand redo should reapply new name");
    }

    editor::SetNodeLockCommand invalidLock{ editor::SceneNodeId{}, true };
    if (invalidLock.execute(dispatcher.context())) {
        return TestResult::fail("SetNodeLockCommand should reject invalid ids");
    }

    editor::SetNodeLockCommand setLock{ nodeId, true };
    if (setLock.name() != "Set Node Lock" || setLock.undo(dispatcher.context()) || setLock.redo(dispatcher.context())) {
        return TestResult::fail("SetNodeLockCommand should expose name and reject undo/redo before execution");
    }

    const editor::CommandResult lockResult = dispatcher.execute(setLock);
    if (!lockResult || !node->metadata().locked || node->is_selectable()) {
        return TestResult::fail("SetNodeLockCommand should update locked state and affect selectability");
    }

    if (!editor::has_flag(lockResult.dirtyFlags, editor::EditorDirtyFlags::Selection)) {
        return TestResult::fail("SetNodeLockCommand should report selection dirty flag");
    }

    if (!dispatcher.undo(setLock) || node->metadata().locked) {
        return TestResult::fail("SetNodeLockCommand undo should restore previous locked state");
    }

    if (!dispatcher.redo(setLock) || !node->metadata().locked) {
        return TestResult::fail("SetNodeLockCommand redo should reapply locked state");
    }

    editor::SetNodeSelectableCommand invalidSelectable{ editor::SceneNodeId{}, false };
    if (invalidSelectable.execute(dispatcher.context())) {
        return TestResult::fail("SetNodeSelectableCommand should reject invalid ids");
    }

    node->metadata().locked = false;
    editor::SetNodeSelectableCommand setSelectable{ nodeId, false };
    if (setSelectable.name() != "Set Node Selectable" ||
        setSelectable.undo(dispatcher.context()) ||
        setSelectable.redo(dispatcher.context())) {
        return TestResult::fail("SetNodeSelectableCommand should expose name and reject undo/redo before execution");
    }

    const editor::CommandResult selectableResult = dispatcher.execute(setSelectable);
    if (!selectableResult || node->metadata().selectable || node->is_selectable()) {
        return TestResult::fail("SetNodeSelectableCommand should update selectable state");
    }

    if (!editor::has_flag(selectableResult.dirtyFlags, editor::EditorDirtyFlags::Selection)) {
        return TestResult::fail("SetNodeSelectableCommand should report selection dirty flag");
    }

    if (!dispatcher.undo(setSelectable) || !node->metadata().selectable || !node->is_selectable()) {
        return TestResult::fail("SetNodeSelectableCommand undo should restore previous selectable state");
    }

    if (!dispatcher.redo(setSelectable) || node->metadata().selectable) {
        return TestResult::fail("SetNodeSelectableCommand redo should reapply selectable state");
    }

    editor::SetNodeVisibilityCommand invalidVisibility{ editor::SceneNodeId{}, false };
    if (invalidVisibility.execute(dispatcher.context())) {
        return TestResult::fail("SetNodeVisibilityCommand should reject invalid ids");
    }

    node->metadata().selectable = true;
    editor::SetNodeVisibilityCommand setVisibility{ nodeId, false };
    if (setVisibility.name() != "Set Node Visibility" ||
        setVisibility.undo(dispatcher.context()) ||
        setVisibility.redo(dispatcher.context())) {
        return TestResult::fail("SetNodeVisibilityCommand should expose name and reject undo/redo before execution");
    }

    const editor::CommandResult visibilityResult = dispatcher.execute(setVisibility);
    if (!visibilityResult || node->metadata().visible || node->is_visible() || node->is_selectable()) {
        return TestResult::fail("SetNodeVisibilityCommand should update visibility state");
    }

    if (!editor::has_flag(visibilityResult.dirtyFlags, editor::EditorDirtyFlags::Selection)) {
        return TestResult::fail("SetNodeVisibilityCommand should report selection dirty flag");
    }

    if (!dispatcher.undo(setVisibility) || !node->metadata().visible || !node->is_visible()) {
        return TestResult::fail("SetNodeVisibilityCommand undo should restore previous visibility state");
    }

    if (!dispatcher.redo(setVisibility) || node->metadata().visible) {
        return TestResult::fail("SetNodeVisibilityCommand redo should reapply visibility state");
    }

    editor.scene().remove_node(nodeId);

    editor::RenameNodeCommand missingRename{ nodeId, "Missing" };
    editor::SetNodeLockCommand missingLock{ nodeId, false };
    editor::SetNodeSelectableCommand missingSelectable{ nodeId, true };
    editor::SetNodeVisibilityCommand missingVisibility{ nodeId, true };

    if (missingRename.execute(dispatcher.context()) ||
        missingLock.execute(dispatcher.context()) ||
        missingSelectable.execute(dispatcher.context()) ||
        missingVisibility.execute(dispatcher.context())) {
        return TestResult::fail("node metadata commands should reject missing nodes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
