/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MeshCommandTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/mesh/ReplaceMeshCommand.h"

#include <glm/vec3.hpp>

namespace {

[[nodiscard]] bool has_all_flags(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flags)
{
    return (mask & flags) == flags;
}

[[nodiscard]] locus::kernel::geometry::LEM make_triangle_mesh()
{
    using namespace locus::kernel::geometry;

    LEM mesh;
    const VertexHandle v0 = mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    mesh.add_face({ v0, v1, v2 });
    return mesh;
}

} // namespace

namespace locus::tests {

TestResult run_replace_mesh_command_tests()
{
    using namespace kernel::geometry;

    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    const editor::SceneNodeId emptyId = editor.scene().create_empty("Empty");
    const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh");
    editor::MeshNode* node = editor.scene().find_mesh(meshId);
    if (!node) {
        return TestResult::fail("test setup should create a mesh node");
    }

    const VertexHandle originalVertex =
        node->mesh().add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    editor.selection().mesh().set_active_mesh(meshId);
    editor.selection().mesh().add_vertex(originalVertex);

    const LEM replacement = make_triangle_mesh();
    editor::ReplaceMeshCommand invalidCommand{ editor::SceneNodeId{}, replacement };
    if (invalidCommand.execute(dispatcher.context())) {
        return TestResult::fail("ReplaceMeshCommand should reject an invalid node");
    }

    editor::ReplaceMeshCommand wrongTypeCommand{ emptyId, replacement };
    if (wrongTypeCommand.execute(dispatcher.context())) {
        return TestResult::fail("ReplaceMeshCommand should reject non-mesh nodes");
    }

    editor::ReplaceMeshCommand replaceCommand{ meshId, replacement };
    if (replaceCommand.name() != "Replace Mesh") {
        return TestResult::fail("ReplaceMeshCommand should expose a stable command name");
    }

    const editor::CommandResult replaceResult = dispatcher.execute(replaceCommand);
    if (!replaceResult ||
        !has_all_flags(
            replaceResult.dirtyFlags,
            editor::EditorDirtyFlags::Mesh |
                editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render |
                editor::EditorDirtyFlags::Picking) ||
        node->mesh().vertex_count() != 3u ||
        node->mesh().face_count() != 1u ||
        editor.selection().mesh().active_mesh() != meshId ||
        !editor.selection().mesh().empty()) {
        return TestResult::fail("ReplaceMeshCommand should replace mesh and clear component selection");
    }

    const editor::CommandResult undoResult = dispatcher.undo(replaceCommand);
    if (!undoResult ||
        node->mesh().vertex_count() != 1u ||
        !node->mesh().is_valid(originalVertex) ||
        !editor.selection().mesh().vertices().contains(originalVertex)) {
        return TestResult::fail("ReplaceMeshCommand undo should restore previous mesh and selection");
    }

    const editor::CommandResult redoResult = dispatcher.redo(replaceCommand);
    if (!redoResult ||
        node->mesh().vertex_count() != 3u ||
        node->mesh().face_count() != 1u ||
        !editor.selection().mesh().empty()) {
        return TestResult::fail("ReplaceMeshCommand redo should restore replacement mesh snapshot");
    }

    editor::ReplaceMeshCommand notExecutedCommand{ meshId, replacement };
    if (notExecutedCommand.undo(dispatcher.context()) ||
        notExecutedCommand.redo(dispatcher.context())) {
        return TestResult::fail("ReplaceMeshCommand undo/redo should fail before execution");
    }

    editor.selection().mesh().add_vertex(VertexHandle{ 0 });
    editor::ReplaceMeshCommand keepSelectionCommand{ meshId, replacement, false };
    if (!dispatcher.execute(keepSelectionCommand) ||
        !editor.selection().mesh().vertices().contains(VertexHandle{ 0 })) {
        return TestResult::fail("ReplaceMeshCommand should optionally keep component selection");
    }

    return TestResult::pass();
}

} // namespace locus::tests
