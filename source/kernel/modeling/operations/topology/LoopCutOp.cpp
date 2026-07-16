/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/topology/LoopCutOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <utility>

namespace locus::kernel::modeling {

    LoopCutOp::LoopCutOp(geometry::EdgeHandle edge)
        : target_(LoopCutTarget::Edges)
        , edges_({ edge })
    {
    }

    LoopCutOp::LoopCutOp(std::vector<geometry::EdgeHandle> edges)
        : target_(LoopCutTarget::Edges)
        , edges_(std::move(edges))
    {
    }

    LoopCutOp LoopCutOp::selected_edges()
    {
        LoopCutOp op;
        op.set_target(LoopCutTarget::SelectedEdges);
        return op;
    }

    std::string_view LoopCutOp::name() const
    {
        return "LoopCutOp";
    }

    void LoopCutOp::set_target(LoopCutTarget target)
    {
        target_ = target;
    }

    LoopCutTarget LoopCutOp::target() const
    {
        return target_;
    }

    void LoopCutOp::set_edges(std::vector<geometry::EdgeHandle> edges)
    {
        edges_ = std::move(edges);
    }

    const std::vector<geometry::EdgeHandle>& LoopCutOp::edges() const
    {
        return edges_;
    }

    void LoopCutOp::clear_edges()
    {
        edges_.clear();
    }

    void LoopCutOp::set_cuts(std::size_t cuts)
    {
        cuts_ = std::max<std::size_t>(1, cuts);
    }

    std::size_t LoopCutOp::cuts() const
    {
        return cuts_;
    }

    void LoopCutOp::set_factor(float factor)
    {
        factor_ = glm::clamp(factor, 0.0001f, 0.9999f);
    }

    float LoopCutOp::factor() const
    {
        return factor_;
    }

    void LoopCutOp::set_even_spacing(bool useEvenSpacing)
    {
        evenSpacing_ = useEvenSpacing;
    }

    bool LoopCutOp::even_spacing() const
    {
        return evenSpacing_;
    }

    OperationResult LoopCutOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();

        const std::vector<geometry::EdgeHandle> targets = collect_edges(mesh);
        if (targets.empty()) {
            return OperationResult::no_change(
                "LoopCut operation found no valid edges.");
        }

        geometry::LEMEditor editor(mesh);

        std::vector<EdgeCut> edgeCuts;
        std::size_t splitCount = 0;

        for (geometry::EdgeHandle edge : targets) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            EdgeCut cut = cut_edge(editor, edge);
            if (cut.vertices.empty()) {
                continue;
            }

            splitCount += cut.vertices.size();
            edgeCuts.push_back(std::move(cut));
        }

        if (splitCount == 0) {
            return OperationResult::no_change(
                "LoopCut operation did not split any edge.");
        }

        [[maybe_unused]] const std::size_t connectedCutPairs =
            connect_edge_cuts(editor, edgeCuts);

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::EdgeHandle> LoopCutOp::collect_edges(
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

        const std::vector<geometry::EdgeHandle> activeEdges =
            geometry::TopologyTraversal::edges(mesh);

        result.reserve(activeEdges.size());

        for (geometry::EdgeHandle edge : activeEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (target_ == LoopCutTarget::SelectedEdges && !mesh.edge(edge).selected) {
                continue;
            }

            result.push_back(edge);
        }

        return result;
    }

    LoopCutOp::EdgeCut LoopCutOp::cut_edge(
        geometry::LEMEditor& editor,
        geometry::EdgeHandle edge) const
    {
        geometry::LEM& mesh = editor.mesh();

        EdgeCut cut;
        cut.originalEdge = edge;

        if (!mesh.is_valid(edge)) {
            return cut;
        }

        const geometry::Edge originalEdge = mesh.edge(edge);

        if (!mesh.is_valid(originalEdge.vertexA) ||
            !mesh.is_valid(originalEdge.vertexB)) {
            return cut;
        }

        geometry::VertexHandle previousVertex = originalEdge.vertexA;
        geometry::VertexHandle endVertex = originalEdge.vertexB;
        geometry::EdgeHandle currentEdge = edge;
        float previousFactor = 0.0f;

        for (std::size_t index = 0; index < cuts_; ++index) {
            if (!mesh.is_valid(currentEdge)) {
                currentEdge = mesh.find_edge(previousVertex, endVertex);
            }

            if (!mesh.is_valid(currentEdge)) {
                break;
            }

            float globalFactor = factor_;

            if (cuts_ > 1 || evenSpacing_) {
                globalFactor =
                    static_cast<float>(index + 1) / static_cast<float>(cuts_ + 1);
            }

            globalFactor = glm::clamp(globalFactor, 0.0001f, 0.9999f);

            const float localDenominator = 1.0f - previousFactor;
            if (localDenominator <= 0.0001f) {
                break;
            }

            float localFactor = (globalFactor - previousFactor) / localDenominator;
            localFactor = glm::clamp(localFactor, 0.0001f, 0.9999f);

            std::optional<geometry::VertexHandle> createdVertex =
                editor.split_edge_at_param(currentEdge, localFactor);

            if (!createdVertex.has_value() || !mesh.is_valid(createdVertex.value())) {
                break;
            }

            geometry::VertexHandle newVertex = createdVertex.value();
            cut.vertices.push_back(newVertex);

            previousVertex = newVertex;
            previousFactor = globalFactor;
            currentEdge = mesh.find_edge(previousVertex, endVertex);
        }

        return cut;
    }

    std::size_t LoopCutOp::connect_edge_cuts(
        geometry::LEMEditor& editor,
        const std::vector<EdgeCut>& edgeCuts) const
    {
        geometry::LEM& mesh = editor.mesh();

        if (edgeCuts.size() < 2) {
            return 0;
        }

        std::size_t splitCount = 0;

        for (std::size_t firstIndex = 0; firstIndex < edgeCuts.size(); ++firstIndex) {
            const EdgeCut& first = edgeCuts[firstIndex];

            if (first.vertices.empty()) {
                continue;
            }

            for (std::size_t secondIndex = firstIndex + 1;
                secondIndex < edgeCuts.size();
                ++secondIndex) {
                const EdgeCut& second = edgeCuts[secondIndex];

                if (second.vertices.empty()) {
                    continue;
                }

                const bool reverseSecond =
                    should_reverse_cut_order(mesh, first, second);

                const std::size_t pairCount =
                    std::min(first.vertices.size(), second.vertices.size());

                for (std::size_t index = 0; index < pairCount; ++index) {
                    const std::size_t secondVertexIndex =
                        reverseSecond
                            ? second.vertices.size() - index - 1
                            : index;

                    if (connect_cut_pair(
                        editor,
                        first.vertices[index],
                        second.vertices[secondVertexIndex])) {
                        ++splitCount;
                    }
                }
            }
        }

        return splitCount;
    }

    bool LoopCutOp::connect_cut_pair(
        geometry::LEMEditor& editor,
        geometry::VertexHandle vertexA,
        geometry::VertexHandle vertexB) const
    {
        geometry::LEM& mesh = editor.mesh();

        if (!mesh.is_valid(vertexA) ||
            !mesh.is_valid(vertexB) ||
            vertexA == vertexB) {
            return false;
        }

        if (mesh.find_edge(vertexA, vertexB).is_valid()) {
            return false;
        }

        const std::vector<geometry::FaceHandle> faces =
            geometry::TopologyTraversal::faces(mesh);

        for (geometry::FaceHandle face : faces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            const std::vector<geometry::VertexHandle> faceVertices =
                geometry::TopologyTraversal::face_vertices(mesh, face);

            if (faceVertices.size() < 4) {
                continue;
            }

            if (!contains(faceVertices, vertexA) ||
                !contains(faceVertices, vertexB)) {
                continue;
            }

            if (editor.split_face(face, vertexA, vertexB).has_value()) {
                return true;
            }
        }

        return false;
    }

    bool LoopCutOp::should_reverse_cut_order(
        const geometry::LEM& mesh,
        const EdgeCut& first,
        const EdgeCut& second) const
    {
        const std::size_t pairCount =
            std::min(first.vertices.size(), second.vertices.size());

        if (pairCount == 0) {
            return false;
        }

        float directDistance = 0.0f;
        float reversedDistance = 0.0f;
        std::size_t validPairs = 0;

        for (std::size_t index = 0; index < pairCount; ++index) {
            const geometry::VertexHandle firstVertex = first.vertices[index];
            const geometry::VertexHandle directSecondVertex = second.vertices[index];
            const geometry::VertexHandle reversedSecondVertex =
                second.vertices[second.vertices.size() - index - 1];

            if (!mesh.is_valid(firstVertex) ||
                !mesh.is_valid(directSecondVertex) ||
                !mesh.is_valid(reversedSecondVertex)) {
                continue;
            }

            const glm::vec3 firstPosition = mesh.vertex(firstVertex).position;
            const glm::vec3 directDelta =
                firstPosition - mesh.vertex(directSecondVertex).position;
            const glm::vec3 reversedDelta =
                firstPosition - mesh.vertex(reversedSecondVertex).position;

            directDistance += glm::dot(directDelta, directDelta);
            reversedDistance += glm::dot(reversedDelta, reversedDelta);
            ++validPairs;
        }

        return validPairs > 0 && reversedDistance < directDistance;
    }

}
