/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DocumentCommandTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/document/ClearSceneCommand.h"

#include <glm/vec3.hpp>

namespace {

[[nodiscard]] bool has_all_document_flags(locus::editor::EditorDirtyFlags flags)
{
    return (flags &
        (locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Selection |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking |
            locus::editor::EditorDirtyFlags::Manufacturing)) ==
        (locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Selection |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking |
            locus::editor::EditorDirtyFlags::Manufacturing);
}

} // namespace

namespace locus::tests {

TestResult run_clear_scene_command_tests()
{
    using namespace kernel::geometry;

    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    editor::ClearSceneCommand emptyClear;
    if (emptyClear.name() != "Clear Scene") {
        return TestResult::fail("ClearSceneCommand should expose a stable command name");
    }

    if (emptyClear.execute(dispatcher.context())) {
        return TestResult::fail("ClearSceneCommand should reject an already empty scene");
    }

    const editor::SceneNodeId root = editor.scene().create_empty("Root");
    const editor::SceneNodeId mesh = editor.scene().create_mesh("Mesh");
    editor.scene().reparent(mesh, root);

    editor::SceneNode* rootNode = editor.scene().find_node(root);
    editor::MeshNode* meshNode = editor.scene().find_mesh(mesh);
    if (!rootNode || !meshNode) {
        return TestResult::fail("test setup should create root and mesh nodes");
    }

    rootNode->metadata().locked = true;
    rootNode->metadata().expanded = false;
    rootNode->transform().set_position(glm::vec3{ 2.0f, 3.0f, 4.0f });
    meshNode->mesh().add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });

    editor.selection().objects().set(mesh);
    editor.selection().objects().set_hovered(root);
    editor.selection().mesh().set_active_mesh(mesh);
    editor.selection().mesh().add_vertex(VertexHandle{ 0 });
    editor.selection().set_granularity(editor::SelectionGranularity::Vertex);
    editor.selection().set_scope(editor::SelectionScope::ActiveMesh);

    editor::ClearSceneCommand clearCommand;
    if (clearCommand.undo(dispatcher.context()) ||
        clearCommand.redo(dispatcher.context())) {
        return TestResult::fail("ClearSceneCommand undo/redo should fail before execution");
    }

    const editor::CommandResult clearResult = dispatcher.execute(clearCommand);
    if (!clearResult ||
        !has_all_document_flags(clearResult.dirtyFlags) ||
        !editor.scene().tree().empty() ||
        !editor.selection().objects().empty() ||
        editor.selection().mesh().active_mesh().is_valid()) {
        return TestResult::fail("ClearSceneCommand should clear scene and selection state");
    }

    const editor::CommandResult undoResult = dispatcher.undo(clearCommand);
    const editor::SceneNode* restoredRoot = editor.scene().find_node(root);
    const editor::MeshNode* restoredMesh = editor.scene().find_mesh(mesh);
    if (!undoResult ||
        !restoredRoot ||
        !restoredMesh ||
        restoredMesh->parent() != root ||
        restoredRoot->metadata().name != "Root" ||
        !restoredRoot->metadata().locked ||
        restoredRoot->metadata().expanded ||
        restoredRoot->transform().position() != glm::vec3{ 2.0f, 3.0f, 4.0f } ||
        restoredMesh->mesh().vertex_count() != 1u ||
        !editor.selection().objects().contains(mesh) ||
        editor.selection().objects().hovered() != root ||
        editor.selection().mesh().active_mesh() != mesh ||
        !editor.selection().mesh().vertices().contains(VertexHandle{ 0 }) ||
        editor.selection().granularity() != editor::SelectionGranularity::Vertex ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("ClearSceneCommand undo should restore hierarchy, mesh and selection snapshots");
    }

    if (!dispatcher.redo(clearCommand) ||
        !editor.scene().tree().empty() ||
        !editor.selection().objects().empty()) {
        return TestResult::fail("ClearSceneCommand redo should clear the restored scene again");
    }

    if (!dispatcher.undo(clearCommand)) {
        return TestResult::fail("ClearSceneCommand should undo again after redo");
    }

    editor.scene().create_empty("Blocking Node");
    if (clearCommand.undo(dispatcher.context())) {
        return TestResult::fail("ClearSceneCommand undo should fail when the current scene is not empty");
    }

    return TestResult::pass();
}

} // namespace locus::tests
