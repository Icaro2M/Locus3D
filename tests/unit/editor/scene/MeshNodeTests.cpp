/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorSceneTestSuite.h"

#include "editor/scene/MeshNode.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/vec3.hpp>

namespace locus::tests {

TestResult run_mesh_node_tests()
{
    using namespace kernel::geometry;

    editor::MeshNode meshNode{ editor::SceneNodeId{ 3 }, "Quad Mesh" };

    if (meshNode.type() != editor::NodeType::Mesh) {
        return TestResult::fail("MeshNode should report mesh node type");
    }

    meshNode.clear_dirty();
    if (meshNode.dirty_flags() != editor::EditorDirtyFlags::None) {
        return TestResult::fail("MeshNode dirty flags should clear");
    }

    LEMEditor meshEditor(meshNode.mesh());
    if (!editor::has_flag(meshNode.dirty_flags(), editor::EditorDirtyFlags::Mesh)) {
        return TestResult::fail("mutable mesh access should mark the node mesh-dirty");
    }

    const VertexHandle v0 = meshEditor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    const VertexHandle v1 = meshEditor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    const VertexHandle v2 = meshEditor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
    const VertexHandle v3 = meshEditor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });

    const FaceHandle face = meshEditor.add_face({ v0, v1, v2, v3 });
    if (!face.is_valid()) {
        return TestResult::fail("LEMEditor should create a valid quad face inside MeshNode");
    }

    if (meshNode.mesh().vertex_count() != 4 ||
        meshNode.mesh().edge_count() != 4 ||
        meshNode.mesh().loop_count() != 4 ||
        meshNode.mesh().face_count() != 1) {
        return TestResult::fail("MeshNode LEM should contain the expected quad topology");
    }

    return TestResult::pass();
}

} // namespace locus::tests
