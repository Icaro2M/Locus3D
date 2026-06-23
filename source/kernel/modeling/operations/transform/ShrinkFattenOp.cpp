/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/transform/ShrinkFattenOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cmath>
#include <utility>

namespace locus::kernel::modeling {

    ShrinkFattenOp::ShrinkFattenOp(float distance)
        : distance_(distance)
    {
    }

    ShrinkFattenOp::ShrinkFattenOp(
        std::vector<geometry::VertexHandle> vertices,
        float distance)
        : vertices_(std::move(vertices))
        , distance_(distance)
    {
    }

    ShrinkFattenOp ShrinkFattenOp::selected(float distance)
    {
        ShrinkFattenOp op(distance);
        op.set_target(ShrinkFattenTarget::SelectedVertices);
        return op;
    }

    std::string_view ShrinkFattenOp::name() const
    {
        return "ShrinkFattenOp";
    }

    void ShrinkFattenOp::set_distance(float distance)
    {
        distance_ = distance;
    }

    float ShrinkFattenOp::distance() const
    {
        return distance_;
    }

    void ShrinkFattenOp::set_target(ShrinkFattenTarget target)
    {
        target_ = target;
    }

    ShrinkFattenTarget ShrinkFattenOp::target() const
    {
        return target_;
    }

    void ShrinkFattenOp::set_vertices(std::vector<geometry::VertexHandle> vertices)
    {
        vertices_ = std::move(vertices);
    }

    const std::vector<geometry::VertexHandle>& ShrinkFattenOp::vertices() const
    {
        return vertices_;
    }

    void ShrinkFattenOp::clear_vertices()
    {
        vertices_.clear();
    }

    OperationResult ShrinkFattenOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::VertexHandle> targets = collect_vertices(mesh);

        if (targets.empty()) {
            return OperationResult::no_change(
                "Shrink/fatten operation has no valid target vertices.");
        }

        if (std::abs(distance_) <= 0.0f) {
            return OperationResult::no_change(
                "Shrink/fatten operation has zero distance.");
        }

        geometry::LEMEditor editor(mesh);
        std::size_t changedCount = 0;

        for (geometry::VertexHandle vertex : targets) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            if (editor.offset_vertex_along_normal(vertex, distance_)) {
                ++changedCount;
            }
        }

        if (changedCount == 0) {
            return OperationResult::no_change(
                "Shrink/fatten operation did not modify any vertex.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::VertexHandle> ShrinkFattenOp::collect_vertices(
        const geometry::LEM& mesh) const
    {
        if (!vertices_.empty()) {
            std::vector<geometry::VertexHandle> result;
            result.reserve(vertices_.size());

            for (geometry::VertexHandle vertex : vertices_) {
                if (mesh.is_valid(vertex)) {
                    result.push_back(vertex);
                }
            }

            return result;
        }

        std::vector<geometry::VertexHandle> result;

        for (geometry::VertexHandle vertex : geometry::TopologyTraversal::vertices(mesh)) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            if (target_ == ShrinkFattenTarget::SelectedVertices &&
                !mesh.vertex(vertex).selected) {
                continue;
            }

            result.push_back(vertex);
        }

        return result;
    }

}