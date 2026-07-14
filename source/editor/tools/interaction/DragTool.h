/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/interaction/ModalTool.h"
#include "editor/tools/interaction/ToolCapture.h"

namespace locus::editor {

    /**
     * @brief Policy applied when the pointer button ending a drag is released.
     */
    enum class DragCompletionPolicy {
        /**
         * @brief Pointer release confirms and completes the interaction.
         */
        ConfirmOnRelease,

        /**
         * @brief Pointer release ends capture but leaves the interaction active.
         *
         * A separate semantic Confirm or Cancel event completes the operation.
         */
        WaitForExplicitConfirmation
    };

    /**
     * @brief Base class for tools driven by pointer drag interactions.
     *
     * DragTool centralizes pointer capture and dispatches begin, update, release,
     * confirmation, and cancellation to specialized tool implementations.
     */
    class DragTool : public ModalTool {
    public:
        /**
         * @brief Creates a drag-based tool.
         *
         * @param descriptor Static tool descriptor.
         * @param completionPolicy Pointer-release completion policy.
         */
        explicit DragTool(
            ToolDescriptor descriptor,
            DragCompletionPolicy completionPolicy =
            DragCompletionPolicy::ConfirmOnRelease);

        /**
         * @brief Destroys the drag tool.
         */
        ~DragTool() override = default;

        /**
         * @brief Returns read-only pointer capture state.
         *
         * @return Capture state.
         */
        [[nodiscard]]
        const ToolCapture& capture() const;

        /**
         * @brief Returns the pointer-release completion policy.
         *
         * @return Completion policy.
         */
        [[nodiscard]]
        DragCompletionPolicy completion_policy() const;

    protected:
        /**
         * @brief Checks whether an event may begin a drag.
         *
         * The default accepts a primary pointer press while the tool is Ready.
         *
         * @param context Tool context.
         * @param event Candidate event.
         * @return True when a drag may begin.
         */
        [[nodiscard]]
        virtual bool can_begin_drag(
            const ToolContext& context,
            const ToolEvent& event) const;

        /**
         * @brief Starts concrete drag state.
         *
         * @param context Tool context.
         * @param event Pointer-press event.
         * @return Drag start result.
         */
        virtual ToolResult on_begin_drag(
            ToolContext& context,
            const ToolEvent& event) = 0;

        /**
         * @brief Updates concrete drag state.
         *
         * @param context Tool context.
         * @param event Pointer-move event.
         * @return Drag update result.
         */
        virtual ToolResult on_update_drag(
            ToolContext& context,
            const ToolEvent& event) = 0;

        /**
         * @brief Handles release of the captured pointer.
         *
         * This hook may update the final preview state before automatic or explicit
         * confirmation.
         *
         * @param context Tool context.
         * @param event Pointer-release event.
         * @return Release result.
         */
        virtual ToolResult on_release_drag(
            ToolContext& context,
            const ToolEvent& event);

        /**
         * @brief Confirms concrete drag state.
         *
         * Persistent changes should be committed through the command/history
         * system in implementations of this method.
         *
         * @param context Tool context.
         * @return Confirmation result.
         */
        virtual ToolResult on_confirm_drag(
            ToolContext& context) = 0;

        /**
         * @brief Cancels concrete drag state.
         *
         * @param context Tool context.
         * @param reason Cancellation reason.
         * @return Cancellation result.
         */
        virtual ToolResult on_cancel_drag(
            ToolContext& context,
            ToolCancelReason reason) = 0;

    private:
        ToolResult on_event(
            ToolContext& context,
            const ToolEvent& event) final;

        ToolResult on_confirm(
            ToolContext& context) final;

        ToolResult on_cancel(
            ToolContext& context,
            ToolCancelReason reason) final;

        ToolResult handle_pointer_press(
            ToolContext& context,
            const ToolEvent& event);

        ToolResult handle_pointer_move(
            ToolContext& context,
            const ToolEvent& event);

        ToolResult handle_pointer_release(
            ToolContext& context,
            const ToolEvent& event);

        ToolCapture capture_{};

        DragCompletionPolicy completionPolicy_ =
            DragCompletionPolicy::ConfirmOnRelease;
    };

} // namespace locus::editor