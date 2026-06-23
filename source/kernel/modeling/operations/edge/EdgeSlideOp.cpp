/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/edge/EdgeSlideOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cmath>
#include <glm/geometric.hpp>
#include <utility>

namespace locus::kernel::modeling {

    EdgeSlideOp::EdgeSlideOp(geometry::EdgeHandle edge, float distance)
        : target_(EdgeSlideTarget::Edges)
        , edges_({ edge })
        , distance_(distance)
    {
    }

    EdgeSlideOp::EdgeSlideOp(
        std::vector<geometry::EdgeHandle> edges,
        float distance)
        : target_(EdgeSlideTarget::Edges)
        , edges_(std::move(edges))
        , distance_(distance)
    {
    }

    EdgeSlideOp::EdgeSlideOp(
        std::vector<geometry::VertexHandle> vertices,
        float distance)
        : target_(EdgeSlideTarget::SelectedVertices)
        , vertices_(std::move(vertices))
        , distance_(distance)
    {
    }

    EdgeSlideOp EdgeSlideOp::selected_edges(float distance)
    {
        EdgeSlideOp op;
        op.set_target(EdgeSlideTarget::SelectedEdges);
        op.set_distance(distance);
        return op;
    }

    EdgeSlideOp EdgeSlideOp::selected_vertices(float distance)
    {
        EdgeSlideOp op;
        op.set_target(EdgeSlideTarget::SelectedVertices);
        op.set_distance(distance);
        return op;
    }

    std::string_view EdgeSlideOp::name() const
    {
        return "EdgeSlideOp";
    }

    void EdgeSlideOp::set_target(EdgeSlideTarget target)
    {
        target_ = target;
    }

    EdgeSlideTarget EdgeSlideOp::target() const
    {
        return target_;
    }

    void EdgeSlideOp::set_distance(float distance)
    {
        distance_ = distance;
    }

    float EdgeSlideOp::distance() const
    {
        return distance_;
    }

    void EdgeSlideOp::set_edges(std::vector<geometry::EdgeHandle> edges)
    {
        edges_ = std::move(edges);
    }

    const std::vector<geometry::EdgeHandle>& EdgeSlideOp::edges() const
    {
        return edges_;
    }

    void EdgeSlideOp::clear_edges()
    {
        edges_.clear();
    }

    void EdgeSlideOp::set_vertices(std::vector<geometry::VertexHandle> vertices)
    {
        vertices_ = std::move(vertices);
    }

    const std::vector<geometry::VertexHandle>& EdgeSlideOp::vertices() const
    {
        return vertices_;
    }

    void EdgeSlideOp::clear_vertices()
    {
        vertices_.clear();
    }

    void EdgeSlideOp::set_exclude_target_edges_from_rails(bool excludeTargetEdges)
    {
        excludeTargetEdgesFromRails_ = excludeTargetEdges;
    }

    bool EdgeSlideOp::exclude_target_edges_from_rails() const
    {
        return excludeTargetEdgesFromRails_;
    }

    OperationResult EdgeSlideOp::execute_impl(OperationContext& context)
    {
        if (std::abs(distance_) <= 0.000001f) {
            return OperationResult::no_change(
                "Edge slide operation has zero distance.");
        }

        geometry::LEM& mesh = context.editable_mesh();

        const std::vector<geometry::EdgeHandle> targetEdges = collect_edges(mesh);
        const std::vector<geometry::VertexHandle> targetVertices =
            collect_vertices(mesh, targetEdges);

        if (targetVertices.empty()) {
            return OperationResult::no_change(
                "Edge slide operation found no valid target vertices.");
        }

        geometry::LEMEditor editor(mesh);
        std::size_t movedCount = 0;

        for (geometry::VertexHandle vertex : targetVertices) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            const glm::vec3 direction = slide_direction(mesh, vertex, targetEdges);
            if (glm::length(direction) <= 0.000001f) {
                continue;
            }

            const glm::vec3 oldPosition = mesh.vertex(vertex).position;
            const glm::vec3 newPosition = oldPosition + direction * distance_;

            if (editor.set_vertex_position(vertex, newPosition)) {
                ++movedCount;
            }
        }

        if (movedCount == 0) {
            return OperationResult::no_change(
                "Edge slide operation did not move any vertex.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::EdgeHandle> EdgeSlideOp::collect_edges(
        const geometry::LEM& mesh) const
    {
        std::vector<geometry::EdgeHandle> result;

        if (!edges_.empty()) {
            result.reserve(edges_.size());

            for (geometry::EdgeHandle edge : edges_) {
                if (!mesh.is_valid(edge) || contains(result, edge)) {
                    continue;
                }

                result.push_back(edge);
            }

            return result;
        }

        if (target_ == EdgeSlideTarget::SelectedVertices) {
            return result;
        }

        const std::vector<geometry::EdgeHandle> activeEdges =
            geometry::TopologyTraversal::edges(mesh);

        result.reserve(activeEdges.size());

        for (geometry::EdgeHandle edge : activeEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (target_ == EdgeSlideTarget::SelectedEdges &&
                !mesh.edge(edge).selected) {
                continue;
            }

            result.push_back(edge);
        }

        return result;
    }

    std::vector<geometry::VertexHandle> EdgeSlideOp::collect_vertices(
        const geometry::LEM& mesh,
        const std::vector<geometry::EdgeHandle>& targetEdges) const
    {
        std::vector<geometry::VertexHandle> result;

        if (!vertices_.empty()) {
            result.reserve(vertices_.size());

            for (geometry::VertexHandle vertex : vertices_) {
                if (!mesh.is_valid(vertex) || contains(result, vertex)) {
                    continue;
                }

                result.push_back(vertex);
            }

            return result;
        }

        if (target_ == EdgeSlideTarget::SelectedVertices) {
            const std::vector<geometry::VertexHandle> activeVertices =
                geometry::TopologyTraversal::vertices(mesh);

            result.reserve(activeVertices.size());

            for (geometry::VertexHandle vertex : activeVertices) {
                if (!mesh.is_valid(vertex) || !mesh.vertex(vertex).selected) {
                    continue;
                }

                result.push_back(vertex);
            }

            return result;
        }

        for (geometry::EdgeHandle edge : targetEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            const geometry::Edge& edgeElement = mesh.edge(edge);

            if (mesh.is_valid(edgeElement.vertexA) &&
                !contains(result, edgeElement.vertexA)) {
                result.push_back(edgeElement.vertexA);
            }

            if (mesh.is_valid(edgeElement.vertexB) &&
                !contains(result, edgeElement.vertexB)) {
                result.push_back(edgeElement.vertexB);
            }
        }

        return result;
    }

    glm::vec3 EdgeSlideOp::slide_direction(
        const geometry::LEM& mesh,
        geometry::VertexHandle vertex,
        const std::vector<geometry::EdgeHandle>& targetEdges) const
    {
        if (!mesh.is_valid(vertex)) {
            return glm::vec3{ 0.0f };
        }

        const glm::vec3 origin = mesh.vertex(vertex).position;
        const std::vector<geometry::EdgeHandle> incidentEdges =
            geometry::TopologyTraversal::vertex_edges(mesh, vertex);

        glm::vec3 accumulated{ 0.0f };
        std::vector<glm::vec3> railDirections;

        for (geometry::EdgeHandle edge : incidentEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (excludeTargetEdgesFromRails_ && contains(targetEdges, edge)) {
                continue;
            }

            const geometry::Edge& edgeElement = mesh.edge(edge);

            geometry::VertexHandle otherVertex{};
            if (edgeElement.vertexA == vertex) {
                otherVertex = edgeElement.vertexB;
            }
            else if (edgeElement.vertexB == vertex) {
                otherVertex = edgeElement.vertexA;
            }
            else {
                continue;
            }

            if (!mesh.is_valid(otherVertex)) {
                continue;
            }

            const glm::vec3 delta = mesh.vertex(otherVertex).position - origin;
            const float length = glm::length(delta);

            if (length <= 0.000001f) {
                continue;
            }

            const glm::vec3 direction = delta / length;
            railDirections.push_back(direction);
            accumulated += direction;
        }

        if (railDirections.empty()) {
            return glm::vec3{ 0.0f };
        }

        const float accumulatedLength = glm::length(accumulated);
        if (accumulatedLength > 0.000001f) {
            return accumulated / accumulatedLength;
        }

        return railDirections.front();
    }

}