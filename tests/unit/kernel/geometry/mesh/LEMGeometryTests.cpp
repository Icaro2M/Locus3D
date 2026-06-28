/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include <glm/gtc/matrix_transform.hpp>

namespace {

[[nodiscard]] bool almost_equal(float lhs, float rhs)
{
    return glm::abs(lhs - rhs) < 0.0001f;
}

[[nodiscard]] bool almost_equal(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return almost_equal(lhs.x, rhs.x) &&
        almost_equal(lhs.y, rhs.y) &&
        almost_equal(lhs.z, rhs.z);
}

} // namespace

namespace locus::tests {

TestResult run_lem_geometry_tests()
{
    using namespace kernel::geometry;

    LEM mesh;
    LEMEditor editor(mesh);
    const QuadMesh quad = make_quad(editor);
    editor.clear_diff();

    if (!editor.translate_vertex(quad.v0, glm::vec3{ 1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("translate_vertex should accept active vertices");
    }

    if (!almost_equal(mesh.vertex(quad.v0).position, glm::vec3{ 0.0f, -1.0f, 0.0f })) {
        return TestResult::fail("translate_vertex should move the requested vertex");
    }

    if (!editor.set_vertex_position_lerp(
            quad.v1,
            glm::vec3{ 3.0f, -1.0f, 0.0f },
            0.5f)) {
        return TestResult::fail("set_vertex_position_lerp should accept active vertices");
    }

    if (!almost_equal(mesh.vertex(quad.v1).position, glm::vec3{ 2.0f, -1.0f, 0.0f })) {
        return TestResult::fail("set_vertex_position_lerp should interpolate and clamp t");
    }

    const std::size_t translated = editor.translate_vertices(
        { quad.v2, quad.v3 },
        glm::vec3{ 0.0f, 0.0f, 2.0f });

    if (translated != 2 ||
        !almost_equal(mesh.vertex(quad.v2).position, glm::vec3{ 1.0f, 1.0f, 2.0f }) ||
        !almost_equal(mesh.vertex(quad.v3).position, glm::vec3{ -1.0f, 1.0f, 2.0f })) {
        return TestResult::fail("translate_vertices should move every active requested vertex");
    }

    const glm::mat4 transform = glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });
    const std::size_t transformed = editor.transform_vertices({ quad.v0 }, transform);
    if (transformed != 1 ||
        !almost_equal(mesh.vertex(quad.v0).position, glm::vec3{ 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("transform_vertices should apply object-space matrix transforms");
    }

    if (!editor.rebuild_normals_around_face(quad.face)) {
        return TestResult::fail("rebuild_normals_around_face should accept active faces");
    }

    if (!almost_equal(glm::length(mesh.face(quad.face).normal), 1.0f)) {
        return TestResult::fail("rebuilt face normal should remain normalized");
    }

    return TestResult::pass();
}

} // namespace locus::tests
