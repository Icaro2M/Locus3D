/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file LEMTestSuite.h
 * @brief Declarations and small helpers for LEM unit test groups.
 */

#include "common/TestResult.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/vec3.hpp>

#include <vector>

namespace locus::tests {

struct QuadMesh {
    kernel::geometry::VertexHandle v0;
    kernel::geometry::VertexHandle v1;
    kernel::geometry::VertexHandle v2;
    kernel::geometry::VertexHandle v3;
    kernel::geometry::FaceHandle face;
};

[[nodiscard]] inline QuadMesh make_quad(kernel::geometry::LEMEditor& editor)
{
    QuadMesh quad{};
    quad.v0 = editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    quad.v1 = editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    quad.v2 = editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
    quad.v3 = editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });
    quad.face = editor.add_face({ quad.v0, quad.v1, quad.v2, quad.v3 });
    return quad;
}

[[nodiscard]] inline std::vector<kernel::geometry::VertexHandle> quad_vertices(const QuadMesh& quad)
{
    return { quad.v0, quad.v1, quad.v2, quad.v3 };
}

[[nodiscard]] TestResult run_lem_topology_tests();
[[nodiscard]] TestResult run_lem_editor_diff_tests();
[[nodiscard]] TestResult run_lem_geometry_tests();
[[nodiscard]] TestResult run_lem_attribute_tests();
[[nodiscard]] TestResult run_face_orientation_render_tests();
[[nodiscard]] TestResult run_bevel_operation_tests();
[[nodiscard]] TestResult run_bridge_edge_operation_tests();
[[nodiscard]] TestResult run_flip_face_operation_tests();
[[nodiscard]] TestResult run_fill_hole_operation_tests();
[[nodiscard]] TestResult run_delete_mesh_elements_operation_tests();
[[nodiscard]] TestResult run_dissolve_mesh_elements_operation_tests();

} // namespace locus::tests
