/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/selection/shapes/ISelectionShape.h"

namespace locus::editor {

    /**
     * @brief Selection shape that resolves one sampled picking identifier.
     *
     * PointSelectionShape consumes the PickingId already read by the application
     * at the current pointer position and converts it into an editor SceneNodeId
     * through ToolContext and PickingSync.
     */
    class PointSelectionShape final : public ISelectionShape {
    public:
        /**
         * @brief Creates a point selection shape.
         */
        PointSelectionShape() = default;

        /**
         * @brief Creates a point selection shape with explicit depth policy.
         *
         * @param depthMode Component depth policy.
         */
        explicit PointSelectionShape(
            SelectionDepthMode depthMode);

        /**
         * @brief Resolves the object under the event pointer.
         *
         * @param context Tool runtime context.
         * @param event Pointer event containing a sampled picking identifier.
         * @return Zero or one resolved scene object.
         */
        [[nodiscard]]
        SelectionShapeResult resolve(
            const ToolContext& context,
            const ToolEvent& event) const override;

        [[nodiscard]]
        SelectionShapeKind kind() const noexcept override
        {
            return SelectionShapeKind::Point;
        }

        [[nodiscard]] SelectionDepthMode depth_mode() const noexcept;
        void set_depth_mode(SelectionDepthMode depthMode) noexcept;

    private:
        SelectionDepthMode depthMode_ = SelectionDepthMode::VisibleOnly;
    };

} // namespace locus::editor
