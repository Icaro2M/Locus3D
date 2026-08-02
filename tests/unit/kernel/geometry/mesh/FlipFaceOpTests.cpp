/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <vector>

namespace {

struct Counts {
    std::size_t vertices = 0u;
    std::size_t edges = 0u;
    std::size_t faces = 0u;
    std::size_t loops = 0u;
};

[[nodiscard]] Counts active_counts(
    const locus::kernel::geometry::LEM& mesh)
{
    return Counts{
        locus::kernel::geometry::TopologyTraversal::vertices(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::edges(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::faces(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::loops(mesh).size()
    };
}

[[nodiscard]] locus::kernel::modeling::OperationContext make_context(
    locus::kernel::geometry::LEM& mesh)
{
    locus::kernel::modeling::OperationContext context{};
    context.mesh = &mesh;
    context.validateAfterExecute = true;
    context.rebuildNormals = true;
    context.allowNonManifold = true;
    return context;
}

[[nodiscard]] std::vector<locus::kernel::geometry::VertexHandle>
face_vertices(
    const locus::kernel::geometry::LEM& mesh,
    locus::kernel::geometry::FaceHandle face)
{
    return locus::kernel::geometry::TopologyTraversal::face_vertices(
        mesh,
        face);
}

[[nodiscard]] bool same_cycle(
    std::vector<locus::kernel::geometry::VertexHandle> lhs,
    std::vector<locus::kernel::geometry::VertexHandle> rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    const auto by_id = [](const auto left, const auto right) {
        return left.id.value < right.id.value;
    };

    std::sort(lhs.begin(), lhs.end(), by_id);
    std::sort(rhs.begin(), rhs.end(), by_id);
    return lhs == rhs;
}

[[nodiscard]] bool reversed_winding(
    const std::vector<locus::kernel::geometry::VertexHandle>& before,
    const std::vector<locus::kernel::geometry::VertexHandle>& after)
{
    if (before.size() != after.size() || before.empty()) {
        return false;
    }

    if (!same_cycle(before, after)) {
        return false;
    }

    const auto start =
        std::find(after.begin(), after.end(), before.front());

    if (start == after.end()) {
        return false;
    }

    for (std::size_t i = 0u; i < before.size(); ++i) {
        const std::size_t afterIndex =
            static_cast<std::size_t>(
                std::distance(after.begin(), start));
        const std::size_t expectedIndex =
            (before.size() - i) % before.size();

        if (after[(afterIndex + i) % after.size()]
            != before[expectedIndex]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool counts_equal(Counts lhs, Counts rhs)
{
    return lhs.vertices == rhs.vertices
        && lhs.edges == rhs.edges
        && lhs.faces == rhs.faces
        && lhs.loops == rhs.loops;
}

[[nodiscard]] locus::tests::QuadMesh make_offset_quad(
    locus::kernel::geometry::LEMEditor& editor,
    float offsetX)
{
    locus::tests::QuadMesh quad{};
    quad.v0 = editor.add_vertex(glm::vec3{ offsetX - 1.0f, -1.0f, 0.0f });
    quad.v1 = editor.add_vertex(glm::vec3{ offsetX + 1.0f, -1.0f, 0.0f });
    quad.v2 = editor.add_vertex(glm::vec3{ offsetX + 1.0f, 1.0f, 0.0f });
    quad.v3 = editor.add_vertex(glm::vec3{ offsetX - 1.0f, 1.0f, 0.0f });
    quad.face = editor.add_face({ quad.v0, quad.v1, quad.v2, quad.v3 });
    return quad;
}

} // namespace

namespace locus::tests {

TestResult run_flip_face_operation_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::modeling;

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const QuadMesh quad = make_quad(editor);
        const Counts countsBefore = active_counts(mesh);
        const std::vector<VertexHandle> verticesBefore =
            face_vertices(mesh, quad.face);
        const glm::vec3 normalBefore =
            NormalBuilder::face_normal(mesh, quad.face);

        OperationContext context = make_context(mesh);
        FlipFaceOp operation{ quad.face };
        const OperationResult result = operation.execute(context);

        const std::vector<VertexHandle> verticesAfter =
            face_vertices(mesh, quad.face);
        const glm::vec3 normalAfter =
            NormalBuilder::face_normal(mesh, quad.face);

        if (!result.is_success()
            || !result.changed()
            || !context.has_mesh()
            || !result.has_validation_report()
            || !result.validation_report().valid()) {
            return TestResult::fail(
                "FlipFaceOp should succeed and attach a valid topology report");
        }

        if (!mesh.is_valid(quad.face)
            || !counts_equal(countsBefore, active_counts(mesh))
            || !reversed_winding(verticesBefore, verticesAfter)
            || glm::dot(normalBefore, normalAfter) > -0.999f) {
            return TestResult::fail(
                "FlipFaceOp should preserve handles and counts while reversing winding and normal");
        }

        if (!TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "FlipFaceOp result should pass topology validation");
        }

        for (const EdgeHandle edge : TopologyTraversal::face_edges(
                 mesh,
                 quad.face)) {
            if (TopologyTraversal::edge_loops(mesh, edge).empty()) {
                return TestResult::fail(
                    "FlipFaceOp should keep radial cycles reachable");
            }
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const QuadMesh first = make_offset_quad(editor, -3.0f);
        const QuadMesh second = make_offset_quad(editor, 3.0f);
        const Counts countsBefore = active_counts(mesh);
        const std::array<std::vector<VertexHandle>, 2u> before{
            face_vertices(mesh, first.face),
            face_vertices(mesh, second.face)
        };

        OperationContext context = make_context(mesh);
        FlipFaceOp firstFlip{ first.face };
        FlipFaceOp secondFlip{ second.face };

        const OperationResult firstResult = firstFlip.execute(context);
        const OperationResult secondResult = secondFlip.execute(context);

        if (!firstResult.is_success()
            || !secondResult.is_success()
            || !counts_equal(countsBefore, active_counts(mesh))
            || !reversed_winding(
                before[0],
                face_vertices(mesh, first.face))
            || !reversed_winding(
                before[1],
                face_vertices(mesh, second.face))) {
            return TestResult::fail(
                "FlipFaceOp should support multiple selected faces through repeated operation callbacks");
        }

        if (!TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "Multiple FlipFaceOp executions should preserve topology invariants");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const QuadMesh quad = make_quad(editor);
        const Counts countsBefore = active_counts(mesh);
        const std::vector<VertexHandle> verticesBefore =
            face_vertices(mesh, quad.face);

        OperationContext context = make_context(mesh);
        FlipFaceOp operation{ FaceHandle{ 999u } };
        const OperationResult result = operation.execute(context);

        if (result.is_success()
            || !counts_equal(countsBefore, active_counts(mesh))
            || face_vertices(mesh, quad.face) != verticesBefore) {
            return TestResult::fail(
                "FlipFaceOp should reject invalid faces without changing topology");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
