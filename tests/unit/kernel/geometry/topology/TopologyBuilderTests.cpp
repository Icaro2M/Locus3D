/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TopologyTestSuite.h"

#include "kernel/geometry/topology/TopologyBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/math/Bounds.h"

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

} // namespace

namespace locus::tests {

TestResult run_topology_builder_tests()
{
    using namespace kernel::geometry;

    LEM mesh;
    const TopologyBuildResult invalidResult =
        TopologyBuilder::build_into(mesh, {}, { { 0, 1, 2 } });
    if (invalidResult ||
        !invalidResult.empty() ||
        !invalidResult.diff.empty() ||
        !mesh.empty()) {
        return TestResult::fail("TopologyBuilder should reject empty or invalid input without mutating the mesh");
    }

    const TopologyBuildResult duplicateIndexResult =
        TopologyBuilder::build_into(
            mesh,
            { glm::vec3{ 0.0f }, glm::vec3{ 1.0f }, glm::vec3{ 2.0f } },
            { { 0, 1, 1 } });
    if (duplicateIndexResult || !mesh.empty()) {
        return TestResult::fail("TopologyBuilder should reject duplicate or adjacent repeated face indices");
    }

    const TopologyBuildResult quad = TopologyBuilder::build_quad_into(
        mesh,
        glm::vec3{ -1.0f, -1.0f, 0.0f },
        glm::vec3{ 1.0f, -1.0f, 0.0f },
        glm::vec3{ 1.0f, 1.0f, 0.0f },
        glm::vec3{ -1.0f, 1.0f, 0.0f });
    if (!quad ||
        quad.empty() ||
        quad.vertices.size() != 4 ||
        quad.edges.size() != 4 ||
        quad.faces.size() != 1 ||
        quad.diff.empty() ||
        mesh.vertex_count() != 4 ||
        mesh.edge_count() != 4 ||
        mesh.loop_count() != 4 ||
        mesh.face_count() != 1 ||
        !near_vec3(mesh.face(quad.faces[0]).normal, glm::vec3{ 0.0f, 0.0f, 1.0f })) {
        return TestResult::fail("build_quad_into should append one valid quad and report created elements");
    }

    const TopologyBuildResult triangle = TopologyBuilder::build_into(
        mesh,
        {
            glm::vec3{ 3.0f, 0.0f, 0.0f },
            glm::vec3{ 4.0f, 0.0f, 0.0f },
            glm::vec3{ 3.0f, 1.0f, 0.0f },
        },
        { { 0, 1, 2 } });
    if (!triangle ||
        triangle.vertices.size() != 3 ||
        triangle.edges.size() != 3 ||
        triangle.faces.size() != 1 ||
        mesh.vertex_count() != 7 ||
        mesh.face_count() != 2) {
        return TestResult::fail("build_into should append topology to existing meshes");
    }

    const LEM builtQuad = TopologyBuilder::build_quad(
        glm::vec3{ -1.0f, -1.0f, 0.0f },
        glm::vec3{ 1.0f, -1.0f, 0.0f },
        glm::vec3{ 1.0f, 1.0f, 0.0f },
        glm::vec3{ -1.0f, 1.0f, 0.0f });
    if (builtQuad.vertex_count() != 4 ||
        builtQuad.edge_count() != 4 ||
        builtQuad.face_count() != 1) {
        return TestResult::fail("build_quad should return a new mesh containing one quad");
    }

    const TopologyBuildResult box = TopologyBuilder::build_box_into(
        mesh,
        glm::vec3{ 10.0f, 0.0f, 0.0f },
        glm::vec3{ 2.0f, 4.0f, 6.0f });
    if (!box ||
        box.vertices.size() != 8 ||
        box.edges.size() != 12 ||
        box.faces.size() != 6 ||
        box.diff.empty()) {
        return TestResult::fail("build_box_into should append a six-face box with shared edges");
    }

    const LEM invalidBox = TopologyBuilder::build_box(glm::vec3{ 0.0f }, glm::vec3{ 1.0f, 0.0f, 1.0f });
    if (!invalidBox.empty()) {
        return TestResult::fail("build_box should reject non-positive dimensions");
    }

    const LEM builtBox = TopologyBuilder::build_box(
        glm::vec3{ 1.0f, 2.0f, 3.0f },
        glm::vec3{ 2.0f, 2.0f, 2.0f });
    kernel::math::Bounds bounds = kernel::math::Bounds::empty();
    for (VertexHandle vertex : TopologyTraversal::vertices(builtBox)) {
        bounds.expand(builtBox.vertex(vertex).position);
    }
    if (builtBox.vertex_count() != 8 ||
        builtBox.edge_count() != 12 ||
        builtBox.face_count() != 6 ||
        !bounds.is_valid() ||
        !near_vec3(bounds.min, glm::vec3{ 0.0f, 1.0f, 2.0f }) ||
        !near_vec3(bounds.max, glm::vec3{ 2.0f, 3.0f, 4.0f })) {
        return TestResult::fail("build_box should return a new mesh with expected extents and topology counts");
    }

    return TestResult::pass();
}

} // namespace locus::tests
