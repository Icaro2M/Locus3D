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
#include <cmath>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

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

        template <typename HandleT>
        void append_unique(std::vector<HandleT>& handles, HandleT handle)
        {
            if (!contains_handle(handles, handle)) {
                handles.push_back(handle);
            }
        }

        bool has_duplicate_vertices(const std::vector<VertexHandle>& vertices)
        {
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                for (std::size_t j = i + 1; j < vertices.size(); ++j) {
                    if (vertices[i] == vertices[j]) {
                        return true;
                    }
                }
            }

            return false;
        }

        std::vector<VertexHandle> replaced_face_vertices(
            const std::vector<VertexHandle>& vertices,
            VertexHandle sourceVertex,
            VertexHandle targetVertex)
        {
            std::vector<VertexHandle> result;
            result.reserve(vertices.size());

            for (VertexHandle vertexHandle : vertices) {
                result.push_back(vertexHandle == sourceVertex ? targetVertex : vertexHandle);
            }

            return result;
        }

        bool is_valid_merge_face(const std::vector<VertexHandle>& vertices)
        {
            if (vertices.size() < 3) {
                return false;
            }

            return !has_duplicate_vertices(vertices);
        }

        std::vector<VertexHandle> active_vertices_from_set(
            const LEM& mesh,
            const std::vector<VertexHandle>& vertices)
        {
            std::vector<VertexHandle> result;
            result.reserve(vertices.size());

            for (VertexHandle vertexHandle : vertices) {
                if (mesh.is_valid(vertexHandle) && !contains_handle(result, vertexHandle)) {
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

    bool merge_vertices(
        LEM& mesh,
        LEMDiff& diff,
        VertexHandle sourceVertex,
        VertexHandle targetVertex)
    {
        if (!mesh.is_valid(sourceVertex) || !mesh.is_valid(targetVertex)) {
            return false;
        }

        if (sourceVertex == targetVertex) {
            return false;
        }

        return merge_vertices_at_position(
            mesh,
            diff,
            sourceVertex,
            targetVertex,
            mesh.vertex(targetVertex).position);
    }

    bool merge_vertices_at_position(
        LEM& mesh,
        LEMDiff& diff,
        VertexHandle sourceVertex,
        VertexHandle targetVertex,
        const glm::vec3& position)
    {
        if (!mesh.is_valid(sourceVertex) || !mesh.is_valid(targetVertex)) {
            return false;
        }

        if (sourceVertex == targetVertex) {
            return false;
        }

        mesh.vertex(targetVertex).position = position;
        diff.record(LEMChangeType::VertexModified, targetVertex);

        const std::vector<FaceHandle> affectedFaces =
            TopologyTraversal::vertex_faces(mesh, sourceVertex);

        const std::vector<EdgeHandle> sourceEdges =
            TopologyTraversal::vertex_edges(mesh, sourceVertex);

        std::vector<std::vector<VertexHandle>> rebuiltFaces;
        rebuiltFaces.reserve(affectedFaces.size());

        for (FaceHandle faceHandle : affectedFaces) {
            if (!mesh.is_valid(faceHandle)) {
                continue;
            }

            std::vector<VertexHandle> vertices =
                replaced_face_vertices(
                    TopologyTraversal::face_vertices(mesh, faceHandle),
                    sourceVertex,
                    targetVertex);

            if (is_valid_merge_face(vertices)) {
                rebuiltFaces.push_back(vertices);
            }
        }

        for (FaceHandle faceHandle : affectedFaces) {
            if (mesh.is_valid(faceHandle)) {
                remove_face(mesh, diff, faceHandle);
            }
        }

        for (EdgeHandle edgeHandle : sourceEdges) {
            if (!mesh.is_valid(edgeHandle)) {
                continue;
            }

            Edge& edge = mesh.edge(edgeHandle);

            VertexHandle otherVertex{};
            if (edge.vertexA == sourceVertex) {
                otherVertex = edge.vertexB;
            }
            else if (edge.vertexB == sourceVertex) {
                otherVertex = edge.vertexA;
            }
            else {
                continue;
            }

            if (!mesh.is_valid(otherVertex) || otherVertex == targetVertex) {
                kill_edge_only(mesh, diff, edgeHandle);
                continue;
            }

            EdgeHandle existingEdge = mesh.find_edge(targetVertex, otherVertex);
            if (mesh.is_valid(existingEdge) && existingEdge != edgeHandle) {
                kill_edge_only(mesh, diff, edgeHandle);
                continue;
            }

            if (edge.vertexA == sourceVertex) {
                edge.vertexA = targetVertex;
            }

            if (edge.vertexB == sourceVertex) {
                edge.vertexB = targetVertex;
            }

            diff.record(LEMChangeType::EdgeModified, edgeHandle);
        }

        for (const std::vector<VertexHandle>& vertices : rebuiltFaces) {
            add_face(mesh, diff, vertices);
        }

        refresh_vertex_incident_edge(mesh, diff, targetVertex);

        if (mesh.is_valid(sourceVertex)) {
            Vertex& source = mesh.vertex(sourceVertex);
            source.edge = {};
            source.deleted = true;
            diff.record(LEMChangeType::VertexModified, sourceVertex);
        }

        return true;
    }

    std::size_t merge_vertices_by_distance(
        LEM& mesh,
        LEMDiff& diff,
        float distance)
    {
        if (distance < 0.0f) {
            return 0;
        }

        const float distanceSquared = distance * distance;
        std::vector<VertexHandle> vertices = TopologyTraversal::vertices(mesh);

        std::size_t mergeCount = 0;

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            VertexHandle targetVertex = vertices[i];

            if (!mesh.is_valid(targetVertex)) {
                continue;
            }

            for (std::size_t j = i + 1; j < vertices.size(); ++j) {
                VertexHandle sourceVertex = vertices[j];

                if (!mesh.is_valid(sourceVertex)) {
                    continue;
                }

                const glm::vec3 delta =
                    mesh.vertex(sourceVertex).position - mesh.vertex(targetVertex).position;

                if (glm::dot(delta, delta) > distanceSquared) {
                    continue;
                }

                const glm::vec3 mergedPosition =
                    (mesh.vertex(sourceVertex).position + mesh.vertex(targetVertex).position) * 0.5f;

                if (merge_vertices_at_position(
                    mesh,
                    diff,
                    sourceVertex,
                    targetVertex,
                    mergedPosition)) {
                    ++mergeCount;
                }
            }
        }

        return mergeCount;
    }

    std::size_t weld_vertices(
        LEM& mesh,
        LEMDiff& diff,
        const std::vector<VertexHandle>& vertices,
        float distance)
    {
        if (distance < 0.0f) {
            return 0;
        }

        const float distanceSquared = distance * distance;
        std::vector<VertexHandle> candidates = active_vertices_from_set(mesh, vertices);

        std::size_t mergeCount = 0;

        for (std::size_t i = 0; i < candidates.size(); ++i) {
            VertexHandle targetVertex = candidates[i];

            if (!mesh.is_valid(targetVertex)) {
                continue;
            }

            for (std::size_t j = i + 1; j < candidates.size(); ++j) {
                VertexHandle sourceVertex = candidates[j];

                if (!mesh.is_valid(sourceVertex)) {
                    continue;
                }

                const glm::vec3 delta =
                    mesh.vertex(sourceVertex).position - mesh.vertex(targetVertex).position;

                if (glm::dot(delta, delta) > distanceSquared) {
                    continue;
                }

                const glm::vec3 mergedPosition =
                    (mesh.vertex(sourceVertex).position + mesh.vertex(targetVertex).position) * 0.5f;

                if (merge_vertices_at_position(
                    mesh,
                    diff,
                    sourceVertex,
                    targetVertex,
                    mergedPosition)) {
                    ++mergeCount;
                }
            }
        }

        return mergeCount;
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