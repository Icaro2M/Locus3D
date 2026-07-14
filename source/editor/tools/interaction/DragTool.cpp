/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/interaction/DragTool.h"

#include <utility>

namespace locus::editor {

    DragTool::DragTool(
        ToolDescriptor descriptor,
        DragCompletionPolicy completionPolicy)
        : ModalTool(std::move(descriptor)),
        completionPolicy_(completionPolicy) {
    }

    const ToolCapture& DragTool::capture() const {
        return capture_;
    }

    DragCompletionPolicy
        DragTool::completion_policy() const {

        return completionPolicy_;
    }

    bool DragTool::can_begin_drag(
        const ToolContext& context,
        const ToolEvent& event) const {

        (void)context;

        return
            state() == ToolState::Ready &&
            event.type == ToolEventType::PointerPress &&
            event.button == ToolPointerButton::Primary &&
            !capture_.is_active();
    }

    ToolResult DragTool::on_release_drag(
        ToolContext& context,
        const ToolEvent& event) {

        (void)context;
        (void)event;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Pointer drag released.");
    }

    ToolResult DragTool::on_event(
        ToolContext& context,
        const ToolEvent& event) {

        switch (event.type) {
        case ToolEventType::PointerPress:
            return handle_pointer_press(
                context,
                event);

        case ToolEventType::PointerMove:
            return handle_pointer_move(
                context,
                event);

        case ToolEventType::PointerRelease:
            return handle_pointer_release(
                context,
                event);

        default:
            return ToolResult::ignored();
        }
    }

    ToolResult DragTool::on_confirm(
        ToolContext& context) {

        ToolResult result = on_confirm_drag(context);

        if (!result.failed()) {
            capture_.clear();
        }

        return result;
    }

    ToolResult DragTool::on_cancel(
        ToolContext& context,
        ToolCancelReason reason) {

        ToolResult result = on_cancel_drag(
            context,
            reason);

        if (!result.failed()) {
            capture_.clear();
        }

        return result;
    }

    ToolResult DragTool::handle_pointer_press(
        ToolContext& context,
        const ToolEvent& event) {

        if (!can_begin_drag(context, event)) {
            return ToolResult::ignored();
        }

        ToolResult result = on_begin_drag(
            context,
            event);

        if (result.failed()) {
            capture_.clear();
            return result;
        }

        if (!result.was_consumed()) {
            return result;
        }

        if (!begin_interaction()) {
            capture_.clear();

            return ToolResult::fail(
                "The drag interaction could not enter the active state.");
        }

        capture_.begin_pointer(
            event.button,
            event.pointer.viewportPosition);

        if (result.code != ToolResultCode::Started) {
            result.code = ToolResultCode::Started;
        }

        return result;
    }

    ToolResult DragTool::handle_pointer_move(
        ToolContext& context,
        const ToolEvent& event) {

        if (state() != ToolState::Interacting ||
            !capture_.has_pointer()) {

            return ToolResult::ignored();
        }

        capture_.update_pointer(
            event.pointer.viewportPosition);

        ToolResult result = on_update_drag(
            context,
            event);

        if (!result.failed() &&
            result.was_consumed() &&
            result.code != ToolResultCode::Updated) {

            result.code = ToolResultCode::Updated;
        }

        return result;
    }

    ToolResult DragTool::handle_pointer_release(
        ToolContext& context,
        const ToolEvent& event) {

        if (state() != ToolState::Interacting ||
            !capture_.matches_button(event.button)) {

            return ToolResult::ignored();
        }

        capture_.update_pointer(
            event.pointer.viewportPosition);

        ToolResult releaseResult = on_release_drag(
            context,
            event);

        if (releaseResult.failed()) {
            return releaseResult;
        }

        capture_.clear();

        if (completionPolicy_ ==
            DragCompletionPolicy::WaitForExplicitConfirmation) {

            if (!releaseResult.was_consumed()) {
                return ToolResult::consumed(
                    releaseResult.dirtyFlags,
                    std::move(releaseResult.message));
            }

            return releaseResult;
        }

        ToolResult confirmResult = on_confirm_drag(context);

        confirmResult.dirtyFlags |=
            releaseResult.dirtyFlags;

        if (confirmResult.message.empty()) {
            confirmResult.message =
                std::move(releaseResult.message);
        }

        if (!confirmResult.failed()) {
            finish_interaction();

            if (confirmResult.code !=
                ToolResultCode::Confirmed) {

                confirmResult.code =
                    ToolResultCode::Confirmed;
            }
        }

        return confirmResult;
    }

} // namespace locus::editor