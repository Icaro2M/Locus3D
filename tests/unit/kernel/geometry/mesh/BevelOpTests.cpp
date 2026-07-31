/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/edge/BevelOp.h"

#include <glm/geometric.hpp>

#include <vector>

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

[[nodiscard]] bool mesh_has_vertex_at(
    const locus::kernel::geometry::LEM& mesh,
    const glm::vec3& position)
{
    using locus::kernel::geometry::TopologyTraversal;

    for (const auto vertex : TopologyTraversal::vertices(mesh)) {
        if (almost_equal(mesh.vertex(vertex).position, position)) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool mesh_has_edge_at(
    const locus::kernel::geometry::LEM& mesh,
    const glm::vec3& first,
    const glm::vec3& second)
{
    using locus::kernel::geometry::TopologyTraversal;

    for (const auto edge : TopologyTraversal::edges(mesh)) {
        const auto vertices =
            TopologyTraversal::edge_vertices(
                mesh,
                edge);

        if (!mesh.is_valid(vertices[0]) ||
            !mesh.is_valid(vertices[1])) {
            continue;
        }

        const glm::vec3 firstPosition =
            mesh.vertex(vertices[0]).position;
        const glm::vec3 secondPosition =
            mesh.vertex(vertices[1]).position;

        if ((almost_equal(firstPosition, first) &&
                almost_equal(secondPosition, second)) ||
            (almost_equal(firstPosition, second) &&
                almost_equal(secondPosition, first))) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool face_has_vertices_at(
    const locus::kernel::geometry::LEM& mesh,
    locus::kernel::geometry::FaceHandle face,
    const std::vector<glm::vec3>& positions)
{
    using locus::kernel::geometry::TopologyTraversal;

    const std::vector<locus::kernel::geometry::VertexHandle> vertices =
        TopologyTraversal::face_vertices(
            mesh,
            face);

    if (vertices.size() != positions.size()) {
        return false;
    }

    for (const glm::vec3& position : positions) {
        bool found = false;

        for (const auto vertex : vertices) {
            if (almost_equal(mesh.vertex(vertex).position, position)) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool mesh_has_face_at(
    const locus::kernel::geometry::LEM& mesh,
    const std::vector<glm::vec3>& positions)
{
    using locus::kernel::geometry::TopologyTraversal;

    for (const auto face : TopologyTraversal::faces(mesh)) {
        if (face_has_vertices_at(mesh, face, positions)) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] locus::kernel::geometry::FaceHandle mesh_face_at(
    const locus::kernel::geometry::LEM& mesh,
    const std::vector<glm::vec3>& positions)
{
    using locus::kernel::geometry::TopologyTraversal;

    for (const auto face : TopologyTraversal::faces(mesh)) {
        if (face_has_vertices_at(mesh, face, positions)) {
            return face;
        }
    }

    return {};
}

} // namespace

namespace locus::tests {

TestResult run_bevel_operation_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::modeling;

    {
        LEM mesh;
        LEMEditor editor(mesh);

        const VertexHandle v0 =
            editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
        const VertexHandle v1 =
            editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
        const VertexHandle v2 =
            editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
        const VertexHandle v3 =
            editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });
        const FaceHandle face =
            editor.add_face({ v0, v1, v2, v3 });
        const EdgeHandle edge =
            mesh.find_edge(v0, v1);

        if (!mesh.is_valid(face) || !mesh.is_valid(edge)) {
            return TestResult::fail(
                "boundary bevel fixture should be valid");
        }

        OperationContext context{};
        context.mesh = &mesh;

        BevelOp operation(edge, 0.25f);
        const OperationResult result =
            operation.execute(context);

        if (!result.is_success() || !result.changed()) {
            return TestResult::fail(
                "boundary bevel should produce a mesh change");
        }

        if (mesh.is_valid(edge)) {
            return TestResult::fail(
                "boundary bevel should remove the original selected edge");
        }

        if (mesh.is_valid(v0) || mesh.is_valid(v1)) {
            return TestResult::fail(
                "boundary bevel should remove loose original edge vertices");
        }

        if (mesh_has_edge_at(
            mesh,
            glm::vec3{ -1.0f, -1.0f, 0.0f },
            glm::vec3{ -1.0f, 1.0f, 0.0f }) ||
            mesh_has_edge_at(
                mesh,
                glm::vec3{ 1.0f, -1.0f, 0.0f },
                glm::vec3{ 1.0f, 1.0f, 0.0f })) {
            return TestResult::fail(
                "boundary bevel should remove loose original side edges");
        }

        if (TopologyTraversal::faces(mesh).size() != 1) {
            return TestResult::fail(
                "boundary bevel should keep one active face");
        }

        if (!mesh_has_vertex_at(mesh, glm::vec3{ -1.0f, -0.75f, 0.0f }) ||
            !mesh_has_vertex_at(mesh, glm::vec3{ 1.0f, -0.75f, 0.0f })) {
            return TestResult::fail(
                "boundary bevel should shift the boundary edge into the face");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);

        const VertexHandle v0 =
            editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
        const VertexHandle v1 =
            editor.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
        const VertexHandle v2 =
            editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
        const VertexHandle v3 =
            editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });
        const VertexHandle v4 =
            editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
        const VertexHandle v5 =
            editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });

        const FaceHandle leftFace =
            editor.add_face({ v0, v1, v2, v3 });
        const FaceHandle rightFace =
            editor.add_face({ v1, v4, v5, v2 });
        const EdgeHandle edge =
            mesh.find_edge(v1, v2);

        if (!mesh.is_valid(leftFace) ||
            !mesh.is_valid(rightFace) ||
            !mesh.is_valid(edge)) {
            return TestResult::fail(
                "shared edge bevel fixture should be valid");
        }

        OperationContext context{};
        context.mesh = &mesh;

        BevelOp operation(edge, 0.25f);
        const OperationResult result =
            operation.execute(context);

        if (!result.is_success() || !result.changed()) {
            return TestResult::fail(
                "shared edge bevel should produce a mesh change");
        }

        if (mesh.is_valid(edge)) {
            return TestResult::fail(
                "shared edge bevel should remove the original selected edge");
        }

        if (mesh.is_valid(v1) || mesh.is_valid(v2)) {
            return TestResult::fail(
                "shared edge bevel should remove loose original edge vertices");
        }

        if (mesh_has_edge_at(
            mesh,
            glm::vec3{ 0.0f, -1.0f, 0.0f },
            glm::vec3{ -1.0f, -1.0f, 0.0f }) ||
            mesh_has_edge_at(
                mesh,
                glm::vec3{ 0.0f, -1.0f, 0.0f },
                glm::vec3{ 1.0f, -1.0f, 0.0f }) ||
            mesh_has_edge_at(
                mesh,
                glm::vec3{ 0.0f, 1.0f, 0.0f },
                glm::vec3{ -1.0f, 1.0f, 0.0f }) ||
            mesh_has_edge_at(
                mesh,
                glm::vec3{ 0.0f, 1.0f, 0.0f },
                glm::vec3{ 1.0f, 1.0f, 0.0f })) {
            return TestResult::fail(
                "shared edge bevel should remove loose original side edges");
        }

        if (TopologyTraversal::faces(mesh).size() != 3) {
            return TestResult::fail(
                "shared edge bevel should create two side faces and one bevel face");
        }

        const FaceHandle bevelFace =
            mesh_face_at(
                mesh,
                {
                glm::vec3{ -0.25f, -1.0f, 0.0f },
                glm::vec3{ -0.25f, 1.0f, 0.0f },
                glm::vec3{ 0.25f, 1.0f, 0.0f },
                glm::vec3{ 0.25f, -1.0f, 0.0f }
                });

        if (!mesh.is_valid(bevelFace)) {
            return TestResult::fail(
                "shared edge bevel should create a real chamfer face");
        }

        if (mesh.face(bevelFace).normal.z <= 0.0f) {
            return TestResult::fail(
                "shared edge bevel should orient the chamfer face normal consistently");
        }
    }

    {
        LEM mesh;
        LEMEditor editor(mesh);

        const VertexHandle bottom =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle top =
            editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 1.0f });
        const VertexHandle leftBottom =
            editor.add_vertex(glm::vec3{ -1.0f, 0.0f, 0.0f });
        const VertexHandle leftTop =
            editor.add_vertex(glm::vec3{ -1.0f, 0.0f, 1.0f });
        const VertexHandle rightBottom =
            editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
        const VertexHandle rightTop =
            editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 1.0f });
        const VertexHandle capBottom =
            editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });
        const VertexHandle capTop =
            editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 1.0f });

        const FaceHandle leftFace =
            editor.add_face({ bottom, leftBottom, leftTop, top });
        const FaceHandle rightFace =
            editor.add_face({ bottom, top, rightTop, rightBottom });
        const FaceHandle bottomFace =
            editor.add_face({ bottom, rightBottom, capBottom, leftBottom });
        const FaceHandle topFace =
            editor.add_face({ top, leftTop, capTop, rightTop });
        const EdgeHandle edge =
            mesh.find_edge(bottom, top);

        if (!mesh.is_valid(leftFace) ||
            !mesh.is_valid(rightFace) ||
            !mesh.is_valid(bottomFace) ||
            !mesh.is_valid(topFace) ||
            !mesh.is_valid(edge)) {
            return TestResult::fail(
                "perpendicular cap bevel fixture should be valid");
        }

        OperationContext context{};
        context.mesh = &mesh;

        BevelOp operation(edge, 0.25f);
        const OperationResult result =
            operation.execute(context);

        if (!result.is_success() || !result.changed()) {
            return TestResult::fail(
                "perpendicular cap bevel should produce a mesh change");
        }

        if (mesh.is_valid(bottom) || mesh.is_valid(top)) {
            return TestResult::fail(
                "perpendicular cap bevel should remove original edge endpoints");
        }

        if (mesh_has_edge_at(
            mesh,
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ -1.0f, 0.0f, 0.0f }) ||
            mesh_has_edge_at(
                mesh,
                glm::vec3{ 0.0f, 0.0f, 0.0f },
                glm::vec3{ 0.0f, 1.0f, 0.0f }) ||
            mesh_has_edge_at(
                mesh,
                glm::vec3{ 0.0f, 0.0f, 1.0f },
                glm::vec3{ -1.0f, 0.0f, 1.0f }) ||
            mesh_has_edge_at(
                mesh,
                glm::vec3{ 0.0f, 0.0f, 1.0f },
                glm::vec3{ 0.0f, 1.0f, 1.0f })) {
            return TestResult::fail(
                "perpendicular cap bevel should remove original cap corner edges");
        }

        if (TopologyTraversal::faces(mesh).size() != 5) {
            return TestResult::fail(
                "perpendicular cap bevel should rebuild side/cap faces and create one bevel face");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
