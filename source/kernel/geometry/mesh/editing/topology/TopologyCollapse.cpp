/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/topology/TopologyCollapse.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/editing/topology/TopologyCreation.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRemoval.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRelink.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Vertex.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <vector>

namespace locus::kernel::geometry::editing::topology {

    namespace {

        template <typename HandleT>
        bool contains_handle(const std::vector<HandleT>& handles, HandleT handle)
        {
            return std::find(handles.begin(), handles.end(), handle) != handles.end();
        }

        std::vector<VertexHandle> compact_vertices(const std::vector<VertexHandle>& vertices)
        {
            std::vector<VertexHandle> result;

            for (VertexHandle vertexHandle : vertices) {
                if (!contains_handle(result, vertexHandle)) {
                    result.push_back(vertexHandle);
                }
            }

            return result;
        }

    }

    bool collapse_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(edgeHandle)) {
            return false;
        }

        const Edge edge = mesh.edge(edgeHandle);

        if (!mesh.is_valid(edge.vertexA) || !mesh.is_valid(edge.vertexB)) {
            return false;
        }

        Vertex& vertexA = mesh.vertex(edge.vertexA);
        Vertex& vertexB = mesh.vertex(edge.vertexB);

        vertexA.position = (vertexA.position + vertexB.position) * 0.5f;
        diff.record(LEMChangeType::VertexModified, edge.vertexA);

        std::vector<FaceHandle> affectedFaces = TopologyTraversal::vertex_faces(mesh, edge.vertexB);

        for (FaceHandle faceHandle : TopologyTraversal::vertex_faces(mesh, edge.vertexA)) {
            if (!contains_handle(affectedFaces, faceHandle)) {
                affectedFaces.push_back(faceHandle);
            }
        }

        std::vector<std::vector<VertexHandle>> rebuiltFaces;

        for (FaceHandle faceHandle : affectedFaces) {
            std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, faceHandle);

            for (VertexHandle& vertexHandle : vertices) {
                if (vertexHandle == edge.vertexB) {
                    vertexHandle = edge.vertexA;
                }
            }

            vertices = compact_vertices(vertices);

            if (vertices.size() >= 3) {
                rebuiltFaces.push_back(vertices);
            }
        }

        for (FaceHandle faceHandle : affectedFaces) {
            remove_face(mesh, diff, faceHandle);
        }

        for (const std::vector<VertexHandle>& vertices : rebuiltFaces) {
            add_face(mesh, diff, vertices);
        }

        for (EdgeHandle incidentEdge : TopologyTraversal::vertex_edges(mesh, edge.vertexB)) {
            if (incidentEdge == edgeHandle) {
                continue;
            }

            if (!mesh.is_valid(incidentEdge)) {
                continue;
            }

            Edge& mutableEdge = mesh.edge(incidentEdge);

            if (mutableEdge.vertexA == edge.vertexB) {
                mutableEdge.vertexA = edge.vertexA;
            }

            if (mutableEdge.vertexB == edge.vertexB) {
                mutableEdge.vertexB = edge.vertexA;
            }

            diff.record(LEMChangeType::EdgeModified, incidentEdge);
        }

        kill_edge_only(mesh, diff, edgeHandle);

        vertexB.edge = {};
        vertexB.deleted = true;
        diff.record(LEMChangeType::VertexModified, edge.vertexB);

        refresh_vertex_incident_edge(mesh, diff, edge.vertexA);

        return true;
    }

    bool dissolve_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(edgeHandle)) {
            return false;
        }

        const std::vector<FaceHandle> faces = TopologyTraversal::edge_faces(mesh, edgeHandle);

        if (faces.empty()) {
            return remove_edge_if_loose(mesh, diff, edgeHandle);
        }

        if (faces.size() != 2) {
            return false;
        }

        std::vector<VertexHandle> merged;

        for (FaceHandle faceHandle : faces) {
            for (VertexHandle vertexHandle : TopologyTraversal::face_vertices(mesh, faceHandle)) {
                if (!contains_handle(merged, vertexHandle)) {
                    merged.push_back(vertexHandle);
                }
            }
        }

        if (merged.size() < 3) {
            return false;
        }

        for (FaceHandle faceHandle : faces) {
            remove_face(mesh, diff, faceHandle);
        }

        kill_edge_only(mesh, diff, edgeHandle);

        add_face(mesh, diff, merged);

        return true;
    }

    bool dissolve_vertex(LEM& mesh, LEMDiff& diff, VertexHandle vertexHandle)
    {
        if (!mesh.is_valid(vertexHandle)) {
            return false;
        }

        const std::vector<LoopHandle> loops = TopologyTraversal::vertex_loops(mesh, vertexHandle);

        if (!loops.empty()) {
            return false;
        }

        std::vector<EdgeHandle> incidentEdges = TopologyTraversal::vertex_edges(mesh, vertexHandle);

        if (incidentEdges.size() > 2) {
            return false;
        }

        std::vector<VertexHandle> neighbors;

        for (EdgeHandle edgeHandle : incidentEdges) {
            if (!mesh.is_valid(edgeHandle)) {
                continue;
            }

            const Edge& edge = mesh.edge(edgeHandle);
            VertexHandle neighbor = edge.vertexA == vertexHandle ? edge.vertexB : edge.vertexA;

            if (mesh.is_valid(neighbor) && !contains_handle(neighbors, neighbor)) {
                neighbors.push_back(neighbor);
            }
        }

        for (EdgeHandle edgeHandle : incidentEdges) {
            kill_edge_only(mesh, diff, edgeHandle);
        }

        if (neighbors.size() == 2 && neighbors[0] != neighbors[1]) {
            find_or_create_edge(mesh, diff, neighbors[0], neighbors[1]);
        }

        return remove_vertex_if_loose(mesh, diff, vertexHandle);
    }

    bool dissolve_face(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle)
    {
        if (!mesh.is_valid(faceHandle)) {
            return false;
        }

        const std::vector<EdgeHandle> edges = TopologyTraversal::face_edges(mesh, faceHandle);
        const std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, faceHandle);

        if (!remove_face(mesh, diff, faceHandle)) {
            return false;
        }

        for (EdgeHandle edgeHandle : edges) {
            remove_edge_if_loose(mesh, diff, edgeHandle);
        }

        for (VertexHandle vertexHandle : vertices) {
            remove_vertex_if_loose(mesh, diff, vertexHandle);
        }

        return true;
    }

}