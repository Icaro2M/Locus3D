/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/edge/BevelOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cmath>
#include <glm/geometric.hpp>
#include <utility>

namespace locus::kernel::modeling {

    BevelOp::BevelOp(geometry::EdgeHandle edge, float width)
        : target_(BevelTarget::Edges)
        , edges_({ edge })
        , width_(width)
    {
    }

    BevelOp::BevelOp(
        std::vector<geometry::EdgeHandle> edges,
        float width)
        : target_(BevelTarget::Edges)
        , edges_(std::move(edges))
        , width_(width)
    {
    }

    BevelOp::BevelOp(
        std::vector<geometry::VertexHandle> vertices,
        float width)
        : target_(BevelTarget::SelectedVertices)
        , vertices_(std::move(vertices))
        , width_(width)
    {
    }

    BevelOp BevelOp::selected_edges(float width)
    {
        BevelOp op;
        op.set_target(BevelTarget::SelectedEdges);
        op.set_width(width);
        return op;
    }

    BevelOp BevelOp::selected_vertices(float width)
    {
        BevelOp op;
        op.set_target(BevelTarget::SelectedVertices);
        op.set_width(width);
        return op;
    }

    std::string_view BevelOp::name() const
    {
        return "BevelOp";
    }

    void BevelOp::set_target(BevelTarget target)
    {
        target_ = target;
    }

    BevelTarget BevelOp::target() const
    {
        return target_;
    }

    void BevelOp::set_width(float width)
    {
        width_ = width;
    }

    float BevelOp::width() const
    {
        return width_;
    }

    void BevelOp::set_edges(std::vector<geometry::EdgeHandle> edges)
    {
        edges_ = std::move(edges);
    }

    const std::vector<geometry::EdgeHandle>& BevelOp::edges() const
    {
        return edges_;
    }

    void BevelOp::clear_edges()
    {
        edges_.clear();
    }

    void BevelOp::set_vertices(std::vector<geometry::VertexHandle> vertices)
    {
        vertices_ = std::move(vertices);
    }

    const std::vector<geometry::VertexHandle>& BevelOp::vertices() const
    {
        return vertices_;
    }

    void BevelOp::clear_vertices()
    {
        vertices_.clear();
    }

    OperationResult BevelOp::execute_impl(OperationContext& context)
    {
        if (width_ <= 0.000001f) {
            return OperationResult::no_change(
                "Bevel operation has zero width.");
        }

        geometry::LEM& mesh = context.editable_mesh();

        const std::vector<geometry::EdgeHandle> targetEdges = collect_edges(mesh);
        const std::vector<geometry::VertexHandle> targetVertices =
            collect_vertices(mesh, targetEdges);

        if (targetVertices.empty()) {
            return OperationResult::no_change(
                "Bevel operation found no valid target vertices.");
        }

        const std::vector<geometry::FaceHandle> targetFaces =
            collect_faces(mesh, targetVertices);

        if (targetFaces.empty()) {
            return OperationResult::no_change(
                "Bevel operation found no affected faces.");
        }

        geometry::LEMEditor editor(mesh);
        std::size_t bevelCount = 0;

        for (geometry::FaceHandle face : targetFaces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            if (bevel_face(mesh, editor, face, targetVertices)) {
                ++bevelCount;
            }
        }

        if (bevelCount == 0) {
            return OperationResult::no_change(
                "Bevel operation did not modify the mesh.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::EdgeHandle> BevelOp::collect_edges(
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

        if (target_ == BevelTarget::SelectedVertices) {
            return result;
        }

        const std::vector<geometry::EdgeHandle> activeEdges =
            geometry::TopologyTraversal::edges(mesh);

        result.reserve(activeEdges.size());

        for (geometry::EdgeHandle edge : activeEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (target_ == BevelTarget::SelectedEdges &&
                !mesh.edge(edge).selected) {
                continue;
            }

            result.push_back(edge);
        }

        return result;
    }

    std::vector<geometry::VertexHandle> BevelOp::collect_vertices(
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

        if (target_ == BevelTarget::SelectedVertices) {
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

    std::vector<geometry::FaceHandle> BevelOp::collect_faces(
        const geometry::LEM& mesh,
        const std::vector<geometry::VertexHandle>& targetVertices) const
    {
        std::vector<geometry::FaceHandle> result;

        for (geometry::VertexHandle vertex : targetVertices) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            const std::vector<geometry::FaceHandle> vertexFaces =
                geometry::TopologyTraversal::vertex_faces(mesh, vertex);

            for (geometry::FaceHandle face : vertexFaces) {
                if (!mesh.is_valid(face) || contains(result, face)) {
                    continue;
                }

                result.push_back(face);
            }
        }

        return result;
    }

    bool BevelOp::bevel_face(
        geometry::LEM& mesh,
        geometry::LEMEditor& editor,
        geometry::FaceHandle face,
        const std::vector<geometry::VertexHandle>& targetVertices) const
    {
        if (!mesh.is_valid(face)) {
            return false;
        }

        const std::vector<geometry::VertexHandle> sourceVertices =
            geometry::TopologyTraversal::face_vertices(mesh, face);

        if (sourceVertices.size() < 3) {
            return false;
        }

        std::size_t affectedCornerCount = 0;
        for (geometry::VertexHandle vertex : sourceVertices) {
            if (contains(targetVertices, vertex)) {
                ++affectedCornerCount;
            }
        }

        if (affectedCornerCount == 0) {
            return false;
        }

        if (affectedCornerCount == sourceVertices.size()) {
            return false;
        }

        std::vector<geometry::VertexHandle> rebuiltFaceVertices;
        rebuiltFaceVertices.reserve(sourceVertices.size() + affectedCornerCount);

        std::vector<CornerCut> cornerCuts;
        cornerCuts.resize(sourceVertices.size());

        for (std::size_t i = 0; i < sourceVertices.size(); ++i) {
            const geometry::VertexHandle current = sourceVertices[i];

            if (!mesh.is_valid(current)) {
                return false;
            }

            if (!contains(targetVertices, current)) {
                rebuiltFaceVertices.push_back(current);
                continue;
            }

            const std::size_t previousIndex =
                (i + sourceVertices.size() - 1) % sourceVertices.size();
            const std::size_t nextIndex = (i + 1) % sourceVertices.size();

            const geometry::VertexHandle previous = sourceVertices[previousIndex];
            const geometry::VertexHandle next = sourceVertices[nextIndex];

            if (!mesh.is_valid(previous) || !mesh.is_valid(next)) {
                return false;
            }

            const glm::vec3 currentPosition = mesh.vertex(current).position;
            const glm::vec3 previousPosition = mesh.vertex(previous).position;
            const glm::vec3 nextPosition = mesh.vertex(next).position;

            const glm::vec3 previousDelta = previousPosition - currentPosition;
            const glm::vec3 nextDelta = nextPosition - currentPosition;

            const float previousLength = glm::length(previousDelta);
            const float nextLength = glm::length(nextDelta);

            if (previousLength <= 0.000001f || nextLength <= 0.000001f) {
                return false;
            }

            const float previousDistance = std::min(width_, previousLength * 0.45f);
            const float nextDistance = std::min(width_, nextLength * 0.45f);

            const glm::vec3 previousCutPosition =
                currentPosition + (previousDelta / previousLength) * previousDistance;
            const glm::vec3 nextCutPosition =
                currentPosition + (nextDelta / nextLength) * nextDistance;

            const geometry::VertexHandle previousSide =
                editor.add_vertex(previousCutPosition);
            const geometry::VertexHandle nextSide =
                editor.add_vertex(nextCutPosition);

            if (!mesh.is_valid(previousSide) || !mesh.is_valid(nextSide)) {
                return false;
            }

            cornerCuts[i] = CornerCut{ previousSide, nextSide };

            rebuiltFaceVertices.push_back(previousSide);
            rebuiltFaceVertices.push_back(nextSide);
        }

        if (rebuiltFaceVertices.size() < 3) {
            return false;
        }

        if (!editor.remove_face(face)) {
            return false;
        }

        std::size_t createdFaceCount = 0;

        const geometry::FaceHandle rebuiltFace =
            editor.add_face(rebuiltFaceVertices);

        if (mesh.is_valid(rebuiltFace)) {
            ++createdFaceCount;
        }

        for (std::size_t i = 0; i < sourceVertices.size(); ++i) {
            const geometry::VertexHandle sourceVertex = sourceVertices[i];

            if (!contains(targetVertices, sourceVertex)) {
                continue;
            }

            const CornerCut& cut = cornerCuts[i];

            if (!mesh.is_valid(sourceVertex) ||
                !mesh.is_valid(cut.previousSide) ||
                !mesh.is_valid(cut.nextSide)) {
                continue;
            }

            const geometry::FaceHandle chamferFace =
                editor.add_face({ cut.previousSide, sourceVertex, cut.nextSide });

            if (mesh.is_valid(chamferFace)) {
                ++createdFaceCount;
            }
        }

        return createdFaceCount > 0;
    }

}