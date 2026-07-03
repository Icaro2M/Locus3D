/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file SpatialTestSuite.h
 * @brief Declarations and helpers for spatial geometry unit tests.
 */

#include "common/TestResult.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/vec3.hpp>

namespace locus::tests {

struct SpatialQuad {
    kernel::geometry::VertexHandle v0;
    kernel::geometry::VertexHandle v1;
    kernel::geometry::VertexHandle v2;
    kernel::geometry::VertexHandle v3;
    kernel::geometry::FaceHandle face;
};

[[nodiscard]] inline SpatialQuad make_spatial_quad(
    kernel::geometry::LEMEditor& editor,
    const glm::vec3& offset = glm::vec3{ 0.0f, 0.0f, 0.0f })
{
    SpatialQuad quad{};
    quad.v0 = editor.add_vertex(offset + glm::vec3{ -1.0f, -1.0f, 0.0f });
    quad.v1 = editor.add_vertex(offset + glm::vec3{ 1.0f, -1.0f, 0.0f });
    quad.v2 = editor.add_vertex(offset + glm::vec3{ 1.0f, 1.0f, 0.0f });
    quad.v3 = editor.add_vertex(offset + glm::vec3{ -1.0f, 1.0f, 0.0f });
    quad.face = editor.add_face({ quad.v0, quad.v1, quad.v2, quad.v3 });
    return quad;
}

[[nodiscard]] TestResult run_bvh_tests();
[[nodiscard]] TestResult run_bvh_query_tests();
[[nodiscard]] TestResult run_spatial_index_tests();

} // namespace locus::tests
