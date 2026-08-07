/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/selection/shapes/BoxSelectionShape.h"

#include "editor/selection/SelectionGranularity.h"

namespace locus::editor {

    BoxSelectionShape::BoxSelectionShape(
        ScreenSelectionRect rect,
        const SelectionContainment containment,
        const SelectionDepthMode depthMode)
        : rect_(rect),
        containment_(containment),
        depthMode_(depthMode)
    {
    }

    SelectionShapeResult BoxSelectionShape::resolve(
        const ToolContext& context,
        const ToolEvent& event) const
    {
        SelectionShapeResult result{};

        const ScreenSelectionRect rect =
            rect_.clipped(event.pointer.viewportSize);

        if (rect.empty()) {
            return result;
        }

        if (is_mesh_granularity(context.selection().granularity())) {
            result.componentNode = context.selection().mesh().active_mesh();
            result.components =
                context.resolve_active_mesh_components(
                    rect,
                    event,
                    containment_,
                    depthMode_);

            if (!result.components.empty()) {
                result.component = result.components.back();
            }

            return result;
        }

        result.objects =
            context.resolve_objects_in_rect(
                rect,
                event,
                containment_);

        return result;
    }

    const ScreenSelectionRect& BoxSelectionShape::rect() const noexcept
    {
        return rect_;
    }

    void BoxSelectionShape::set_rect(
        const ScreenSelectionRect& rect) noexcept
    {
        rect_ = rect;
    }

    SelectionContainment BoxSelectionShape::containment() const noexcept
    {
        return containment_;
    }

    void BoxSelectionShape::set_containment(
        const SelectionContainment containment) noexcept
    {
        containment_ = containment;
    }

    SelectionDepthMode BoxSelectionShape::depth_mode() const noexcept
    {
        return depthMode_;
    }

    void BoxSelectionShape::set_depth_mode(
        const SelectionDepthMode depthMode) noexcept
    {
        depthMode_ = depthMode;
    }

} // namespace locus::editor
