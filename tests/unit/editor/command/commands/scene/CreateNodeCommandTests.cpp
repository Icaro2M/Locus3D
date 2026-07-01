/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/CreateEmptyNodeCommand.h"
#include "editor/command/scene/CreateMeshNodeCommand.h"
#include "editor/scene/NodeType.h"

namespace {

[[nodiscard]] bool has_all_flags(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flags)
{
    return (mask & flags) == flags;
}

} // namespace

namespace locus::tests {

TestResult run_create_node_command_tests()
{
    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    editor::CreateEmptyNodeCommand emptyCommand("");
    if (emptyCommand.name() != "Create Empty Node" ||
        emptyCommand.created_node().is_valid()) {
        return TestResult::fail("CreateEmptyNodeCommand should expose name and invalid id before execution");
    }

    if (emptyCommand.undo(dispatcher.context())) {
        return TestResult::fail("CreateEmptyNodeCommand undo should fail before execution");
    }

    editor.clear_dirty();
    const editor::CommandResult emptyResult = dispatcher.execute(emptyCommand);
    const editor::SceneNodeId emptyId = emptyCommand.created_node();
    const editor::SceneNode* emptyNode = editor.scene().find_node(emptyId);

    if (!emptyResult ||
        !emptyId.is_valid() ||
        !emptyNode ||
        emptyNode->type() != editor::NodeType::Empty ||
        emptyNode->metadata().name != "Empty") {
        return TestResult::fail("CreateEmptyNodeCommand should create an empty node with fallback name");
    }

    if (!has_all_flags(
            emptyResult.dirtyFlags,
            editor::EditorDirtyFlags::Scene |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking)) {
        return TestResult::fail("CreateEmptyNodeCommand should report scene/render/picking dirty flags");
    }

    if (!editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
        return TestResult::fail("dispatcher should apply CreateEmptyNodeCommand dirty flags");
    }

    const editor::CommandResult emptyUndo = dispatcher.undo(emptyCommand);
    if (!emptyUndo ||
        emptyCommand.created_node().is_valid() ||
        editor.scene().find_node(emptyId) != nullptr ||
        !editor::has_flag(emptyUndo.dirtyFlags, editor::EditorDirtyFlags::Selection)) {
        return TestResult::fail("CreateEmptyNodeCommand undo should remove created node and clear id");
    }

    editor::CreateMeshNodeCommand meshCommand("Editable Mesh");
    if (meshCommand.name() != "Create Mesh Node" ||
        meshCommand.created_node().is_valid()) {
        return TestResult::fail("CreateMeshNodeCommand should expose name and invalid id before execution");
    }

    if (meshCommand.undo(dispatcher.context())) {
        return TestResult::fail("CreateMeshNodeCommand undo should fail before execution");
    }

    editor.clear_dirty();
    const editor::CommandResult meshResult = dispatcher.execute(meshCommand);
    const editor::SceneNodeId meshId = meshCommand.created_node();
    const editor::SceneNode* meshNode = editor.scene().find_node(meshId);

    if (!meshResult ||
        !meshId.is_valid() ||
        !meshNode ||
        meshNode->type() != editor::NodeType::Mesh ||
        meshNode->metadata().name != "Editable Mesh" ||
        editor.scene().find_mesh(meshId) == nullptr) {
        return TestResult::fail("CreateMeshNodeCommand should create a mesh node with requested name");
    }

    if (!has_all_flags(
            meshResult.dirtyFlags,
            editor::EditorDirtyFlags::Scene |
                editor::EditorDirtyFlags::Mesh |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking)) {
        return TestResult::fail("CreateMeshNodeCommand should report scene/mesh/render/picking dirty flags");
    }

    const editor::CommandResult meshUndo = dispatcher.undo(meshCommand);
    if (!meshUndo ||
        meshCommand.created_node().is_valid() ||
        editor.scene().find_node(meshId) != nullptr ||
        !editor::has_flag(meshUndo.dirtyFlags, editor::EditorDirtyFlags::Mesh)) {
        return TestResult::fail("CreateMeshNodeCommand undo should remove created mesh and clear id");
    }

    editor::CreateMeshNodeCommand fallbackMeshCommand("");
    if (!dispatcher.execute(fallbackMeshCommand)) {
        return TestResult::fail("CreateMeshNodeCommand should execute with an empty constructor name");
    }

    const editor::SceneNode* fallbackMeshNode =
        editor.scene().find_node(fallbackMeshCommand.created_node());
    if (!fallbackMeshNode || fallbackMeshNode->metadata().name != "Mesh") {
        return TestResult::fail("CreateMeshNodeCommand should use Mesh as fallback name");
    }

    return TestResult::pass();
}

} // namespace locus::tests
