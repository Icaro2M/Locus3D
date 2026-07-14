/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ITool.h"
#include "editor/tools/interaction/ToolCancelReason.h"

namespace locus::editor {

    /**
     * @brief Base class for persistent tools with confirmable interactions.
     *
     * ModalTool centralizes activation, deactivation, confirmation, cancellation,
     * and lifecycle state transitions. Derived tools implement only their concrete
     * activation and interaction behavior.
     */
    class ModalTool : public ITool {
    public:
        /**
         * @brief Creates a modal tool.
         *
         * @param descriptor Static tool descriptor.
         */
        explicit ModalTool(ToolDescriptor descriptor);

        /**
         * @brief Destroys the modal tool.
         */
        ~ModalTool() override = default;

        ModalTool(const ModalTool&) = delete;
        ModalTool& operator=(const ModalTool&) = delete;
        ModalTool(ModalTool&&) = default;
        ModalTool& operator=(ModalTool&&) = default;

        /**
         * @brief Returns static tool metadata.
         *
         * @return Tool descriptor.
         */
        [[nodiscard]]
        const ToolDescriptor& descriptor() const final;

        /**
         * @brief Returns the current lifecycle state.
         *
         * @return Tool state.
         */
        [[nodiscard]]
        ToolState state() const final;

        /**
         * @brief Checks whether the tool can be activated.
         *
         * @param context Tool context.
         * @return True when activation is valid.
         */
        [[nodiscard]]
        bool can_activate(
            const ToolContext& context) const final;

        /**
         * @brief Activates the tool.
         *
         * @param context Tool context.
         * @return Activation result.
         */
        ToolResult activate(
            ToolContext& context) final;

        /**
         * @brief Deactivates the tool.
         *
         * An active interaction is cancelled before deactivation.
         *
         * @param context Tool context.
         * @return Deactivation result.
         */
        ToolResult deactivate(
            ToolContext& context) final;

        /**
         * @brief Handles a normalized editor event.
         *
         * @param context Tool context.
         * @param event Normalized event.
         * @return Event handling result.
         */
        ToolResult handle_event(
            ToolContext& context,
            const ToolEvent& event) final;

        /**
         * @brief Confirms the active interaction.
         *
         * @param context Tool context.
         * @return Confirmation result.
         */
        ToolResult confirm(
            ToolContext& context) final;

        /**
         * @brief Cancels the active interaction as a user request.
         *
         * @param context Tool context.
         * @return Cancellation result.
         */
        ToolResult cancel(
            ToolContext& context) final;

        /**
         * @brief Cancels the active interaction with an explicit reason.
         *
         * This method may be used by the tool manager when focus loss, tool
         * switching, or another external lifecycle event requires cancellation.
         *
         * @param context Tool context.
         * @param reason Cancellation reason.
         * @return Cancellation result.
         */
        ToolResult cancel(
            ToolContext& context,
            ToolCancelReason reason);

    protected:
        /**
         * @brief Checks tool-specific activation requirements.
         *
         * @param context Tool context.
         * @return True when the tool may activate.
         */
        [[nodiscard]]
        virtual bool can_activate_tool(
            const ToolContext& context) const;

        /**
         * @brief Performs tool-specific activation work.
         *
         * @param context Tool context.
         * @return Activation result.
         */
        virtual ToolResult on_activate(
            ToolContext& context);

        /**
         * @brief Performs tool-specific deactivation work.
         *
         * @param context Tool context.
         * @return Deactivation result.
         */
        virtual ToolResult on_deactivate(
            ToolContext& context);

        /**
         * @brief Handles a tool-specific normalized event.
         *
         * @param context Tool context.
         * @param event Normalized event.
         * @return Event handling result.
         */
        virtual ToolResult on_event(
            ToolContext& context,
            const ToolEvent& event) = 0;

        /**
         * @brief Confirms tool-specific temporary state.
         *
         * @param context Tool context.
         * @return Confirmation result.
         */
        virtual ToolResult on_confirm(
            ToolContext& context) = 0;

        /**
         * @brief Cancels tool-specific temporary state.
         *
         * @param context Tool context.
         * @param reason Cancellation reason.
         * @return Cancellation result.
         */
        virtual ToolResult on_cancel(
            ToolContext& context,
            ToolCancelReason reason) = 0;

        /**
         * @brief Marks the tool as interacting.
         *
         * @return True when the state changed from Ready to Interacting.
         */
        bool begin_interaction();

        /**
         * @brief Marks the current interaction as finished.
         *
         * The tool returns to Ready when active.
         */
        void finish_interaction();

        /**
         * @brief Changes the lifecycle state.
         *
         * Intended for specialized bases such as DragTool.
         *
         * @param state New state.
         */
        void set_state(ToolState state);

    private:
        ToolDescriptor descriptor_{};
        ToolState state_ = ToolState::Inactive;
    };

} // namespace locus::editor