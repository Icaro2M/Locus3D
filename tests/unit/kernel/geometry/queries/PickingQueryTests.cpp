/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QueryTestSuite.h"

#include "kernel/geometry/queries/ProximityQuery.h"
#include "kernel/geometry/queries/RaycastQuery.h"
#include "kernel/geometry/queries/SelectionQuery.h"
#include "kernel/math/Ray.h"

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

TestResult run_picking_query_tests()
{
    using namespace kernel::geometry;
    using namespace kernel::math;

    QueryMesh fixture = make_query_mesh();
    LEM& mesh = fixture.mesh;
    const EdgeHandle bottomRightEdge = mesh.find_edge(fixture.v1, fixture.v4);

    const SelectionHit vertexHit =
        ProximityQuery::closest_vertex(mesh, glm::vec3{ -1.0f, -1.0f, 0.2f }, 0.3f);
    if (!vertexHit.is_vertex() ||
        vertexHit.vertex != fixture.v0 ||
        !near(vertexHit.distance, 0.2f) ||
        ProximityQuery::closest_vertex(mesh, glm::vec3{ -1.0f, -1.0f, 0.2f }, 0.1f).hit) {
        return TestResult::fail("closest_vertex should return the nearest visible vertex within maxDistance");
    }

    const SelectionHit edgeHit =
        ProximityQuery::closest_edge(mesh, glm::vec3{ 0.5f, -1.0f, 0.25f }, 0.5f);
    if (!edgeHit.is_edge() ||
        edgeHit.edge != bottomRightEdge ||
        !near(edgeHit.distance, 0.25f) ||
        !near_vec3(edgeHit.position, glm::vec3{ 0.5f, -1.0f, 0.0f })) {
        return TestResult::fail("closest_edge should return the nearest point on a visible edge");
    }

    const SelectionHit faceHit =
        ProximityQuery::closest_face(mesh, glm::vec3{ 0.5f, 0.0f, 0.3f }, 0.5f);
    if (!faceHit.is_face() ||
        faceHit.face != fixture.rightFace ||
        !near(faceHit.distance, 0.3f) ||
        !near_vec3(faceHit.position, glm::vec3{ 0.5f, 0.0f, 0.0f })) {
        return TestResult::fail("closest_face should test polygon faces as a triangle fan");
    }

    mesh.vertex(fixture.v0).hidden = true;
    if (ProximityQuery::closest_vertex(mesh, glm::vec3{ -1.0f, -1.0f, 0.0f }, 0.01f).hit) {
        return TestResult::fail("proximity queries should ignore hidden vertices");
    }
    mesh.vertex(fixture.v0).hidden = false;

    const Ray faceRay{ glm::vec3{ 0.5f, 0.0f, 2.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f } };
    const SelectionHit rayFaceHit = RaycastQuery::raycast_faces(mesh, faceRay);
    if (!rayFaceHit.is_face() ||
        rayFaceHit.face != fixture.rightFace ||
        !near(rayFaceHit.distance, 2.0f) ||
        !near_vec3(rayFaceHit.position, glm::vec3{ 0.5f, 0.0f, 0.0f }) ||
        RaycastQuery::raycast_faces(mesh, faceRay, 1.0f).hit) {
        return TestResult::fail("raycast_faces should return the nearest face hit and honor maxDistance");
    }

    const Ray vertexRay{ glm::vec3{ 0.0f, -1.0f, 2.0f }, glm::vec3{ 0.0f, 0.0f, -2.0f } };
    const SelectionHit rayVertexHit = RaycastQuery::raycast_vertices(mesh, vertexRay, 0.01f);
    if (!rayVertexHit.is_vertex() ||
        rayVertexHit.vertex != fixture.v1 ||
        !near(rayVertexHit.distance, 2.0f) ||
        RaycastQuery::raycast_vertices(mesh, vertexRay, 0.0f).hit) {
        return TestResult::fail("raycast_vertices should use a pick radius around the normalized ray");
    }

    const Ray edgeRay{ glm::vec3{ 0.5f, -1.0f, 2.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f } };
    const SelectionHit rayEdgeHit = RaycastQuery::raycast_edges(mesh, edgeRay, 0.01f);
    if (!rayEdgeHit.is_edge() ||
        rayEdgeHit.edge != bottomRightEdge ||
        !near(rayEdgeHit.distance, 2.0f) ||
        !near_vec3(rayEdgeHit.position, glm::vec3{ 0.5f, -1.0f, 0.0f }) ||
        RaycastQuery::raycast_edges(mesh, edgeRay, 0.0f).hit) {
        return TestResult::fail("raycast_edges should use a pick radius against edge segments");
    }

    const Ray missRay{ glm::vec3{ 0.5f, 0.0f, 2.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f } };
    if (RaycastQuery::raycast_element(mesh, missRay, 0.1f, 0.1f).hit ||
        !RaycastQuery::raycast_mesh_bounds(mesh, faceRay).hit) {
        return TestResult::fail("raycast helpers should miss invalid rays and hit visible mesh bounds");
    }

    SelectionQueryOptions options;
    options.mask = SelectionElementMask::Face;
    const SelectionHit selectedFace = SelectionQuery::pick_by_ray(mesh, faceRay, options);
    if (!selectedFace.is_face() || selectedFace.face != fixture.rightFace) {
        return TestResult::fail("pick_by_ray should honor a face-only selection mask");
    }

    options.mask = SelectionElementMask::Vertex;
    options.vertexRadius = 0.01f;
    const SelectionHit preferredVertex = SelectionQuery::pick_by_ray(mesh, vertexRay, options);
    if (!preferredVertex.is_vertex() || preferredVertex.vertex != fixture.v1) {
        return TestResult::fail("pick_by_ray should honor a vertex-only selection mask");
    }

    options.mask = SelectionElementMask::Edge;
    const SelectionHit selectedEdge = SelectionQuery::pick_by_point(
        mesh,
        glm::vec3{ 0.5f, -1.0f, 0.25f },
        options);
    if (!selectedEdge.is_edge() || selectedEdge.edge != bottomRightEdge) {
        return TestResult::fail("pick_by_point should honor an edge-only selection mask");
    }

    if (!has_selection_mask(SelectionElementMask::All, SelectionElementMask::Vertex) ||
        has_selection_mask(SelectionElementMask::Edge, SelectionElementMask::Face) ||
        ((SelectionElementMask::Vertex | SelectionElementMask::Edge) & SelectionElementMask::Face) !=
            SelectionElementMask::None) {
        return TestResult::fail("selection mask operators should combine and test element flags");
    }

    return TestResult::pass();
}

} // namespace locus::tests
