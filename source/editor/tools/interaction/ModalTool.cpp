/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/interaction/ModalTool.h"

#include <utility>

namespace locus::editor {

    ModalTool::ModalTool(ToolDescriptor descriptor)
        : descriptor_(std::move(descriptor)) {
    }

    const ToolDescriptor& ModalTool::descriptor() const {
        return descriptor_;
    }

    ToolState ModalTool::state() const {
        return state_;
    }

    bool ModalTool::can_activate(
        const ToolContext& context) const {

        return
            state_ == ToolState::Inactive &&
            descriptor_.is_valid() &&
            can_activate_tool(context);
    }

    ToolResult ModalTool::activate(
        ToolContext& context) {

        if (state_ != ToolState::Inactive) {
            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "The tool is already active.");
        }

        if (!descriptor_.is_valid()) {
            return ToolResult::fail(
                "Cannot activate a tool with an invalid descriptor.");
        }

        if (!can_activate_tool(context)) {
            return ToolResult::fail(
                "The tool cannot be activated in the current editor state.");
        }

        ToolResult result = on_activate(context);

        if (result.failed()) {
            state_ = ToolState::Inactive;
            return result;
        }

        state_ = ToolState::Ready;
        return result;
    }

    ToolResult ModalTool::deactivate(
        ToolContext& context) {

        if (state_ == ToolState::Inactive) {
            return ToolResult::ignored();
        }

        EditorDirtyFlags dirtyFlags =
            EditorDirtyFlags::None;

        if (state_ == ToolState::Interacting) {
            ToolResult cancellation = cancel(
                context,
                ToolCancelReason::ToolDeactivated);

            dirtyFlags |= cancellation.dirtyFlags;

            if (cancellation.failed()) {
                return cancellation;
            }
        }

        ToolResult result = on_deactivate(context);
        result.dirtyFlags |= dirtyFlags;

        if (result.failed()) {
            return result;
        }

        state_ = ToolState::Inactive;
        return result;
    }

    ToolResult ModalTool::handle_event(
        ToolContext& context,
        const ToolEvent& event) {

        if (state_ == ToolState::Inactive ||
            state_ == ToolState::Suspended) {

            return ToolResult::ignored();
        }

        return on_event(context, event);
    }

    ToolResult ModalTool::confirm(
        ToolContext& context) {

        if (state_ != ToolState::Interacting) {
            return ToolResult::ignored();
        }

        ToolResult result = on_confirm(context);

        if (!result.failed() && result.is_terminal()) {
            finish_interaction();
        }

        return result;
    }

    ToolResult ModalTool::cancel(
        ToolContext& context) {

        return cancel(
            context,
            ToolCancelReason::UserRequest);
    }

    ToolResult ModalTool::cancel(
        ToolContext& context,
        ToolCancelReason reason) {

        if (state_ != ToolState::Interacting) {
            return ToolResult::ignored();
        }

        ToolResult result = on_cancel(
            context,
            reason);

        if (!result.failed()) {
            finish_interaction();
        }

        return result;
    }

    bool ModalTool::can_activate_tool(
        const ToolContext& context) const {

        (void)context;
        return true;
    }

    ToolResult ModalTool::on_activate(
        ToolContext& context) {

        (void)context;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Tool activated.");
    }

    ToolResult ModalTool::on_deactivate(
        ToolContext& context) {

        (void)context;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Tool deactivated.");
    }

    bool ModalTool::begin_interaction() {
        if (state_ != ToolState::Ready) {
            return false;
        }

        state_ = ToolState::Interacting;
        return true;
    }

    void ModalTool::finish_interaction() {
        if (state_ != ToolState::Inactive) {
            state_ = ToolState::Ready;
        }
    }

    void ModalTool::set_state(ToolState state) {
        state_ = state;
    }

} // namespace locus::editor