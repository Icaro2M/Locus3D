/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MeshCommandTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"

#include <glm/vec3.hpp>

namespace locus::tests {

TestResult run_apply_mesh_operation_command_tests()
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

    node->mesh().add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });

    editor::ApplyMeshOperationCommand fallbackName{ meshId, [](LEMEditor&) { return true; }, "" };
    if (fallbackName.name() != "Apply Mesh Operation") {
        return TestResult::fail("ApplyMeshOperationCommand should use a fallback label");
    }

    editor::ApplyMeshOperationCommand invalidNode{ editor::SceneNodeId{}, [](LEMEditor&) { return true; } };
    if (invalidNode.execute(dispatcher.context())) {
        return TestResult::fail("ApplyMeshOperationCommand should reject an invalid node");
    }

    editor::ApplyMeshOperationCommand missingCallback{ meshId, {} };
    if (missingCallback.execute(dispatcher.context())) {
        return TestResult::fail("ApplyMeshOperationCommand should reject a missing callback");
    }

    editor::ApplyMeshOperationCommand wrongType{ emptyId, [](LEMEditor&) { return true; } };
    if (wrongType.execute(dispatcher.context())) {
        return TestResult::fail("ApplyMeshOperationCommand should reject non-mesh nodes");
    }

    int operationCalls = 0;
    editor::ApplyMeshOperationCommand addVertex{
        meshId,
        [&operationCalls](LEMEditor& meshEditor) {
            ++operationCalls;
            meshEditor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
            return true;
        },
        "Add Vertex"
    };

    if (addVertex.name() != "Add Vertex") {
        return TestResult::fail("ApplyMeshOperationCommand should expose the provided label");
    }

    const editor::CommandResult executeResult = dispatcher.execute(addVertex);
    if (!executeResult ||
        operationCalls != 1 ||
        node->mesh().vertex_count() != 2u ||
        editor.selection().mesh().active_mesh() != meshId) {
        return TestResult::fail("ApplyMeshOperationCommand should apply the callback once");
    }

    if (!dispatcher.undo(addVertex) || node->mesh().vertex_count() != 1u) {
        return TestResult::fail("ApplyMeshOperationCommand undo should restore the previous mesh");
    }

    if (!dispatcher.redo(addVertex) ||
        operationCalls != 1 ||
        node->mesh().vertex_count() != 2u) {
        return TestResult::fail("ApplyMeshOperationCommand redo should restore the captured next mesh");
    }

    if (!dispatcher.execute(addVertex) ||
        operationCalls != 1 ||
        node->mesh().vertex_count() != 2u) {
        return TestResult::fail("ApplyMeshOperationCommand execute after first run should delegate to redo");
    }

    editor::ApplyMeshOperationCommand notExecuted{ meshId, [](LEMEditor&) { return true; } };
    if (notExecuted.undo(dispatcher.context()) ||
        notExecuted.redo(dispatcher.context())) {
        return TestResult::fail("ApplyMeshOperationCommand undo/redo should fail before execution");
    }

    const std::size_t beforeFailureCount = node->mesh().vertex_count();
    editor::ApplyMeshOperationCommand failingOperation{
        meshId,
        [](LEMEditor& meshEditor) {
            meshEditor.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f });
            return false;
        }
    };

    if (failingOperation.execute(dispatcher.context()) ||
        node->mesh().vertex_count() != beforeFailureCount) {
        return TestResult::fail("ApplyMeshOperationCommand should restore the previous mesh on callback failure");
    }

    return TestResult::pass();
}

} // namespace locus::tests
