/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/topology/BridgeEdgeOp.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace {

struct BridgeFixture {
    locus::kernel::geometry::VertexHandle a0{};
    locus::kernel::geometry::VertexHandle a1{};
    locus::kernel::geometry::VertexHandle b0{};
    locus::kernel::geometry::VertexHandle b1{};
    locus::kernel::geometry::EdgeHandle firstEdge{};
    locus::kernel::geometry::EdgeHandle secondEdge{};
};

[[nodiscard]] BridgeFixture make_open_edge_fixture(
    locus::kernel::geometry::LEMEditor& editor)
{
    BridgeFixture fixture{};
    fixture.a0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    fixture.a1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    fixture.b0 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    fixture.b1 = editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
    fixture.firstEdge =
        editor.find_or_create_edge(fixture.a0, fixture.a1);
    fixture.secondEdge =
        editor.find_or_create_edge(fixture.b0, fixture.b1);
    return fixture;
}

[[nodiscard]] BridgeFixture make_reversed_open_edge_fixture(
    locus::kernel::geometry::LEMEditor& editor)
{
    BridgeFixture fixture{};
    fixture.a0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    fixture.a1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    fixture.b0 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    fixture.b1 = editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
    fixture.firstEdge =
        editor.find_or_create_edge(fixture.a0, fixture.a1);
    fixture.secondEdge =
        editor.find_or_create_edge(fixture.b1, fixture.b0);
    return fixture;
}

[[nodiscard]] bool face_vertices_equal(
    const locus::kernel::geometry::LEM& mesh,
    locus::kernel::geometry::FaceHandle face,
    const std::vector<locus::kernel::geometry::VertexHandle>& expected)
{
    const std::vector<locus::kernel::geometry::VertexHandle> vertices =
        locus::kernel::geometry::TopologyTraversal::face_vertices(
            mesh,
            face);

    if (vertices.size() != expected.size()) {
        return false;
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (vertices[i] != expected[i]) {
            return false;
        }
    }

    return true;
}

} // namespace

namespace locus::tests {

TestResult run_bridge_edge_operation_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::modeling;

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const BridgeFixture fixture =
            make_open_edge_fixture(editor);

        OperationContext context{};
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        BridgeEdgeOp operation =
            BridgeEdgeOp::edges(
                { fixture.firstEdge },
                { fixture.secondEdge });
        operation.set_closed(false);

        const OperationResult result =
            operation.execute(context);

        if (!result.is_success() || !result.changed()) {
            return TestResult::fail(
                "BridgeEdgeOp should bridge two compatible boundary edges");
        }

        const std::vector<FaceHandle> faces =
            TopologyTraversal::faces(mesh);

        if (faces.size() != 1u) {
            return TestResult::fail(
                "BridgeEdgeOp should create exactly one bridge face");
        }

        if (!face_vertices_equal(
                mesh,
                faces.front(),
                {
                    fixture.a0,
                    fixture.a1,
                    fixture.b1,
                    fixture.b0
                })) {
            return TestResult::fail(
                "BridgeEdgeOp should create a face spanning the two edges");
        }

        if (!TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "BridgeEdgeOp result should pass topology validation");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const BridgeFixture fixture =
            make_reversed_open_edge_fixture(editor);

        OperationContext context{};
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        BridgeEdgeOp operation =
            BridgeEdgeOp::edges(
                { fixture.firstEdge },
                { fixture.secondEdge });
        operation.set_closed(false);

        const OperationResult result =
            operation.execute(context);

        if (!result.is_success() || !result.changed()) {
            return TestResult::fail(
                "BridgeEdgeOp should bridge reversed single boundary edges");
        }

        const std::vector<FaceHandle> faces =
            TopologyTraversal::faces(mesh);

        if (faces.size() != 1u) {
            return TestResult::fail(
                "BridgeEdgeOp reversed fixture should create one bridge face");
        }

        if (!face_vertices_equal(
                mesh,
                faces.front(),
                {
                    fixture.a0,
                    fixture.a1,
                    fixture.b1,
                    fixture.b0
                })) {
            return TestResult::fail(
                "BridgeEdgeOp should orient reversed edges into a non-crossing quad");
        }

        if (!TopologyValidator::validate(mesh).valid()) {
            return TestResult::fail(
                "BridgeEdgeOp reversed result should pass topology validation");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);

        const VertexHandle a0 =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle a1 =
            editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const VertexHandle a2 =
            editor.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
        const VertexHandle b0 =
            editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
        const VertexHandle b1 =
            editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
        const VertexHandle b2 =
            editor.add_vertex(glm::vec3{ 1.0f, 2.0f, 0.0f });

        const FaceHandle firstAdjacent =
            editor.add_face({ a0, a1, a2 });
        const FaceHandle secondAdjacent =
            editor.add_face({ b0, b1, b2 });
        const EdgeHandle firstEdge = mesh.find_edge(a0, a1);
        const EdgeHandle secondEdge = mesh.find_edge(b0, b1);

        OperationContext context{};
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;

        BridgeEdgeOp operation =
            BridgeEdgeOp::edges({ firstEdge }, { secondEdge });
        operation.set_closed(false);

        const OperationResult result =
            operation.execute(context);
        const std::vector<FaceHandle> faces =
            TopologyTraversal::faces(mesh);

        if (!mesh.is_valid(firstAdjacent) ||
            !mesh.is_valid(secondAdjacent) ||
            !result.is_success() ||
            faces.size() != 3u ||
            glm::dot(mesh.face(firstAdjacent).normal, mesh.face(faces.back()).normal) <= 0.0f ||
            glm::dot(mesh.face(secondAdjacent).normal, mesh.face(faces.back()).normal) <= 0.0f) {
            return TestResult::fail(
                "BridgeEdgeOp should orient bridge faces with adjacent boundary normals");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);
        const VertexHandle v0 =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle v1 =
            editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const VertexHandle v2 =
            editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
        const VertexHandle v3 =
            editor.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
        const VertexHandle b0 =
            editor.add_vertex(glm::vec3{ 0.0f, 2.0f, 0.0f });
        const VertexHandle b1 =
            editor.add_vertex(glm::vec3{ 1.0f, 2.0f, 0.0f });

        const FaceHandle firstFace =
            editor.add_face({ v0, v1, v2 });
        const FaceHandle secondFace =
            editor.add_face({ v1, v0, v3 });
        const EdgeHandle interiorEdge =
            mesh.find_edge(v0, v1);
        const EdgeHandle boundaryEdge =
            editor.find_or_create_edge(b0, b1);

        if (!mesh.is_valid(firstFace) ||
            !mesh.is_valid(secondFace) ||
            !mesh.is_valid(interiorEdge) ||
            !mesh.is_valid(boundaryEdge)) {
            return TestResult::fail(
                "BridgeEdgeOp incompatible fixture should be valid");
        }

        const std::size_t faceCountBefore =
            TopologyTraversal::faces(mesh).size();

        OperationContext context{};
        context.mesh = &mesh;
        context.validateAfterExecute = true;

        BridgeEdgeOp operation =
            BridgeEdgeOp::edges(
                { interiorEdge },
                { boundaryEdge });
        operation.set_closed(false);

        const OperationResult result =
            operation.execute(context);

        if (!result.is_failure() ||
            TopologyTraversal::faces(mesh).size() != faceCountBefore) {
            return TestResult::fail(
                "BridgeEdgeOp should reject non-boundary edges without change");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
