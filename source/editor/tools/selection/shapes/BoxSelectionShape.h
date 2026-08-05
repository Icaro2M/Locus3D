/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/selection/shapes/ISelectionShape.h"

namespace locus::editor {

    class BoxSelectionShape final : public ISelectionShape {
    public:
        BoxSelectionShape() = default;

        explicit BoxSelectionShape(
            ScreenSelectionRect rect,
            SelectionContainment containment =
                SelectionContainment::Intersecting,
            SelectionDepthMode depthMode =
                SelectionDepthMode::VisibleOnly);

        [[nodiscard]] SelectionShapeResult resolve(
            const ToolContext& context,
            const ToolEvent& event) const override;

        [[nodiscard]] SelectionShapeKind kind() const noexcept override
        {
            return SelectionShapeKind::Box;
        }

        [[nodiscard]] const ScreenSelectionRect& rect() const noexcept;
        void set_rect(const ScreenSelectionRect& rect) noexcept;

        [[nodiscard]] SelectionContainment containment() const noexcept;
        void set_containment(SelectionContainment containment) noexcept;

        [[nodiscard]] SelectionDepthMode depth_mode() const noexcept;
        void set_depth_mode(SelectionDepthMode depthMode) noexcept;

    private:
        ScreenSelectionRect rect_{};
        SelectionContainment containment_ =
            SelectionContainment::Intersecting;
        SelectionDepthMode depthMode_ =
            SelectionDepthMode::VisibleOnly;
    };

} // namespace locus::editor
