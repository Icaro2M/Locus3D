/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/topology/FillHoleOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace locus::kernel::modeling {
    namespace {

        [[nodiscard]] std::array<geometry::VertexHandle, 2> fill_direction_for_boundary_edge(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edge)
        {
            const geometry::Edge& edgeElement = mesh.edge(edge);
            std::array<geometry::VertexHandle, 2> direction{
                edgeElement.vertexA,
                edgeElement.vertexB
            };

            const std::vector<geometry::LoopHandle> loops =
                geometry::TopologyTraversal::edge_loops(mesh, edge);
            if (loops.size() != 1u) {
                return direction;
            }

            const geometry::Loop& loop = mesh.loop(loops.front());
            if (!mesh.is_valid(loop.next)) {
                return direction;
            }

            const geometry::VertexHandle adjacentStart = loop.vertex;
            const geometry::VertexHandle adjacentEnd = mesh.loop(loop.next).vertex;
            if (mesh.is_valid(adjacentStart) && mesh.is_valid(adjacentEnd)) {
                direction = { adjacentEnd, adjacentStart };
            }

            return direction;
        }

    }

    FillHoleOp::FillHoleOp(std::vector<geometry::VertexHandle> vertices)
        : mode_(FillHoleMode::VertexCycle)
        , vertices_(std::move(vertices)) {
    }

    FillHoleOp FillHoleOp::edges(std::vector<geometry::EdgeHandle> edges) {
        FillHoleOp op;
        op.set_mode(FillHoleMode::EdgeCycle);
        op.set_edges(std::move(edges));
        return op;
    }

    FillHoleOp FillHoleOp::selected_boundary_edges() {
        FillHoleOp op;
        op.set_mode(FillHoleMode::SelectedBoundaryEdges);
        return op;
    }

    std::string_view FillHoleOp::name() const {
        return "FillHoleOp";
    }

    void FillHoleOp::set_mode(FillHoleMode mode) {
        mode_ = mode;
    }

    FillHoleMode FillHoleOp::mode() const {
        return mode_;
    }

    void FillHoleOp::set_vertices(std::vector<geometry::VertexHandle> vertices) {
        vertices_ = std::move(vertices);
        mode_ = FillHoleMode::VertexCycle;
    }

    const std::vector<geometry::VertexHandle>& FillHoleOp::vertices() const {
        return vertices_;
    }

    void FillHoleOp::clear_vertices() {
        vertices_.clear();
    }

    void FillHoleOp::set_edges(std::vector<geometry::EdgeHandle> edges) {
        edges_ = std::move(edges);
        mode_ = FillHoleMode::EdgeCycle;
    }

    const std::vector<geometry::EdgeHandle>& FillHoleOp::edges() const {
        return edges_;
    }

    void FillHoleOp::clear_edges() {
        edges_.clear();
    }

    void FillHoleOp::set_flip_winding(bool flipWinding) {
        flipWinding_ = flipWinding;
    }

    bool FillHoleOp::flip_winding() const {
        return flipWinding_;
    }

    OperationResult FillHoleOp::execute_impl(OperationContext& context) {
        geometry::LEM& mesh = context.editable_mesh();

        std::vector<geometry::VertexHandle> cycle = collect_vertex_cycle(mesh);
        if (cycle.empty()) {
            return OperationResult::no_change("FillHole operation found no valid closed cycle.");
        }

        if (flipWinding_) {
            std::reverse(cycle.begin(), cycle.end());
        }

        if (!validate_vertex_cycle(mesh, cycle)) {
            return OperationResult::fail(
                kernel::ErrorCode::InvalidArgument,
                "FillHole operation received an invalid vertex cycle.");
        }

        if (!validate_topology_policy(context, mesh, cycle)) {
            return OperationResult::fail(
                kernel::ErrorCode::NonManifoldTopology,
                "FillHole operation would create non-manifold topology.");
        }

        if (!has_non_degenerate_area(mesh, cycle)) {
            return OperationResult::fail(
                kernel::ErrorCode::DegenerateGeometry,
                "FillHole operation cannot create a face from a degenerate cycle.");
        }

        geometry::LEMEditor editor(mesh);
        const geometry::FaceHandle face = editor.add_face(cycle);

        if (!mesh.is_valid(face)) {
            return OperationResult::fail(
                kernel::ErrorCode::InvalidState,
                "FillHole operation failed to create the face.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_normals_around_face(face);
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::VertexHandle> FillHoleOp::collect_vertex_cycle(
        const geometry::LEM& mesh) const {
        switch (mode_) {
        case FillHoleMode::VertexCycle:
            return vertices_;

        case FillHoleMode::EdgeCycle:
            return order_edge_cycle(mesh, edges_);

        case FillHoleMode::SelectedBoundaryEdges:
            return order_edge_cycle(mesh, collect_selected_boundary_edges(mesh));
        }

        return {};
    }

    std::vector<geometry::EdgeHandle> FillHoleOp::collect_selected_boundary_edges(
        const geometry::LEM& mesh) const {
        std::vector<geometry::EdgeHandle> result;

        const std::vector<geometry::EdgeHandle> activeEdges =
            geometry::TopologyTraversal::edges(mesh);

        result.reserve(activeEdges.size());

        for (geometry::EdgeHandle edge : activeEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (!mesh.edge(edge).selected) {
                continue;
            }

            if (!geometry::TopologyTraversal::is_boundary_edge(mesh, edge)) {
                continue;
            }

            result.push_back(edge);
        }

        return result;
    }

    std::vector<geometry::VertexHandle> FillHoleOp::order_edge_cycle(
        const geometry::LEM& mesh,
        const std::vector<geometry::EdgeHandle>& edges) const {
        if (edges.size() < 3) {
            return {};
        }

        std::vector<geometry::EdgeHandle> uniqueEdges;
        uniqueEdges.reserve(edges.size());

        for (geometry::EdgeHandle edge : edges) {
            if (!mesh.is_valid(edge)) {
                return {};
            }

            if (!geometry::TopologyTraversal::is_boundary_edge(mesh, edge)) {
                return {};
            }

            if (!contains_edge(uniqueEdges, edge)) {
                uniqueEdges.push_back(edge);
            }
        }

        if (uniqueEdges.size() < 3) {
            return {};
        }

        const std::array<geometry::VertexHandle, 2> firstDirection =
            fill_direction_for_boundary_edge(mesh, uniqueEdges.front());
        if (!mesh.is_valid(firstDirection[0]) || !mesh.is_valid(firstDirection[1])) {
            return {};
        }

        std::vector<geometry::VertexHandle> orderedVertices;
        orderedVertices.reserve(uniqueEdges.size());

        std::vector<geometry::EdgeHandle> usedEdges;
        usedEdges.reserve(uniqueEdges.size());

        geometry::VertexHandle startVertex = firstDirection[0];
        geometry::VertexHandle currentVertex = firstDirection[0];
        geometry::VertexHandle nextVertex = firstDirection[1];

        orderedVertices.push_back(currentVertex);
        usedEdges.push_back(uniqueEdges.front());

        currentVertex = nextVertex;

        while (currentVertex != startVertex) {
            orderedVertices.push_back(currentVertex);

            geometry::EdgeHandle nextEdge{};
            geometry::VertexHandle candidateNextVertex{};

            for (geometry::EdgeHandle edgeHandle : uniqueEdges) {
                if (contains_edge(usedEdges, edgeHandle)) {
                    continue;
                }

                const geometry::Edge& edge = mesh.edge(edgeHandle);

                if (edge.vertexA == currentVertex) {
                    nextEdge = edgeHandle;
                    candidateNextVertex = edge.vertexB;
                    break;
                }

                if (edge.vertexB == currentVertex) {
                    nextEdge = edgeHandle;
                    candidateNextVertex = edge.vertexA;
                    break;
                }
            }

            if (!mesh.is_valid(nextEdge)) {
                return {};
            }

            usedEdges.push_back(nextEdge);
            currentVertex = candidateNextVertex;

            if (usedEdges.size() > uniqueEdges.size()) {
                return {};
            }
        }

        if (usedEdges.size() != uniqueEdges.size()) {
            return {};
        }

        if (orderedVertices.size() != uniqueEdges.size()) {
            return {};
        }

        return orderedVertices;
    }

    bool FillHoleOp::validate_vertex_cycle(
        const geometry::LEM& mesh,
        const std::vector<geometry::VertexHandle>& vertices) const {
        if (vertices.size() < 3) {
            return false;
        }

        for (geometry::VertexHandle vertex : vertices) {
            if (!mesh.is_valid(vertex)) {
                return false;
            }
        }

        std::vector<geometry::VertexHandle> uniqueVertices;
        uniqueVertices.reserve(vertices.size());

        for (geometry::VertexHandle vertex : vertices) {
            if (contains_vertex(uniqueVertices, vertex)) {
                return false;
            }

            uniqueVertices.push_back(vertex);
        }

        return true;
    }

    bool FillHoleOp::validate_topology_policy(
        const OperationContext& context,
        const geometry::LEM& mesh,
        const std::vector<geometry::VertexHandle>& vertices) const {
        if (context.allowNonManifold) {
            return true;
        }

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const geometry::VertexHandle vertexA = vertices[i];
            const geometry::VertexHandle vertexB = vertices[(i + 1) % vertices.size()];

            const geometry::EdgeHandle edge = mesh.find_edge(vertexA, vertexB);
            if (!mesh.is_valid(edge)) {
                continue;
            }

            const std::vector<geometry::LoopHandle> loops =
                geometry::TopologyTraversal::edge_loops(mesh, edge);

            if (loops.size() >= 2) {
                return false;
            }
        }

        return true;
    }

    bool FillHoleOp::has_non_degenerate_area(
        const geometry::LEM& mesh,
        const std::vector<geometry::VertexHandle>& vertices) const {
        glm::vec3 normalSum{ 0.0f, 0.0f, 0.0f };

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const glm::vec3& a = mesh.vertex(vertices[i]).position;
            const glm::vec3& b = mesh.vertex(vertices[(i + 1) % vertices.size()]).position;

            normalSum += glm::cross(a, b);
        }

        const float areaMagnitudeSquared = glm::dot(normalSum, normalSum);
        return std::isfinite(areaMagnitudeSquared) && areaMagnitudeSquared > 1.0e-10f;
    }

    bool FillHoleOp::contains_vertex(
        const std::vector<geometry::VertexHandle>& handles,
        geometry::VertexHandle handle) {
        return std::find(handles.begin(), handles.end(), handle) != handles.end();
    }

    bool FillHoleOp::contains_edge(
        const std::vector<geometry::EdgeHandle>& handles,
        geometry::EdgeHandle handle) {
        return std::find(handles.begin(), handles.end(), handle) != handles.end();
    }

}
