/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include <glm/geometric.hpp>

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

TestResult run_lem_topology_tests()
{
    using namespace kernel::geometry;

    LEM mesh;
    if (!mesh.empty()) {
        return TestResult::fail("new LEM should start empty");
    }

    const VertexHandle v0 = mesh.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
    const VertexHandle v3 = mesh.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });

    if (!mesh.is_valid(v0) || !mesh.is_valid(v1) ||
        !mesh.is_valid(v2) || !mesh.is_valid(v3)) {
        return TestResult::fail("added vertices should be valid");
    }

    if (!almost_equal(mesh.vertex(v2).position, glm::vec3{ 1.0f, 1.0f, 0.0f })) {
        return TestResult::fail("add_vertex should store object-space position");
    }

    const EdgeHandle edge = mesh.find_or_create_edge(v0, v1);
    if (!edge.is_valid() || !mesh.is_valid(edge)) {
        return TestResult::fail("find_or_create_edge should create a valid edge");
    }

    if (mesh.find_or_create_edge(v1, v0) != edge || mesh.find_edge(v0, v1) != edge) {
        return TestResult::fail("edges should be reused independent of endpoint order");
    }

    if (mesh.find_or_create_edge(v0, v0).is_valid()) {
        return TestResult::fail("self edges should be rejected");
    }

    if (mesh.add_face({ v0, v1 }).is_valid() ||
        mesh.add_face({ v0, v1, v1 }).is_valid() ||
        mesh.add_face({ v0, v1, v2, v0 }).is_valid()) {
        return TestResult::fail("invalid face boundaries should be rejected");
    }

    const FaceHandle face = mesh.add_face({ v0, v1, v2, v3 });
    if (!face.is_valid() || !mesh.is_valid(face)) {
        return TestResult::fail("valid quad face should be created");
    }

    if (mesh.vertex_count() != 4 ||
        mesh.edge_count() != 4 ||
        mesh.loop_count() != 4 ||
        mesh.face_count() != 1) {
        return TestResult::fail("quad should have expected element counts");
    }

    const std::vector<LoopHandle> loops = mesh.face_loops(face);
    if (loops.size() != 4) {
        return TestResult::fail("face_loops should return all quad boundary loops");
    }

    for (std::size_t i = 0; i < loops.size(); ++i) {
        const LoopHandle current = loops[i];
        const LoopHandle next = loops[(i + 1) % loops.size()];
        const LoopHandle previous = loops[(i + loops.size() - 1) % loops.size()];
        const Loop& loop = mesh.loop(current);

        if (loop.face != face || loop.next != next || loop.previous != previous) {
            return TestResult::fail("face loop cycle should be linked in boundary order");
        }
    }

    if (!almost_equal(mesh.face(face).normal, glm::vec3{ 0.0f, 0.0f, 1.0f })) {
        return TestResult::fail("quad normal should follow face winding");
    }

    const FaceHandle reverseFace = mesh.add_face({ v3, v2, v1, v0 });
    if (!reverseFace.is_valid()) {
        return TestResult::fail("opposite winding face should be accepted");
    }

    const EdgeHandle sharedEdge = mesh.find_edge(v0, v1);
    const LoopHandle firstLoop = mesh.edge(sharedEdge).loop;
    if (!firstLoop.is_valid() ||
        mesh.loop(firstLoop).radialNext == firstLoop ||
        mesh.loop(firstLoop).radialPrevious == firstLoop) {
        return TestResult::fail("shared edge should have a multi-loop radial cycle");
    }

    mesh.clear();
    if (!mesh.empty() || mesh.vertex_count() != 0 || mesh.face_count() != 0) {
        return TestResult::fail("clear should remove all LEM element slots");
    }

    return TestResult::pass();
}

} // namespace locus::tests
