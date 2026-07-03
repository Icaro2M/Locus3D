/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MeshCommandTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/mesh/MeshSnapshot.h"

#include <glm/vec3.hpp>

namespace locus::tests {

TestResult run_mesh_snapshot_tests()
{
    using namespace kernel::geometry;

    editor::Editor editor;
    const editor::SceneNodeId meshId = editor.scene().create_mesh("Mesh");
    editor::MeshNode* node = editor.scene().find_mesh(meshId);
    if (!node) {
        return TestResult::fail("test setup should create a mesh node");
    }

    const VertexHandle v0 = node->mesh().add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = node->mesh().add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    editor.selection().mesh().set_active_mesh(meshId);
    editor.selection().mesh().add_vertex(v0);
    editor.selection().mesh().set_hovered_vertex(v1);
    editor.selection().set_granularity(editor::SelectionGranularity::Vertex);
    editor.selection().set_scope(editor::SelectionScope::ActiveMesh);

    editor::MeshSnapshot snapshot;
    if (snapshot.is_valid()) {
        return TestResult::fail("MeshSnapshot should start invalid");
    }

    snapshot.capture(*node, editor.selection());
    if (!snapshot.is_valid() || snapshot.mesh().vertex_count() != 2u) {
        return TestResult::fail("MeshSnapshot should capture mesh data");
    }

    node->mesh().clear();
    editor.selection().clear();
    snapshot.restore(*node, editor.selection());

    if (node->mesh().vertex_count() != 2u ||
        !node->mesh().is_valid(v0) ||
        editor.selection().mesh().active_mesh() != meshId ||
        !editor.selection().mesh().vertices().contains(v0) ||
        editor.selection().mesh().hovered_vertex() != v1 ||
        editor.selection().granularity() != editor::SelectionGranularity::Vertex ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("MeshSnapshot should restore mesh and mesh selection state");
    }

    return TestResult::pass();
}

} // namespace locus::tests
