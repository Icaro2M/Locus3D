/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "QueryTestSuite.h"

#include "kernel/geometry/queries/AdjacencyQuery.h"

#include <algorithm>
#include <vector>

namespace {

template <typename Handle>
[[nodiscard]] bool contains(const std::vector<Handle>& handles, Handle handle)
{
    return std::find(handles.begin(), handles.end(), handle) != handles.end();
}

} // namespace

namespace locus::tests {

TestResult run_adjacency_query_tests()
{
    using namespace kernel::geometry;

    QueryMesh fixture = make_query_mesh();
    const LEM& mesh = fixture.mesh;

    const std::vector<EdgeHandle> vertexEdges = AdjacencyQuery::vertex_edges(mesh, fixture.v1);
    if (vertexEdges.size() != 3 ||
        !contains(vertexEdges, fixture.boundaryEdge) ||
        !contains(vertexEdges, fixture.sharedEdge) ||
        !contains(vertexEdges, mesh.find_edge(fixture.v1, fixture.v4))) {
        return TestResult::fail("vertex_edges should return every edge incident to a vertex");
    }

    const std::vector<VertexHandle> adjacentVertices = AdjacencyQuery::adjacent_vertices(mesh, fixture.v1);
    if (adjacentVertices.size() != 3 ||
        !contains(adjacentVertices, fixture.v0) ||
        !contains(adjacentVertices, fixture.v2) ||
        !contains(adjacentVertices, fixture.v4) ||
        !AdjacencyQuery::are_vertices_adjacent(mesh, fixture.v1, fixture.v4) ||
        AdjacencyQuery::are_vertices_adjacent(mesh, fixture.v1, fixture.v5)) {
        return TestResult::fail("adjacent_vertices should return edge-connected vertex neighbors");
    }

    const std::vector<VertexHandle> sharedEdgeVertices =
        AdjacencyQuery::edge_vertices(mesh, fixture.sharedEdge);
    if (sharedEdgeVertices.size() != 2 ||
        !contains(sharedEdgeVertices, fixture.v1) ||
        !contains(sharedEdgeVertices, fixture.v2) ||
        !AdjacencyQuery::edge_vertices(mesh, EdgeHandle{}).empty()) {
        return TestResult::fail("edge_vertices should return valid endpoints and ignore invalid edges");
    }

    const std::vector<LoopHandle> sharedLoops = AdjacencyQuery::edge_loops(mesh, fixture.sharedEdge);
    const std::vector<FaceHandle> sharedFaces = AdjacencyQuery::edge_faces(mesh, fixture.sharedEdge);
    if (sharedLoops.size() != 2 ||
        sharedFaces.size() != 2 ||
        !contains(sharedFaces, fixture.leftFace) ||
        !contains(sharedFaces, fixture.rightFace)) {
        return TestResult::fail("edge_loops and edge_faces should traverse radial cycles");
    }

    const std::vector<VertexHandle> leftVertices =
        AdjacencyQuery::face_vertices(mesh, fixture.leftFace);
    const std::vector<EdgeHandle> leftEdges =
        AdjacencyQuery::face_edges(mesh, fixture.leftFace);
    if (leftVertices.size() != 4 ||
        leftEdges.size() != 4 ||
        leftVertices[0] != fixture.v0 ||
        leftVertices[1] != fixture.v1 ||
        leftVertices[2] != fixture.v2 ||
        leftVertices[3] != fixture.v3 ||
        !contains(leftEdges, fixture.sharedEdge)) {
        return TestResult::fail("face vertex and edge queries should preserve the face boundary");
    }

    const std::vector<FaceHandle> edgeAdjacentFaces =
        AdjacencyQuery::adjacent_faces(mesh, fixture.leftFace);
    const std::vector<FaceHandle> vertexConnectedFaces =
        AdjacencyQuery::connected_faces_by_vertex(mesh, fixture.leftFace);
    if (edgeAdjacentFaces.size() != 1 ||
        edgeAdjacentFaces[0] != fixture.rightFace ||
        vertexConnectedFaces.size() != 1 ||
        vertexConnectedFaces[0] != fixture.rightFace ||
        !AdjacencyQuery::are_faces_adjacent(mesh, fixture.leftFace, fixture.rightFace) ||
        AdjacencyQuery::are_faces_adjacent(mesh, fixture.leftFace, fixture.leftFace)) {
        return TestResult::fail("face adjacency queries should report unique neighboring faces");
    }

    if (!AdjacencyQuery::is_boundary_edge(mesh, fixture.boundaryEdge) ||
        AdjacencyQuery::is_boundary_edge(mesh, fixture.sharedEdge) ||
        !AdjacencyQuery::is_manifold_edge(mesh, fixture.sharedEdge) ||
        !AdjacencyQuery::is_loose_vertex(mesh, fixture.looseVertex) ||
        AdjacencyQuery::is_loose_vertex(mesh, fixture.v0) ||
        !AdjacencyQuery::is_loose_edge(mesh, fixture.looseEdge) ||
        AdjacencyQuery::is_loose_edge(mesh, fixture.boundaryEdge)) {
        return TestResult::fail("adjacency classification should distinguish boundary, manifold, and loose elements");
    }

    return TestResult::pass();
}

} // namespace locus::tests
