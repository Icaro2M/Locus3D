/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/edge/CreaseOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <utility>

namespace locus::kernel::modeling {

    CreaseOp::CreaseOp(float crease)
        : crease_(crease)
    {
    }

    CreaseOp::CreaseOp(
        std::vector<geometry::EdgeHandle> edges,
        float crease)
        : edges_(std::move(edges))
        , crease_(crease)
    {
    }

    CreaseOp CreaseOp::selected(float crease)
    {
        CreaseOp op(crease);
        op.set_target(CreaseTarget::SelectedEdges);
        return op;
    }

    std::string_view CreaseOp::name() const
    {
        return "CreaseOp";
    }

    void CreaseOp::set_crease(float crease)
    {
        crease_ = crease;
    }

    float CreaseOp::crease() const
    {
        return crease_;
    }

    void CreaseOp::set_target(CreaseTarget target)
    {
        target_ = target;
    }

    CreaseTarget CreaseOp::target() const
    {
        return target_;
    }

    void CreaseOp::set_edges(std::vector<geometry::EdgeHandle> edges)
    {
        edges_ = std::move(edges);
    }

    const std::vector<geometry::EdgeHandle>& CreaseOp::edges() const
    {
        return edges_;
    }

    void CreaseOp::clear_edges()
    {
        edges_.clear();
    }

    OperationResult CreaseOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::EdgeHandle> targets = collect_edges(mesh);

        if (targets.empty()) {
            return OperationResult::no_change(
                "Crease operation has no valid target edges.");
        }

        geometry::LEMEditor editor(mesh);
        std::size_t changedCount = 0;

        for (geometry::EdgeHandle edge : targets) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (editor.set_crease(edge, crease_)) {
                ++changedCount;
            }
        }

        if (changedCount == 0) {
            return OperationResult::no_change(
                "Crease operation did not modify any edge.");
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::EdgeHandle> CreaseOp::collect_edges(
        const geometry::LEM& mesh) const
    {
        if (!edges_.empty()) {
            std::vector<geometry::EdgeHandle> result;
            result.reserve(edges_.size());

            for (geometry::EdgeHandle edge : edges_) {
                if (mesh.is_valid(edge)) {
                    result.push_back(edge);
                }
            }

            return result;
        }

        std::vector<geometry::EdgeHandle> result;

        for (geometry::EdgeHandle edge : geometry::TopologyTraversal::edges(mesh)) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (target_ == CreaseTarget::SelectedEdges &&
                !mesh.edge(edge).selected) {
                continue;
            }

            result.push_back(edge);
        }

        return result;
    }

}