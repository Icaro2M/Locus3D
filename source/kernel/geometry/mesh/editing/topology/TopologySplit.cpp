/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/topology/TopologySplit.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/editing/topology/TopologyCreation.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRemoval.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <glm/common.hpp>
#include <vector>

namespace locus::kernel::geometry::editing::topology {

    namespace {

        std::optional<std::size_t> find_vertex_index(
            const std::vector<VertexHandle>& vertices,
            VertexHandle vertexHandle)
        {
            const auto iterator = std::find(vertices.begin(), vertices.end(), vertexHandle);

            if (iterator == vertices.end()) {
                return std::nullopt;
            }

            return static_cast<std::size_t>(std::distance(vertices.begin(), iterator));
        }

        bool are_adjacent_indices(std::size_t a, std::size_t b, std::size_t count)
        {
            if (count < 2) {
                return false;
            }

            return (a + 1) % count == b || (b + 1) % count == a;
        }

        std::vector<VertexHandle> path_between(
            const std::vector<VertexHandle>& vertices,
            std::size_t begin,
            std::size_t end)
        {
            std::vector<VertexHandle> result;

            if (vertices.empty()) {
                return result;
            }

            std::size_t current = begin;

            while (true) {
                result.push_back(vertices[current]);

                if (current == end) {
                    break;
                }

                current = (current + 1) % vertices.size();
            }

            return result;
        }

    }

    std::optional<VertexHandle> split_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle)
    {
        return split_edge_at_param(mesh, diff, edgeHandle, 0.5f);
    }

    std::optional<VertexHandle> split_edge_at_param(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle, float t)
    {
        if (!mesh.is_valid(edgeHandle)) {
            return std::nullopt;
        }

        const Edge edge = mesh.edge(edgeHandle);

        if (!mesh.is_valid(edge.vertexA) || !mesh.is_valid(edge.vertexB)) {
            return std::nullopt;
        }

        const float clampedT = glm::clamp(t, 0.0f, 1.0f);
        const glm::vec3 position =
            mesh.vertex(edge.vertexA).position * (1.0f - clampedT) +
            mesh.vertex(edge.vertexB).position * clampedT;

        VertexHandle newVertex = add_vertex(mesh, diff, position);

        if (!mesh.is_valid(newVertex)) {
            return std::nullopt;
        }

        const std::vector<FaceHandle> affectedFaces = TopologyTraversal::edge_faces(mesh, edgeHandle);

        if (affectedFaces.empty()) {
            kill_edge_only(mesh, diff, edgeHandle);
            find_or_create_edge(mesh, diff, edge.vertexA, newVertex);
            find_or_create_edge(mesh, diff, newVertex, edge.vertexB);
            return newVertex;
        }

        std::vector<std::vector<VertexHandle>> rebuiltFaces;

        for (FaceHandle faceHandle : affectedFaces) {
            std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, faceHandle);

            if (vertices.size() < 3) {
                continue;
            }

            std::vector<VertexHandle> rebuilt;

            for (std::size_t index = 0; index < vertices.size(); ++index) {
                const VertexHandle current = vertices[index];
                const VertexHandle next = vertices[(index + 1) % vertices.size()];

                rebuilt.push_back(current);

                const bool forward = current == edge.vertexA && next == edge.vertexB;
                const bool backward = current == edge.vertexB && next == edge.vertexA;

                if (forward || backward) {
                    rebuilt.push_back(newVertex);
                }
            }

            rebuiltFaces.push_back(rebuilt);
        }

        for (FaceHandle faceHandle : affectedFaces) {
            remove_face(mesh, diff, faceHandle);
        }

        kill_edge_only(mesh, diff, edgeHandle);

        find_or_create_edge(mesh, diff, edge.vertexA, newVertex);
        find_or_create_edge(mesh, diff, newVertex, edge.vertexB);

        for (const std::vector<VertexHandle>& vertices : rebuiltFaces) {
            if (vertices.size() >= 3) {
                add_face(mesh, diff, vertices);
            }
        }

        return newVertex;
    }

    std::optional<EdgeHandle> split_face(
        LEM& mesh,
        LEMDiff& diff,
        FaceHandle faceHandle,
        VertexHandle vertexA,
        VertexHandle vertexB)
    {
        if (!mesh.is_valid(faceHandle) || !mesh.is_valid(vertexA) || !mesh.is_valid(vertexB)) {
            return std::nullopt;
        }

        if (vertexA == vertexB) {
            return std::nullopt;
        }

        const std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, faceHandle);

        if (vertices.size() < 4) {
            return std::nullopt;
        }

        const std::optional<std::size_t> indexA = find_vertex_index(vertices, vertexA);
        const std::optional<std::size_t> indexB = find_vertex_index(vertices, vertexB);

        if (!indexA.has_value() || !indexB.has_value()) {
            return std::nullopt;
        }

        if (are_adjacent_indices(indexA.value(), indexB.value(), vertices.size())) {
            return std::nullopt;
        }

        std::vector<VertexHandle> first = path_between(vertices, indexA.value(), indexB.value());
        std::vector<VertexHandle> second = path_between(vertices, indexB.value(), indexA.value());

        if (first.size() < 3 || second.size() < 3) {
            return std::nullopt;
        }

        remove_face(mesh, diff, faceHandle);

        EdgeHandle diagonal = find_or_create_edge(mesh, diff, vertexA, vertexB);

        add_face(mesh, diff, first);
        add_face(mesh, diff, second);

        if (!mesh.is_valid(diagonal)) {
            return std::nullopt;
        }

        return diagonal;
    }

}