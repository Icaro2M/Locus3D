/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"

#include <glm/geometric.hpp>

#include <vector>

namespace {

[[nodiscard]] glm::vec3 triangle_normal(
    const locus::kernel::geometry::RenderMesh& mesh,
    const locus::kernel::geometry::RenderTriangle& triangle)
{
    return locus::kernel::geometry::NormalBuilder::triangle_normal(
        mesh.vertices[triangle.a].position,
        mesh.vertices[triangle.b].position,
        mesh.vertices[triangle.c].position);
}

[[nodiscard]] bool all_triangles_follow_normal(
    const locus::kernel::geometry::RenderMesh& mesh,
    const glm::vec3& normal)
{
    for (const locus::kernel::geometry::RenderTriangle& triangle :
        mesh.triangles) {
        if (glm::dot(triangle_normal(mesh, triangle), normal) < 0.999f) {
            return false;
        }
    }

    return !mesh.triangles.empty();
}

[[nodiscard]] bool has_inward_box_face(
    const locus::kernel::geometry::LEM& mesh,
    const glm::vec3& center)
{
    for (const locus::kernel::geometry::FaceHandle face :
        locus::kernel::geometry::TopologyTraversal::faces(mesh)) {
        glm::vec3 centroid{ 0.0f };
        const std::vector<locus::kernel::geometry::VertexHandle> vertices =
            locus::kernel::geometry::TopologyTraversal::face_vertices(
                mesh,
                face);

        for (const locus::kernel::geometry::VertexHandle vertex : vertices) {
            centroid += mesh.vertex(vertex).position;
        }

        centroid /= static_cast<float>(vertices.size());
        const glm::vec3 outward = glm::normalize(centroid - center);

        if (glm::dot(mesh.face(face).normal, outward) < -0.999f) {
            return true;
        }
    }

    return false;
}

} // namespace

namespace locus::tests {

TestResult run_face_orientation_render_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::modeling;

    {
        LEM mesh;
        const TopologyBuildResult build =
            TopologyBuilder::build_into(
                mesh,
                {
                    { -1.0f, -1.0f, 0.0f },
                    { 1.0f, -1.0f, 0.0f },
                    { 0.0f, 1.0f, 0.0f }
                },
                { { 0u, 1u, 2u } });

        const RenderMesh renderMesh = MeshTriangulator::triangulate(mesh);

        if (!build || renderMesh.triangles.size() != 1u ||
            !all_triangles_follow_normal(
                renderMesh,
                mesh.face(build.faces.front()).normal)) {
            return TestResult::fail(
                "Face Orientation triangle winding should reach render indices unchanged");
        }

        LEM reversed;
        const TopologyBuildResult reversedBuild =
            TopologyBuilder::build_into(
                reversed,
                {
                    { -1.0f, -1.0f, 0.0f },
                    { 1.0f, -1.0f, 0.0f },
                    { 0.0f, 1.0f, 0.0f }
                },
                { { 2u, 1u, 0u } });

        const RenderMesh reversedRender =
            MeshTriangulator::triangulate(reversed);

        if (!reversedBuild || reversedRender.triangles.size() != 1u ||
            glm::dot(
                triangle_normal(
                    renderMesh,
                    renderMesh.triangles.front()),
                triangle_normal(
                    reversedRender,
                    reversedRender.triangles.front())) > -0.999f) {
            return TestResult::fail(
                "Reversed triangle winding should invert raster front/back classification");
        }
    }

    {
        LEM mesh;
        const TopologyBuildResult build =
            TopologyBuilder::build_quad_into(
                mesh,
                { -1.0f, -1.0f, 0.0f },
                { 1.0f, -1.0f, 0.0f },
                { 1.0f, 1.0f, 0.0f },
                { -1.0f, 1.0f, 0.0f });

        const RenderMesh renderMesh = MeshTriangulator::triangulate(mesh);

        if (!build || renderMesh.triangles.size() != 2u ||
            !all_triangles_follow_normal(
                renderMesh,
                mesh.face(build.faces.front()).normal)) {
            return TestResult::fail(
                "Quad triangulation should keep both triangles on the same front side");
        }
    }

    {
        LEM mesh;
        const TopologyBuildResult build =
            TopologyBuilder::build_into(
                mesh,
                {
                    { -1.0f, -1.0f, 0.0f },
                    { 0.0f, -1.4f, 0.0f },
                    { 1.0f, -1.0f, 0.0f },
                    { 1.2f, 0.2f, 0.0f },
                    { 0.0f, 1.0f, 0.0f },
                    { -1.2f, 0.2f, 0.0f }
                },
                { { 0u, 1u, 2u, 3u, 4u, 5u } });

        const RenderMesh renderMesh = MeshTriangulator::triangulate(mesh);

        if (!build || renderMesh.triangles.size() != 4u ||
            !all_triangles_follow_normal(
                renderMesh,
                mesh.face(build.faces.front()).normal)) {
            return TestResult::fail(
                "N-gon triangulation should preserve face orientation for every derived triangle");
        }
    }

    {
        LEM mesh;
        const glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        const TopologyBuildResult build =
            TopologyBuilder::build_box_into(mesh, center);

        for (const FaceHandle face : build.faces) {
            glm::vec3 centroid{ 0.0f };
            const std::vector<VertexHandle> vertices =
                TopologyTraversal::face_vertices(mesh, face);

            for (const VertexHandle vertex : vertices) {
                centroid += mesh.vertex(vertex).position;
            }

            centroid /= static_cast<float>(vertices.size());
            const glm::vec3 outward = glm::normalize(centroid - center);

            if (glm::dot(mesh.face(face).normal, outward) < 0.999f) {
                return TestResult::fail(
                    "Box builder should emit outward face winding for external views");
            }
        }

        OperationContext context{};
        context.mesh = &mesh;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        FlipFaceOp flip{ build.faces.front() };
        const OperationResult result = flip.execute(context);

        if (!result.is_success() || !has_inward_box_face(mesh, center)) {
            return TestResult::fail(
                "Flip Faces should invert the render-visible orientation of the flipped box face");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
