/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/topology/BridgeEdgeOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <cstdlib>
#include <glm/geometric.hpp>
#include <utility>

namespace locus::kernel::modeling {

    BridgeEdgeOp::BridgeEdgeOp(
        std::vector<geometry::VertexHandle> firstCycle,
        std::vector<geometry::VertexHandle> secondCycle)
        : mode_(BridgeEdgeMode::VertexCycles)
        , firstCycle_(std::move(firstCycle))
        , secondCycle_(std::move(secondCycle)) {
    }

    BridgeEdgeOp BridgeEdgeOp::edges(
        std::vector<geometry::EdgeHandle> firstEdges,
        std::vector<geometry::EdgeHandle> secondEdges) {
        BridgeEdgeOp op;
        op.set_edge_cycles(std::move(firstEdges), std::move(secondEdges));
        return op;
    }

    BridgeEdgeOp BridgeEdgeOp::selected_boundary_edges() {
        BridgeEdgeOp op;
        op.set_mode(BridgeEdgeMode::SelectedBoundaryEdges);
        return op;
    }

    std::string_view BridgeEdgeOp::name() const {
        return "BridgeEdgeOp";
    }

    void BridgeEdgeOp::set_mode(BridgeEdgeMode mode) {
        mode_ = mode;
    }

    BridgeEdgeMode BridgeEdgeOp::mode() const {
        return mode_;
    }

    void BridgeEdgeOp::set_vertex_cycles(
        std::vector<geometry::VertexHandle> firstCycle,
        std::vector<geometry::VertexHandle> secondCycle) {
        firstCycle_ = std::move(firstCycle);
        secondCycle_ = std::move(secondCycle);
        mode_ = BridgeEdgeMode::VertexCycles;
    }

    const std::vector<geometry::VertexHandle>& BridgeEdgeOp::first_cycle() const {
        return firstCycle_;
    }

    const std::vector<geometry::VertexHandle>& BridgeEdgeOp::second_cycle() const {
        return secondCycle_;
    }

    void BridgeEdgeOp::set_edge_cycles(
        std::vector<geometry::EdgeHandle> firstEdges,
        std::vector<geometry::EdgeHandle> secondEdges) {
        firstEdges_ = std::move(firstEdges);
        secondEdges_ = std::move(secondEdges);
        mode_ = BridgeEdgeMode::EdgeCycles;
    }

    const std::vector<geometry::EdgeHandle>& BridgeEdgeOp::first_edges() const {
        return firstEdges_;
    }

    const std::vector<geometry::EdgeHandle>& BridgeEdgeOp::second_edges() const {
        return secondEdges_;
    }

    void BridgeEdgeOp::clear_inputs() {
        firstCycle_.clear();
        secondCycle_.clear();
        firstEdges_.clear();
        secondEdges_.clear();
    }

    void BridgeEdgeOp::set_closed(bool closed) {
        closed_ = closed;
    }

    bool BridgeEdgeOp::closed() const {
        return closed_;
    }

    void BridgeEdgeOp::set_flip_second_cycle(bool flipSecondCycle) {
        flipSecondCycle_ = flipSecondCycle;
    }

    bool BridgeEdgeOp::flip_second_cycle() const {
        return flipSecondCycle_;
    }

    void BridgeEdgeOp::set_twist_offset(int twistOffset) {
        twistOffset_ = twistOffset;
    }

    int BridgeEdgeOp::twist_offset() const {
        return twistOffset_;
    }

    OperationResult BridgeEdgeOp::execute_impl(OperationContext& context) {
        geometry::LEM& mesh = context.editable_mesh();

        BridgeCycles cycles = collect_cycles(mesh);
        apply_cycle_options(cycles);

        if (!validate_cycles(mesh, cycles)) {
            return OperationResult::fail(
                kernel::ErrorCode::InvalidArgument,
                "BridgeEdge operation received incompatible cycles.");
        }

        orient_open_single_edge_bridge(mesh, cycles);

        geometry::LEMEditor editor(mesh);

        const std::size_t vertexCount = cycles.first.size();
        const std::size_t faceCount = closed_ ? vertexCount : vertexCount - 1;
        std::size_t createdFaces = 0;

        for (std::size_t i = 0; i < faceCount; ++i) {
            const std::size_t next = closed_ ? (i + 1) % vertexCount : i + 1;

            const geometry::VertexHandle firstA = cycles.first[i];
            const geometry::VertexHandle firstB = cycles.first[next];
            const geometry::VertexHandle secondB = cycles.second[next];
            const geometry::VertexHandle secondA = cycles.second[i];

            if (!mesh.is_valid(firstA) ||
                !mesh.is_valid(firstB) ||
                !mesh.is_valid(secondA) ||
                !mesh.is_valid(secondB)) {
                continue;
            }

            const geometry::FaceHandle face = editor.add_face({
                firstA,
                firstB,
                secondB,
                secondA
                });

            if (mesh.is_valid(face)) {
                ++createdFaces;

                if (context.rebuildNormals) {
                    editor.rebuild_normals_around_face(face);
                }
            }
        }

        if (createdFaces == 0) {
            return OperationResult::no_change(
                "BridgeEdge operation did not create any face.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    BridgeEdgeOp::BridgeCycles BridgeEdgeOp::collect_cycles(
        const geometry::LEM& mesh) const {
        switch (mode_) {
        case BridgeEdgeMode::VertexCycles:
            return BridgeCycles{ firstCycle_, secondCycle_ };

        case BridgeEdgeMode::EdgeCycles: {
            const ExtractedEdgeCycle first = order_edge_cycle(mesh, firstEdges_);
            const ExtractedEdgeCycle second = order_edge_cycle(mesh, secondEdges_);
            return BridgeCycles{ first.vertices, second.vertices };
        }

        case BridgeEdgeMode::SelectedBoundaryEdges: {
            const std::vector<ExtractedEdgeCycle> selectedCycles =
                collect_selected_boundary_cycles(mesh);

            if (selectedCycles.size() != 2) {
                return {};
            }

            return BridgeCycles{
                selectedCycles[0].vertices,
                selectedCycles[1].vertices
            };
        }
        }

        return {};
    }

    std::vector<BridgeEdgeOp::ExtractedEdgeCycle> BridgeEdgeOp::collect_selected_boundary_cycles(
        const geometry::LEM& mesh) const {
        std::vector<geometry::EdgeHandle> selectedEdges;

        for (geometry::EdgeHandle edge : geometry::TopologyTraversal::edges(mesh)) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (!mesh.edge(edge).selected) {
                continue;
            }

            if (!geometry::TopologyTraversal::is_boundary_edge(mesh, edge)) {
                continue;
            }

            selectedEdges.push_back(edge);
        }

        std::vector<ExtractedEdgeCycle> cycles;

        while (!selectedEdges.empty()) {
            const ExtractedEdgeCycle cycle = extract_one_edge_cycle(mesh, selectedEdges, false);

            if (cycle.vertices.empty() || cycle.edges.empty()) {
                return {};
            }

            cycles.push_back(cycle);

            std::vector<geometry::EdgeHandle> remaining;
            remaining.reserve(selectedEdges.size());

            for (geometry::EdgeHandle edge : selectedEdges) {
                if (!contains_edge(cycle.edges, edge)) {
                    remaining.push_back(edge);
                }
            }

            selectedEdges = std::move(remaining);

            if (cycles.size() > 2) {
                return {};
            }
        }

        return cycles;
    }

    BridgeEdgeOp::ExtractedEdgeCycle BridgeEdgeOp::order_edge_cycle(
        const geometry::LEM& mesh,
        const std::vector<geometry::EdgeHandle>& edges) const {
        if (edges.empty()) {
            return {};
        }

        if (edges.size() == 1) {
            const geometry::EdgeHandle edge = edges.front();

            if (!mesh.is_valid(edge)) {
                return {};
            }

            if (!geometry::TopologyTraversal::is_boundary_edge(mesh, edge)) {
                return {};
            }

            const geometry::Edge& edgeElement = mesh.edge(edge);

            if (!mesh.is_valid(edgeElement.vertexA) || !mesh.is_valid(edgeElement.vertexB)) {
                return {};
            }

            return ExtractedEdgeCycle{
                { edgeElement.vertexA, edgeElement.vertexB },
                { edge }
            };
        }

        return extract_one_edge_cycle(mesh, edges, true);
    }

    BridgeEdgeOp::ExtractedEdgeCycle BridgeEdgeOp::extract_one_edge_cycle(
        const geometry::LEM& mesh,
        const std::vector<geometry::EdgeHandle>& edges,
        bool requireAllEdges) const {
        if (edges.size() < 2) {
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

        if (uniqueEdges.size() < 2) {
            return {};
        }

        const geometry::Edge& firstEdge = mesh.edge(uniqueEdges.front());

        if (!mesh.is_valid(firstEdge.vertexA) || !mesh.is_valid(firstEdge.vertexB)) {
            return {};
        }

        ExtractedEdgeCycle result;
        result.vertices.reserve(uniqueEdges.size());
        result.edges.reserve(uniqueEdges.size());

        geometry::VertexHandle startVertex = firstEdge.vertexA;
        geometry::VertexHandle currentVertex = firstEdge.vertexA;
        geometry::VertexHandle nextVertex = firstEdge.vertexB;

        result.vertices.push_back(currentVertex);
        result.edges.push_back(uniqueEdges.front());

        currentVertex = nextVertex;

        while (currentVertex != startVertex) {
            result.vertices.push_back(currentVertex);

            geometry::EdgeHandle nextEdge{};
            geometry::VertexHandle candidateNextVertex{};

            for (geometry::EdgeHandle edgeHandle : uniqueEdges) {
                if (contains_edge(result.edges, edgeHandle)) {
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
                if (!closed_) {
                    if (requireAllEdges && result.edges.size() != uniqueEdges.size()) {
                        return {};
                    }

                    return result;
                }

                return {};
            }

            result.edges.push_back(nextEdge);
            currentVertex = candidateNextVertex;

            if (result.edges.size() > uniqueEdges.size()) {
                return {};
            }
        }

        if (closed_ && result.vertices.size() != result.edges.size()) {
            return {};
        }

        if (requireAllEdges && result.edges.size() != uniqueEdges.size()) {
            return {};
        }

        return result;
    }

    bool BridgeEdgeOp::validate_cycles(
        const geometry::LEM& mesh,
        const BridgeCycles& cycles) const {
        if (cycles.first.size() != cycles.second.size()) {
            return false;
        }

        if (closed_) {
            if (cycles.first.size() < 3) {
                return false;
            }
        }
        else {
            if (cycles.first.size() < 2) {
                return false;
            }
        }

        std::vector<geometry::VertexHandle> seen;
        seen.reserve(cycles.first.size() + cycles.second.size());

        for (geometry::VertexHandle vertex : cycles.first) {
            if (!mesh.is_valid(vertex)) {
                return false;
            }

            if (contains_vertex(seen, vertex)) {
                return false;
            }

            seen.push_back(vertex);
        }

        for (geometry::VertexHandle vertex : cycles.second) {
            if (!mesh.is_valid(vertex)) {
                return false;
            }

            if (contains_vertex(seen, vertex)) {
                return false;
            }

            seen.push_back(vertex);
        }

        return true;
    }

    void BridgeEdgeOp::apply_cycle_options(BridgeCycles& cycles) const {
        if (flipSecondCycle_) {
            std::reverse(cycles.second.begin(), cycles.second.end());
        }

        if (cycles.second.empty() || twistOffset_ == 0) {
            return;
        }

        const int count = static_cast<int>(cycles.second.size());
        int offset = twistOffset_ % count;

        if (offset < 0) {
            offset += count;
        }

        std::rotate(
            cycles.second.begin(),
            cycles.second.begin() + offset,
            cycles.second.end());
    }

    void BridgeEdgeOp::orient_open_single_edge_bridge(
        const geometry::LEM& mesh,
        BridgeCycles& cycles) const {
        if (closed_ ||
            mode_ == BridgeEdgeMode::VertexCycles ||
            flipSecondCycle_ ||
            twistOffset_ != 0 ||
            cycles.first.size() != 2u ||
            cycles.second.size() != 2u) {
            return;
        }

        const glm::vec3& firstA = mesh.vertex(cycles.first[0]).position;
        const glm::vec3& firstB = mesh.vertex(cycles.first[1]).position;
        const glm::vec3& secondA = mesh.vertex(cycles.second[0]).position;
        const glm::vec3& secondB = mesh.vertex(cycles.second[1]).position;

        const float currentCost =
            glm::dot(firstB - secondB, firstB - secondB) +
            glm::dot(secondA - firstA, secondA - firstA);
        const float reversedCost =
            glm::dot(firstB - secondA, firstB - secondA) +
            glm::dot(secondB - firstA, secondB - firstA);

        if (reversedCost < currentCost) {
            std::reverse(cycles.second.begin(), cycles.second.end());
        }
    }

    bool BridgeEdgeOp::contains_vertex(
        const std::vector<geometry::VertexHandle>& handles,
        geometry::VertexHandle handle) {
        return std::find(handles.begin(), handles.end(), handle) != handles.end();
    }

    bool BridgeEdgeOp::contains_edge(
        const std::vector<geometry::EdgeHandle>& handles,
        geometry::EdgeHandle handle) {
        return std::find(handles.begin(), handles.end(), handle) != handles.end();
    }

}
