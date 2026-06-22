/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/topology/MergeVerticesOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <algorithm>
#include <utility>

namespace locus::kernel::modeling {

    MergeVerticesOp::MergeVerticesOp(
        geometry::VertexHandle sourceVertex,
        geometry::VertexHandle targetVertex)
        : mode_(MergeVerticesMode::Pair)
        , sourceVertex_(sourceVertex)
        , targetVertex_(targetVertex)
    {
    }

    MergeVerticesOp::MergeVerticesOp(
        geometry::VertexHandle sourceVertex,
        geometry::VertexHandle targetVertex,
        const glm::vec3& position)
        : mode_(MergeVerticesMode::PairAtPosition)
        , sourceVertex_(sourceVertex)
        , targetVertex_(targetVertex)
        , position_(position)
    {
    }

    MergeVerticesOp::MergeVerticesOp(float distance)
        : mode_(MergeVerticesMode::Distance)
        , distance_(distance)
    {
    }

    MergeVerticesOp::MergeVerticesOp(
        std::vector<geometry::VertexHandle> vertices,
        float distance)
        : mode_(MergeVerticesMode::VertexSetDistance)
        , distance_(distance)
        , vertices_(std::move(vertices))
    {
    }

    std::string_view MergeVerticesOp::name() const
    {
        return "MergeVerticesOp";
    }

    void MergeVerticesOp::set_mode(MergeVerticesMode mode)
    {
        mode_ = mode;
    }

    MergeVerticesMode MergeVerticesOp::mode() const
    {
        return mode_;
    }

    void MergeVerticesOp::set_pair(
        geometry::VertexHandle sourceVertex,
        geometry::VertexHandle targetVertex)
    {
        mode_ = MergeVerticesMode::Pair;
        sourceVertex_ = sourceVertex;
        targetVertex_ = targetVertex;
    }

    void MergeVerticesOp::set_pair_at_position(
        geometry::VertexHandle sourceVertex,
        geometry::VertexHandle targetVertex,
        const glm::vec3& position)
    {
        mode_ = MergeVerticesMode::PairAtPosition;
        sourceVertex_ = sourceVertex;
        targetVertex_ = targetVertex;
        position_ = position;
    }

    geometry::VertexHandle MergeVerticesOp::source_vertex() const
    {
        return sourceVertex_;
    }

    geometry::VertexHandle MergeVerticesOp::target_vertex() const
    {
        return targetVertex_;
    }

    void MergeVerticesOp::set_position(const glm::vec3& position)
    {
        position_ = position;
    }

    const glm::vec3& MergeVerticesOp::position() const
    {
        return position_;
    }

    void MergeVerticesOp::set_distance(float distance)
    {
        distance_ = distance;
    }

    float MergeVerticesOp::distance() const
    {
        return distance_;
    }

    void MergeVerticesOp::set_vertices(std::vector<geometry::VertexHandle> vertices)
    {
        vertices_ = std::move(vertices);
    }

    const std::vector<geometry::VertexHandle>& MergeVerticesOp::vertices() const
    {
        return vertices_;
    }

    void MergeVerticesOp::clear_vertices()
    {
        vertices_.clear();
    }

    OperationResult MergeVerticesOp::execute_impl(OperationContext& context)
    {
        switch (mode_) {
        case MergeVerticesMode::Pair:
        case MergeVerticesMode::PairAtPosition:
            return execute_pair(context);

        case MergeVerticesMode::Distance:
            return execute_distance(context);

        case MergeVerticesMode::VertexSetDistance:
            return execute_vertex_set_distance(context);
        }

        return OperationResult::no_change("Merge vertices operation has no merge mode.");
    }

    OperationResult MergeVerticesOp::execute_pair(OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();

        if (!mesh.is_valid(sourceVertex_) || !mesh.is_valid(targetVertex_)) {
            return OperationResult::no_change(
                "Merge vertices operation has invalid source or target vertices.");
        }

        if (sourceVertex_ == targetVertex_) {
            return OperationResult::no_change(
                "Merge vertices operation source and target are the same vertex.");
        }

        geometry::LEMEditor editor(mesh);

        bool changed = false;

        if (mode_ == MergeVerticesMode::PairAtPosition) {
            changed = editor.merge_vertices_at_position(
                sourceVertex_,
                targetVertex_,
                position_);
        }
        else {
            changed = editor.merge_vertices(sourceVertex_, targetVertex_);
        }

        if (!changed) {
            return OperationResult::no_change(
                "Merge vertices operation did not modify the mesh.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    OperationResult MergeVerticesOp::execute_distance(OperationContext& context) const
    {
        if (distance_ <= 0.0f) {
            return OperationResult::no_change(
                "Merge vertices operation requires a positive distance.");
        }

        geometry::LEM& mesh = context.editable_mesh();
        geometry::LEMEditor editor(mesh);

        const std::size_t mergeCount = editor.merge_vertices_by_distance(distance_);

        if (mergeCount == 0) {
            return OperationResult::no_change(
                "Merge vertices operation found no vertices within the distance threshold.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    OperationResult MergeVerticesOp::execute_vertex_set_distance(OperationContext& context) const
    {
        if (distance_ <= 0.0f) {
            return OperationResult::no_change(
                "Merge vertices operation requires a positive distance.");
        }

        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::VertexHandle> targets = collect_valid_vertices(mesh);

        if (targets.size() < 2) {
            return OperationResult::no_change(
                "Merge vertices operation requires at least two valid vertices.");
        }

        geometry::LEMEditor editor(mesh);

        const std::size_t mergeCount = editor.weld_vertices(targets, distance_);

        if (mergeCount == 0) {
            return OperationResult::no_change(
                "Merge vertices operation found no vertices within the distance threshold.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::VertexHandle> MergeVerticesOp::collect_valid_vertices(
        const geometry::LEM& mesh) const
    {
        std::vector<geometry::VertexHandle> result;
        result.reserve(vertices_.size());

        for (geometry::VertexHandle vertexHandle : vertices_) {
            if (!mesh.is_valid(vertexHandle)) {
                continue;
            }

            const auto it = std::find(result.begin(), result.end(), vertexHandle);
            if (it != result.end()) {
                continue;
            }

            result.push_back(vertexHandle);
        }

        return result;
    }

}