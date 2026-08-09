/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/NodePivot.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/tools/interaction/DragTool.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace locus::editor {

    /**
     * @brief Viewport tool that edits the active object pivot interactively.
     */
    class PivotTool final : public DragTool {
    public:
        /**
         * @brief Stable identifier used by the tool registry.
         */
        static constexpr const char* Id = "editor.pivot";

        PivotTool();

        [[nodiscard]] static ToolDescriptor make_descriptor();

        [[nodiscard]] bool hovered() const noexcept;
        [[nodiscard]] bool dragging() const noexcept;
        [[nodiscard]] SceneNodeId active_node() const noexcept;

    protected:
        [[nodiscard]] bool can_activate_tool(
            const ToolContext& context) const override;

        ToolResult on_activate(
            ToolContext& context) override;

        ToolResult on_deactivate(
            ToolContext& context) override;

        ToolResult on_pointer_hover(
            ToolContext& context,
            const ToolEvent& event) override;

        ToolResult on_begin_drag(
            ToolContext& context,
            const ToolEvent& event) override;

        ToolResult on_update_drag(
            ToolContext& context,
            const ToolEvent& event) override;

        ToolResult on_release_drag(
            ToolContext& context,
            const ToolEvent& event) override;

        ToolResult on_confirm_drag(
            ToolContext& context) override;

        ToolResult on_cancel_drag(
            ToolContext& context,
            ToolCancelReason reason) override;

    private:
        [[nodiscard]] static SceneNodeId resolve_active_object(
            const ToolContext& context);

        [[nodiscard]] static bool project_to_viewport(
            const ToolEvent& event,
            const glm::vec3& worldPosition,
            glm::vec2& screenPosition);

        [[nodiscard]] static bool intersect_drag_plane(
            const ToolEvent& event,
            const glm::vec3& planePoint,
            const glm::vec3& planeNormal,
            glm::vec3& intersection);

        [[nodiscard]] bool hit_test(
            const ToolContext& context,
            const ToolEvent& event) const;

        [[nodiscard]] NodePivot pivot_from_world_position(
            const ToolContext& context,
            const glm::vec3& worldPosition) const;

        void apply_preview(
            ToolContext& context,
            const NodePivot& pivot);

        void restore_initial(
            ToolContext& context);

        void clear_state() noexcept;

        SceneNodeId activeNode_{};
        NodePivot initialPivot_{};
        NodePivot previewPivot_{};
        glm::vec3 initialPivotWorld_{ 0.0f, 0.0f, 0.0f };
        glm::vec3 dragPlaneNormal_{ 0.0f, 0.0f, -1.0f };
        bool hovered_ = false;
        bool dragging_ = false;
        bool changed_ = false;
        float hitRadiusPixels_ = 20.0f;
    };

} // namespace locus::editor
