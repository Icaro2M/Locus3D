/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/topology/DeleteMeshElementsOp.h"

namespace {

struct Counts {
    std::size_t vertices = 0u;
    std::size_t edges = 0u;
    std::size_t faces = 0u;
};

[[nodiscard]] Counts active_counts(
    const locus::kernel::geometry::LEM& mesh)
{
    return Counts{
        locus::kernel::geometry::TopologyTraversal::vertices(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::edges(mesh).size(),
        locus::kernel::geometry::TopologyTraversal::faces(mesh).size()
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

} // namespace

namespace locus::tests {

TestResult run_delete_mesh_elements_operation_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::modeling;

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const QuadMesh quad = make_quad(editor);

        OperationContext context = make_context(mesh);
        DeleteMeshElementsOp operation =
            DeleteMeshElementsOp::faces({ quad.face, quad.face });

        const OperationResult result = operation.execute(context);
        const Counts counts = active_counts(mesh);

        if (!result.is_success()
            || !result.changed()
            || counts.faces != 0u
            || counts.edges != 4u
            || counts.vertices != 4u) {
            return TestResult::fail(
                "Delete face should remove the face and preserve boundary edges and vertices");
        }

        if (!TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "Delete face result should pass topology validation");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const QuadMesh quad = make_quad(editor);
        const EdgeHandle edge =
            mesh.find_edge(quad.v0, quad.v1);

        OperationContext context = make_context(mesh);
        DeleteMeshElementsOp operation =
            DeleteMeshElementsOp::edges({ edge, edge });

        const OperationResult result = operation.execute(context);
        const Counts counts = active_counts(mesh);

        if (!result.is_success()
            || !result.changed()
            || counts.faces != 0u
            || counts.edges != 3u
            || counts.vertices != 4u
            || mesh.is_valid(edge)) {
            return TestResult::fail(
                "Delete edge should remove incident faces and the selected edge while preserving vertices");
        }

        if (!TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "Delete edge result should pass topology validation");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const QuadMesh quad = make_quad(editor);

        OperationContext context = make_context(mesh);
        DeleteMeshElementsOp operation =
            DeleteMeshElementsOp::vertices({ quad.v0, quad.v0 });

        const OperationResult result = operation.execute(context);
        const Counts counts = active_counts(mesh);

        if (!result.is_success()
            || !result.changed()
            || counts.faces != 0u
            || counts.edges != 2u
            || counts.vertices != 3u
            || mesh.is_valid(quad.v0)) {
            return TestResult::fail(
                "Delete vertex should remove incident faces, incident edges, and the selected vertex");
        }

        if (!TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "Delete vertex result should pass topology validation");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        (void)make_quad(editor);

        OperationContext context = make_context(mesh);
        DeleteMeshElementsOp operation =
            DeleteMeshElementsOp::faces({ FaceHandle{ 999u } });

        if (!operation.execute(context).is_failure()) {
            return TestResult::fail(
                "Delete operation should reject invalid handles");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
