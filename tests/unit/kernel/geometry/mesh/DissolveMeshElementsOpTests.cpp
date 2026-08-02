/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/topology/DissolveMeshElementsOp.h"

#include <glm/vec3.hpp>

#include <vector>

namespace {

struct Counts {
    std::size_t vertices = 0u;
    std::size_t edges = 0u;
    std::size_t faces = 0u;
    std::size_t loops = 0u;
};

struct TwoQuadMesh {
    locus::kernel::geometry::VertexHandle v0{};
    locus::kernel::geometry::VertexHandle v1{};
    locus::kernel::geometry::VertexHandle v2{};
    locus::kernel::geometry::VertexHandle v3{};
    locus::kernel::geometry::VertexHandle v4{};
    locus::kernel::geometry::VertexHandle v5{};
    locus::kernel::geometry::FaceHandle leftFace{};
    locus::kernel::geometry::FaceHandle rightFace{};
    locus::kernel::geometry::EdgeHandle sharedEdge{};
    locus::kernel::geometry::EdgeHandle boundaryEdge{};
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

[[nodiscard]] bool counts_equal(Counts lhs, Counts rhs)
{
    return lhs.vertices == rhs.vertices
        && lhs.edges == rhs.edges
        && lhs.faces == rhs.faces
        && lhs.loops == rhs.loops;
}

[[nodiscard]] locus::kernel::modeling::OperationContext make_context(
    locus::kernel::geometry::LEM& mesh)
{
    locus::kernel::modeling::OperationContext context{};
    context.mesh = &mesh;
    context.validateAfterExecute = true;
    context.rebuildNormals = true;
    context.allowNonManifold = false;
    return context;
}

[[nodiscard]] TwoQuadMesh make_two_quads(
    locus::kernel::geometry::LEMEditor& editor)
{
    TwoQuadMesh mesh{};
    mesh.v0 = editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    mesh.v1 = editor.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
    mesh.v2 = editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    mesh.v3 = editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });
    mesh.v4 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    mesh.v5 = editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });

    mesh.leftFace = editor.add_face({ mesh.v0, mesh.v1, mesh.v4, mesh.v3 });
    mesh.rightFace = editor.add_face({ mesh.v1, mesh.v2, mesh.v5, mesh.v4 });
    mesh.sharedEdge = editor.mesh().find_edge(mesh.v1, mesh.v4);
    mesh.boundaryEdge = editor.mesh().find_edge(mesh.v0, mesh.v1);
    return mesh;
}

} // namespace

namespace locus::tests {

TestResult run_dissolve_mesh_elements_operation_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::modeling;

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const TwoQuadMesh quads = make_two_quads(editor);

        OperationContext context = make_context(mesh);
        DissolveMeshElementsOp operation =
            DissolveMeshElementsOp::edges({ quads.sharedEdge });
        const OperationResult result = operation.execute(context);

        if (!result.is_success()
            || !result.changed()
            || !result.has_validation_report()
            || !result.validation_report().valid()) {
            return TestResult::fail(
                "Dissolve Edge should succeed and validate on two adjacent quads");
        }

        const std::vector<FaceHandle> faces =
            TopologyTraversal::faces(mesh);

        if (faces.size() != 1u
            || TopologyTraversal::edges(mesh).size() != 6u
            || TopologyTraversal::face_vertices(mesh, faces[0]).size() != 6u
            || mesh.is_valid(quads.sharedEdge)
            || !TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "Dissolve Edge should remove the shared edge and create one polygonal face");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const TwoQuadMesh quads = make_two_quads(editor);
        const Counts before = active_counts(mesh);

        OperationContext context = make_context(mesh);
        DissolveMeshElementsOp operation =
            DissolveMeshElementsOp::edges({ quads.boundaryEdge });
        const OperationResult result = operation.execute(context);

        if (result.is_success()
            || !counts_equal(before, active_counts(mesh))
            || !mesh.is_valid(quads.boundaryEdge)) {
            return TestResult::fail(
                "Dissolve Edge should reject boundary edges atomically");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const VertexHandle a =
            editor.add_vertex(glm::vec3{ -1.0f, 0.0f, 0.0f });
        const VertexHandle v =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle b =
            editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        (void)editor.find_or_create_edge(a, v);
        (void)editor.find_or_create_edge(v, b);

        OperationContext context = make_context(mesh);
        DissolveMeshElementsOp operation =
            DissolveMeshElementsOp::vertices({ v });
        const OperationResult result = operation.execute(context);

        if (!result.is_success()
            || !result.changed()
            || mesh.is_valid(v)
            || !mesh.is_valid(mesh.find_edge(a, b))
            || TopologyTraversal::vertices(mesh).size() != 2u
            || TopologyTraversal::edges(mesh).size() != 1u) {
            return TestResult::fail(
                "Dissolve Vertex should replace a loose two-edge chain with one edge");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const QuadMesh quad = make_quad(editor);
        const Counts before = active_counts(mesh);

        OperationContext context = make_context(mesh);
        DissolveMeshElementsOp operation =
            DissolveMeshElementsOp::faces({ quad.face });
        const OperationResult result = operation.execute(context);

        if (result.is_success()
            || !counts_equal(before, active_counts(mesh))
            || !mesh.is_valid(quad.face)) {
            return TestResult::fail(
                "Dissolve Face should not behave like Delete Face");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const VertexHandle a =
            editor.add_vertex(glm::vec3{ -1.0f, 0.0f, 0.0f });
        const VertexHandle b =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle c =
            editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const VertexHandle d =
            editor.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f });
        (void)editor.find_or_create_edge(a, b);
        (void)editor.find_or_create_edge(b, c);
        (void)editor.find_or_create_edge(c, d);

        OperationContext context = make_context(mesh);
        DissolveMeshElementsOp operation =
            DissolveMeshElementsOp::vertices({ b, b, c });
        const OperationResult result = operation.execute(context);

        if (!result.is_success()
            || TopologyTraversal::vertices(mesh).size() != 2u
            || TopologyTraversal::edges(mesh).size() != 1u
            || !mesh.is_valid(mesh.find_edge(a, d))) {
            return TestResult::fail(
                "Dissolve Vertex multi-selection should deduplicate and execute deterministically");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
