/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QueryTestSuite.h"

#include "kernel/geometry/queries/BoundsQuery.h"

#include <cmath>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] bool near_bounds(
    const locus::kernel::math::Bounds& bounds,
    const glm::vec3& min,
    const glm::vec3& max)
{
    return bounds.is_valid() && near_vec3(bounds.min, min) && near_vec3(bounds.max, max);
}

} // namespace

namespace locus::tests {

TestResult run_bounds_query_tests()
{
    using namespace kernel::geometry;

    QueryMesh fixture = make_query_mesh();
    LEM& mesh = fixture.mesh;

    mesh.vertex(fixture.looseVertex).hidden = true;
    const kernel::math::Bounds meshBounds = BoundsQuery::mesh_bounds(mesh);
    if (!near_bounds(
            meshBounds,
            glm::vec3{ -1.0f, -1.0f, 0.0f },
            glm::vec3{ 5.0f, 1.0f, 0.0f })) {
        return TestResult::fail("mesh_bounds should include visible vertices and ignore hidden vertices");
    }

    mesh.vertex(fixture.v0).selected = true;
    mesh.edge(mesh.find_edge(fixture.v1, fixture.v4)).selected = true;
    mesh.face(fixture.leftFace).selected = true;
    const kernel::math::Bounds selectedBounds = BoundsQuery::selected_bounds(mesh);
    if (!near_bounds(
            selectedBounds,
            glm::vec3{ -1.0f, -1.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 0.0f })) {
        return TestResult::fail("selected_bounds should include selected visible vertices, edges, and faces");
    }

    mesh.vertex(fixture.v4).hidden = true;
    const kernel::math::Bounds edgeBounds =
        BoundsQuery::edge_bounds(mesh, mesh.find_edge(fixture.v1, fixture.v4));
    if (!near_bounds(
            edgeBounds,
            glm::vec3{ 0.0f, -1.0f, 0.0f },
            glm::vec3{ 0.0f, -1.0f, 0.0f })) {
        return TestResult::fail("edge_bounds should skip hidden endpoints while keeping visible ones");
    }

    mesh.face(fixture.leftFace).hidden = true;
    if (BoundsQuery::face_bounds(mesh, fixture.leftFace).is_valid() ||
        BoundsQuery::vertex_bounds(mesh, VertexHandle{}).is_valid() ||
        BoundsQuery::edge_bounds(mesh, EdgeHandle{}).is_valid() ||
        BoundsQuery::face_bounds(mesh, FaceHandle{}).is_valid()) {
        return TestResult::fail("bounds queries should return empty bounds for hidden or invalid elements");
    }

    mesh.vertex(fixture.v4).hidden = false;
    mesh.face(fixture.leftFace).hidden = false;
    const kernel::math::Bounds faceBounds = BoundsQuery::face_bounds(mesh, fixture.rightFace);
    if (!near_bounds(
            faceBounds,
            glm::vec3{ 0.0f, -1.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 0.0f })) {
        return TestResult::fail("face_bounds should enclose visible vertices around the face boundary");
    }

    return TestResult::pass();
}

} // namespace locus::tests
