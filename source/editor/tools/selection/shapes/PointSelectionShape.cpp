/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/selection/shapes/PointSelectionShape.h"

namespace locus::editor {

    PointSelectionShape::PointSelectionShape(
        const SelectionDepthMode depthMode)
        : depthMode_(depthMode)
    {
    }

    SelectionShapeResult PointSelectionShape::resolve(
        const ToolContext& context,
        const ToolEvent& event) const {

        SelectionShapeResult result{};

        if (!event.is_pointer_event() ||
            !event.pointer.has_picking_hit()) {
            result.component =
                context.resolve_active_mesh_component(
                    event,
                    depthMode_);
            if (result.component.hit) {
                result.componentNode =
                    context.selection().mesh().active_mesh();
                result.components.push_back(result.component);
            }
            return result;
        }

        const SceneNodeId nodeId =
            context.resolve_scene_node(
                event.pointer.pickingId);

        if (!nodeId.is_valid()) {
            result.component =
                context.resolve_active_mesh_component(
                    event,
                    depthMode_);
            if (result.component.hit) {
                result.componentNode =
                    context.selection().mesh().active_mesh();
                result.components.push_back(result.component);
            }
            return result;
        }

        result.objects.push_back(nodeId);

        result.component =
            context.resolve_mesh_component(
                nodeId,
                event,
                depthMode_);

        if (result.component.hit) {
            result.componentNode = nodeId;
            result.components.push_back(result.component);
            return result;
        }

        result.component =
            context.resolve_active_mesh_component(
                event,
                depthMode_);
        if (result.component.hit) {
            result.componentNode =
                context.selection().mesh().active_mesh();
            result.components.push_back(result.component);
        }
        return result;
    }

    SelectionDepthMode PointSelectionShape::depth_mode() const noexcept
    {
        return depthMode_;
    }

    void PointSelectionShape::set_depth_mode(
        const SelectionDepthMode depthMode) noexcept
    {
        depthMode_ = depthMode;
    }

} // namespace locus::editor
