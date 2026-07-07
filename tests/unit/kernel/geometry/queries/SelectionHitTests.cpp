/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QueryTestSuite.h"

#include "kernel/geometry/queries/SelectionHit.h"

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

TestResult run_selection_hit_tests()
{
    using namespace kernel::geometry;

    const SelectionHit miss = SelectionHit::miss();
    if (miss.hit || miss.is_vertex() || miss.is_edge() || miss.is_loop() || miss.is_face()) {
        return TestResult::fail("SelectionHit::miss should not report any concrete hit type");
    }

    const VertexHandle vertex{ 7 };
    const SelectionHit vertexHit = SelectionHit::vertex_hit(
        vertex,
        2.0f,
        glm::vec3{ 1.0f, 2.0f, 3.0f },
        glm::vec3{ 0.0f, 0.0f, 1.0f });
    if (!vertexHit.is_vertex() ||
        vertexHit.is_edge() ||
        vertexHit.vertex != vertex ||
        vertexHit.type != LEMElementType::Vertex ||
        !near(vertexHit.distance, 2.0f) ||
        !near_vec3(vertexHit.position, glm::vec3{ 1.0f, 2.0f, 3.0f }) ||
        !near_vec3(vertexHit.normal, glm::vec3{ 0.0f, 0.0f, 1.0f })) {
        return TestResult::fail("vertex_hit should populate a valid vertex selection result");
    }

    const EdgeHandle edge{ 3 };
    const SelectionHit edgeHit = SelectionHit::edge_hit(edge, 1.5f, glm::vec3{ 0.0f, 1.0f, 0.0f });
    if (!edgeHit.is_edge() ||
        edgeHit.is_vertex() ||
        edgeHit.edge != edge ||
        edgeHit.type != LEMElementType::Edge ||
        !near(edgeHit.distance, 1.5f)) {
        return TestResult::fail("edge_hit should populate a valid edge selection result");
    }

    const LoopHandle loop{ 5 };
    const SelectionHit loopHit = SelectionHit::loop_hit(loop, 0.5f, glm::vec3{ 1.0f, 0.0f, 0.0f });
    if (!loopHit.is_loop() ||
        loopHit.loop != loop ||
        loopHit.type != LEMElementType::Loop) {
        return TestResult::fail("loop_hit should populate a valid loop selection result");
    }

    const FaceHandle face{ 11 };
    const SelectionHit faceHit = SelectionHit::face_hit(face, 4.0f, glm::vec3{ 2.0f, 0.0f, 0.0f });
    if (!faceHit.is_face() ||
        faceHit.face != face ||
        faceHit.type != LEMElementType::Face ||
        !near_vec3(faceHit.normal, glm::vec3{ 0.0f, 1.0f, 0.0f })) {
        return TestResult::fail("face_hit should populate a valid face selection result with fallback normal");
    }

    if (SelectionHit::vertex_hit(VertexHandle{}, 1.0f, glm::vec3{ 0.0f }).hit ||
        SelectionHit::edge_hit(EdgeHandle{}, 1.0f, glm::vec3{ 0.0f }).hit ||
        SelectionHit::loop_hit(LoopHandle{}, 1.0f, glm::vec3{ 0.0f }).hit ||
        SelectionHit::face_hit(FaceHandle{}, 1.0f, glm::vec3{ 0.0f }).hit) {
        return TestResult::fail("SelectionHit factories should mark invalid handles as misses");
    }

    return TestResult::pass();
}

} // namespace locus::tests
