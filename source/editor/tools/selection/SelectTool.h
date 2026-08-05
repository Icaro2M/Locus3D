/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/interaction/ModalTool.h"
#include "editor/tools/interaction/ToolCapture.h"
#include "editor/tools/selection/shapes/ISelectionShape.h"
#include "editor/tools/selection/shapes/BoxSelectionShape.h"

#include <memory>

namespace locus::editor {

    /**
     * @brief Persistent editor tool used for object and component selection.
     *
     * The initial implementation supports point-based object selection. Other
     * shapes and mesh component targets may be added without changing the tool
     * lifecycle or command routing.
     */
    class SelectTool final : public ModalTool {
    public:
        /**
         * @brief Stable identifier used by the tool registry.
         */
        static constexpr const char* Id = "editor.select";

        /**
         * @brief Creates a point-based selection tool.
         */
        SelectTool();

        /**
         * @brief Creates a selection tool using a custom shape strategy.
         *
         * @param shape Selection query strategy.
         */
        explicit SelectTool(
            std::unique_ptr<ISelectionShape> shape);

        /**
         * @brief Returns the default descriptor for this tool.
         *
         * @return Selection tool descriptor.
         */
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

        /**
         * @brief Returns the current selection shape.
         *
         * @return Read-only shape pointer.
         */
        [[nodiscard]]
        const ISelectionShape* shape() const;

        [[nodiscard]] bool is_box_selecting() const noexcept;
        [[nodiscard]] ScreenSelectionRect selection_rect() const noexcept;

    protected:
        /**
         * @brief Checks whether the selection tool can activate.
         *
         * @param context Tool context.
         * @return True when a valid shape strategy is available.
         */
        [[nodiscard]]
        bool can_activate_tool(
            const ToolContext& context) const override;

        /**
         * @brief Handles normalized selection events.
         *
         * Pointer movement updates ephemeral hover state. Primary pointer presses
         * execute persistent object selection commands.
         *
         * @param context Tool context.
         * @param event Normalized event.
         * @return Event handling result.
         */
        ToolResult on_event(
            ToolContext& context,
            const ToolEvent& event) override;

        /**
         * @brief Confirms an active modal interaction.
         *
         * Point selection does not currently own a confirmable interaction.
         *
         * @param context Tool context.
         * @return Ignored result.
         */
        ToolResult on_confirm(
            ToolContext& context) override;

        /**
         * @brief Cancels an active modal interaction.
         *
         * Point selection does not currently own temporary modal state.
         *
         * @param context Tool context.
         * @param reason Cancellation reason.
         * @return Ignored result.
         */
        ToolResult on_cancel(
            ToolContext& context,
            ToolCancelReason reason) override;

    private:
        /**
         * @brief Updates the ephemeral hovered object.
         *
         * @param context Tool context.
         * @param event Pointer movement event.
         * @return Event handling result.
         */
        ToolResult update_hover(
            ToolContext& context,
            const ToolEvent& event);

        /**
         * @brief Applies object selection for a primary pointer press.
         *
         * @param context Tool context.
         * @param event Pointer press event.
         * @return Command-backed selection result.
         */
        ToolResult apply_point_selection(
            ToolContext& context,
            const ToolEvent& event);

        ToolResult begin_pointer_selection(
            ToolContext& context,
            const ToolEvent& event);

        ToolResult update_pointer_selection(
            ToolContext& context,
            const ToolEvent& event);

        ToolResult finish_pointer_selection(
            ToolContext& context,
            const ToolEvent& event);

        ToolResult apply_box_selection(
            ToolContext& context,
            const ToolEvent& event);

        void clear_interaction_state();

        /**
         * @brief Converts command execution into a tool result.
         *
         * @param result Command execution result.
         * @param successMessage Message used when the command succeeds without one.
         * @return Converted tool result.
         */
        [[nodiscard]]
        static ToolResult from_command_result(
            CommandResult result,
            const char* successMessage);

        std::unique_ptr<ISelectionShape> shape_{};
        BoxSelectionShape boxShape_{};
        ToolCapture capture_{};
        SelectionOperation operation_ = SelectionOperation::Replace;
        bool boxSelecting_ = false;
        float dragThresholdPixels_ = 4.0f;
    };

} // namespace locus::editor
