/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MeshCommandTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/mesh/EditMeshSelectionCommand.h"

#include <glm/vec3.hpp>

namespace locus::tests {

TestResult run_edit_mesh_selection_command_tests()
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

    const VertexHandle vertex = node->mesh().add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });

    editor::EditMeshSelectionCommand fallbackName{
        meshId,
        [](LEM&, editor::SelectionState&) { return true; },
        ""
    };
    if (fallbackName.name() != "Edit Mesh Selection") {
        return TestResult::fail("EditMeshSelectionCommand should use a fallback label");
    }

    editor::EditMeshSelectionCommand invalidNode{
        editor::SceneNodeId{},
        [](LEM&, editor::SelectionState&) { return true; }
    };
    if (invalidNode.execute(dispatcher.context())) {
        return TestResult::fail("EditMeshSelectionCommand should reject an invalid node");
    }

    editor::EditMeshSelectionCommand missingCallback{ meshId, {} };
    if (missingCallback.execute(dispatcher.context())) {
        return TestResult::fail("EditMeshSelectionCommand should reject a missing callback");
    }

    editor::EditMeshSelectionCommand wrongType{
        emptyId,
        [](LEM&, editor::SelectionState&) { return true; }
    };
    if (wrongType.execute(dispatcher.context())) {
        return TestResult::fail("EditMeshSelectionCommand should reject non-mesh nodes");
    }

    int editCalls = 0;
    editor::EditMeshSelectionCommand selectVertex{
        meshId,
        [meshId, vertex, &editCalls](LEM& mesh, editor::SelectionState& selection) {
            ++editCalls;
            if (selection.mesh().active_mesh() != meshId) {
                return false;
            }

            mesh.vertex(vertex).selected = true;
            selection.mesh().set_vertex(vertex);
            selection.set_granularity(editor::SelectionGranularity::Vertex);
            selection.set_scope(editor::SelectionScope::ActiveMesh);
            return true;
        },
        "Select Vertex"
    };

    if (selectVertex.name() != "Select Vertex") {
        return TestResult::fail("EditMeshSelectionCommand should expose the provided label");
    }

    const editor::CommandResult executeResult = dispatcher.execute(selectVertex);
    if (!executeResult ||
        editCalls != 1 ||
        !node->mesh().vertex(vertex).selected ||
        editor.selection().mesh().active_mesh() != meshId ||
        !editor.selection().mesh().vertices().contains(vertex) ||
        editor.selection().granularity() != editor::SelectionGranularity::Vertex ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("EditMeshSelectionCommand should edit mesh and selection state");
    }

    if (!dispatcher.undo(selectVertex) ||
        node->mesh().vertex(vertex).selected ||
        !editor.selection().mesh().empty()) {
        return TestResult::fail("EditMeshSelectionCommand undo should restore previous mesh selection state");
    }

    if (!dispatcher.redo(selectVertex) ||
        editCalls != 1 ||
        !node->mesh().vertex(vertex).selected ||
        !editor.selection().mesh().vertices().contains(vertex)) {
        return TestResult::fail("EditMeshSelectionCommand redo should restore captured edited selection");
    }

    editor::EditMeshSelectionCommand notExecuted{
        meshId,
        [](LEM&, editor::SelectionState&) { return true; }
    };
    if (notExecuted.undo(dispatcher.context()) ||
        notExecuted.redo(dispatcher.context())) {
        return TestResult::fail("EditMeshSelectionCommand undo/redo should fail before execution");
    }

    editor.selection().mesh().clear_components();
    node->mesh().vertex(vertex).selected = false;
    editor::EditMeshSelectionCommand failingEdit{
        meshId,
        [vertex](LEM& mesh, editor::SelectionState& selection) {
            mesh.vertex(vertex).selected = true;
            selection.mesh().set_vertex(vertex);
            return false;
        }
    };

    if (failingEdit.execute(dispatcher.context()) ||
        node->mesh().vertex(vertex).selected ||
        !editor.selection().mesh().empty()) {
        return TestResult::fail("EditMeshSelectionCommand should restore previous state on edit failure");
    }

    return TestResult::pass();
}

} // namespace locus::tests
