/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/selection/shapes/PointSelectionShape.h"

namespace locus::editor {

    SelectionShapeResult PointSelectionShape::resolve(
        const ToolContext& context,
        const ToolEvent& event) const {

        SelectionShapeResult result{};

        if (!event.is_pointer_event() ||
            !event.pointer.has_picking_hit()) {
            result.component =
                context.resolve_active_mesh_component(event);
            if (result.component.hit) {
                result.componentNode =
                    context.selection().mesh().active_mesh();
            }
            return result;
        }

        const SceneNodeId nodeId =
            context.resolve_scene_node(
                event.pointer.pickingId);

        if (!nodeId.is_valid()) {
            result.component =
                context.resolve_active_mesh_component(event);
            if (result.component.hit) {
                result.componentNode =
                    context.selection().mesh().active_mesh();
            }
            return result;
        }

        result.objects.push_back(nodeId);

        result.component =
            context.resolve_mesh_component(nodeId, event);

        if (result.component.hit) {
            result.componentNode = nodeId;
            return result;
        }

        result.component =
            context.resolve_active_mesh_component(event);
        if (result.component.hit) {
            result.componentNode =
                context.selection().mesh().active_mesh();
        }
        return result;
    }

} // namespace locus::editor
