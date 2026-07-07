/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TopologyTestSuite.h"

#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <array>
#include <vector>

namespace {

template <typename Handle>
[[nodiscard]] bool contains(const std::vector<Handle>& handles, Handle handle)
{
    return std::find(handles.begin(), handles.end(), handle) != handles.end();
}

} // namespace

namespace locus::tests {

TestResult run_topology_traversal_tests()
{
    using namespace kernel::geometry;

    TwoQuadTopology fixture = make_two_quad_topology();
    LEM& mesh = fixture.mesh;

    if (TopologyTraversal::vertices(mesh).size() != 9 ||
        TopologyTraversal::edges(mesh).size() != 8 ||
        TopologyTraversal::loops(mesh).size() != 8 ||
        TopologyTraversal::faces(mesh).size() != 2) {
        return TestResult::fail("TopologyTraversal should enumerate active vertices, edges, loops, and faces");
    }

    TwoQuadTopology deletedFixture = make_two_quad_topology();
    deletedFixture.mesh.vertex(deletedFixture.looseVertex).deleted = true;
    deletedFixture.mesh.edge(deletedFixture.looseEdge).deleted = true;
    if (TopologyTraversal::vertices(deletedFixture.mesh).size() != 8 ||
        TopologyTraversal::edges(deletedFixture.mesh).size() != 7) {
        return TestResult::fail("TopologyTraversal should skip deleted vertex and edge slots");
    }

    const std::vector<LoopHandle> leftLoops =
        TopologyTraversal::face_loops(mesh, fixture.leftFace);
    const std::vector<VertexHandle> leftVertices =
        TopologyTraversal::face_vertices(mesh, fixture.leftFace);
    const std::vector<EdgeHandle> leftEdges =
        TopologyTraversal::face_edges(mesh, fixture.leftFace);
    if (leftLoops.size() != 4 ||
        leftVertices.size() != 4 ||
        leftEdges.size() != 4 ||
        leftVertices[0] != fixture.v0 ||
        leftVertices[1] != fixture.v1 ||
        leftVertices[2] != fixture.v2 ||
        leftVertices[3] != fixture.v3 ||
        !contains(leftEdges, fixture.sharedEdge) ||
        !TopologyTraversal::face_loops(mesh, FaceHandle{}).empty()) {
        return TestResult::fail("face traversal should return ordered loops, vertices, and unique edges");
    }

    const std::array<VertexHandle, 2> endpoints =
        TopologyTraversal::edge_vertices(mesh, fixture.sharedEdge);
    const std::array<VertexHandle, 2> invalidEndpoints =
        TopologyTraversal::edge_vertices(mesh, EdgeHandle{});
    if (((endpoints[0] != fixture.v1 || endpoints[1] != fixture.v2) &&
            (endpoints[0] != fixture.v2 || endpoints[1] != fixture.v1)) ||
        invalidEndpoints[0].is_valid() ||
        invalidEndpoints[1].is_valid()) {
        return TestResult::fail("edge_vertices should return endpoints or invalid handles for missing edges");
    }

    const std::vector<LoopHandle> sharedLoops =
        TopologyTraversal::edge_loops(mesh, fixture.sharedEdge);
    const std::vector<FaceHandle> sharedFaces =
        TopologyTraversal::edge_faces(mesh, fixture.sharedEdge);
    if (sharedLoops.size() != 2 ||
        sharedFaces.size() != 2 ||
        !contains(sharedFaces, fixture.leftFace) ||
        !contains(sharedFaces, fixture.rightFace) ||
        !TopologyTraversal::edge_loops(mesh, EdgeHandle{}).empty()) {
        return TestResult::fail("edge traversal should follow radial loops and collect unique faces");
    }

    const std::vector<EdgeHandle> vertexEdges =
        TopologyTraversal::vertex_edges(mesh, fixture.v1);
    const std::vector<LoopHandle> vertexLoops =
        TopologyTraversal::vertex_loops(mesh, fixture.v1);
    const std::vector<FaceHandle> vertexFaces =
        TopologyTraversal::vertex_faces(mesh, fixture.v1);
    const std::vector<VertexHandle> adjacentVertices =
        TopologyTraversal::adjacent_vertices(mesh, fixture.v1);
    if (vertexEdges.size() != 3 ||
        vertexLoops.size() != 2 ||
        vertexFaces.size() != 2 ||
        adjacentVertices.size() != 3 ||
        !contains(vertexEdges, fixture.boundaryEdge) ||
        !contains(vertexEdges, fixture.sharedEdge) ||
        !contains(vertexFaces, fixture.leftFace) ||
        !contains(vertexFaces, fixture.rightFace) ||
        !contains(adjacentVertices, fixture.v0) ||
        !contains(adjacentVertices, fixture.v2) ||
        !contains(adjacentVertices, fixture.v4) ||
        !TopologyTraversal::vertex_edges(mesh, VertexHandle{}).empty()) {
        return TestResult::fail("vertex traversal should collect incident edges, loops, faces, and neighbors");
    }

    if (!TopologyTraversal::is_boundary_edge(mesh, fixture.boundaryEdge) ||
        TopologyTraversal::is_boundary_edge(mesh, fixture.sharedEdge) ||
        !TopologyTraversal::is_manifold_edge(mesh, fixture.sharedEdge) ||
        TopologyTraversal::is_boundary_edge(mesh, EdgeHandle{}) ||
        TopologyTraversal::is_manifold_edge(mesh, EdgeHandle{})) {
        return TestResult::fail("edge classification should distinguish boundary, manifold, and invalid edges");
    }

    mesh.edge(fixture.sharedEdge).loop = LoopHandle{};
    if (!TopologyTraversal::edge_loops(mesh, fixture.sharedEdge).empty() ||
        !TopologyTraversal::edge_faces(mesh, fixture.sharedEdge).empty()) {
        return TestResult::fail("edge traversal should return empty results when an edge has no radial entry loop");
    }

    return TestResult::pass();
}

} // namespace locus::tests
