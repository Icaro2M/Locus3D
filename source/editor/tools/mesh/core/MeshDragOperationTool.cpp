/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/core/MeshDragOperationTool.h"

#include "editor/tools/core/ToolContext.h"
#include "kernel/modeling/core/IOperation.h"

#include <utility>

namespace locus::editor {

    MeshDragOperationTool::MeshDragOperationTool(
        ToolDescriptor descriptor,
        SelectionGranularity targetGranularity,
        DragCompletionPolicy completionPolicy)
        : DragTool(
            std::move(descriptor),
            completionPolicy),
        targetGranularity_(targetGranularity)
    {
    }

    SelectionGranularity
        MeshDragOperationTool::target_granularity() const
    {
        return targetGranularity_;
    }

    const MeshOperationSession&
        MeshDragOperationTool::mesh_session() const
    {
        return meshSession_;
    }

    MeshOperationSession&
        MeshDragOperationTool::mutable_mesh_session()
    {
        return meshSession_;
    }

    const kernel::modeling::OperationPreview&
        MeshDragOperationTool::operation_preview() const
    {
        return meshSession_.preview();
    }

    bool MeshDragOperationTool::has_operation_preview() const
    {
        return meshSession_.has_ready_preview();
    }

    bool MeshDragOperationTool::can_activate_mesh_tool(
        const ToolContext& context) const
    {
        (void)context;
        return true;
    }

    ToolResult MeshDragOperationTool::on_mesh_tool_activate(
        ToolContext& context)
    {
        (void)context;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Mesh operation tool activated.");
    }

    ToolResult MeshDragOperationTool::on_mesh_tool_deactivate(
        ToolContext& context)
    {
        (void)context;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Mesh operation tool deactivated.");
    }

    MeshToolTarget
        MeshDragOperationTool::capture_mesh_target(
            const ToolContext& context,
            const ToolEvent& event) const
    {
        (void)event;

        return MeshToolTarget::capture(
            context.selection().mesh(),
            targetGranularity_);
    }

    bool MeshDragOperationTool::can_begin_mesh_operation(
        const ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target) const
    {
        (void)context;
        (void)event;
        (void)target;

        return true;
    }

    void MeshDragOperationTool::clear_mesh_operation()
    {
    }

    bool MeshDragOperationTool::can_activate_tool(
        const ToolContext& context) const
    {
        return context.mode() == EditorMode::Mesh &&
            can_activate_mesh_tool(context);
    }

    ToolResult MeshDragOperationTool::on_activate(
        ToolContext& context)
    {
        clear_operation_state();

        return on_mesh_tool_activate(context);
    }

    ToolResult MeshDragOperationTool::on_deactivate(
        ToolContext& context)
    {
        clear_operation_state();

        ToolResult result =
            on_mesh_tool_deactivate(context);

        result.dirtyFlags |= EditorDirtyFlags::Render;

        return result;
    }

    bool MeshDragOperationTool::can_begin_drag(
        const ToolContext& context,
        const ToolEvent& event) const
    {
        if (!DragTool::can_begin_drag(
            context,
            event)) {
            return false;
        }

        if (context.mode() != EditorMode::Mesh) {
            return false;
        }

        const MeshToolTarget target =
            capture_mesh_target(
                context,
                event);

        if (!target.is_valid()) {
            return false;
        }

        return can_begin_mesh_operation(
            context,
            event,
            target);
    }

    ToolResult MeshDragOperationTool::on_begin_drag(
        ToolContext& context,
        const ToolEvent& event)
    {
        MeshToolTarget target =
            capture_mesh_target(
                context,
                event);

        if (!target.is_valid()) {
            return ToolResult::ignored();
        }

        ToolResult sessionResult =
            meshSession_.begin(
                context,
                std::move(target));

        if (sessionResult.failed()) {
            clear_operation_state();
            return sessionResult;
        }

        ToolResult beginResult =
            begin_mesh_operation(
                context,
                event,
                meshSession_.target());

        if (beginResult.failed()) {
            clear_operation_state();

            beginResult.dirtyFlags |=
                EditorDirtyFlags::Render;

            return beginResult;
        }

        if (!beginResult.was_consumed()) {
            clear_operation_state();
            return beginResult;
        }

        ToolResult previewResult =
            rebuild_operation_preview(context);

        if (previewResult.failed()) {
            clear_operation_state();

            previewResult.dirtyFlags |=
                EditorDirtyFlags::Render;

            return previewResult;
        }

        ToolResult result =
            combine_results(
                std::move(beginResult),
                sessionResult);

        result =
            combine_results(
                std::move(result),
                previewResult);

        return result;
    }

    ToolResult MeshDragOperationTool::on_update_drag(
        ToolContext& context,
        const ToolEvent& event)
    {
        if (!meshSession_.is_active()) {
            return ToolResult::ignored();
        }

        ToolResult updateResult =
            update_mesh_operation(
                context,
                event,
                meshSession_.target());

        if (updateResult.failed() ||
            !updateResult.was_consumed()) {
            return updateResult;
        }

        ToolResult previewResult =
            rebuild_operation_preview(context);

        return combine_results(
            std::move(previewResult),
            updateResult);
    }

    ToolResult MeshDragOperationTool::on_release_drag(
        ToolContext& context,
        const ToolEvent& event)
    {
        (void)context;
        (void)event;

        if (!meshSession_.is_active()) {
            return ToolResult::ignored();
        }

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Mesh operation pointer released.");
    }

    ToolResult MeshDragOperationTool::on_confirm_drag(
        ToolContext& context)
    {
        if (!meshSession_.is_active()) {
            return ToolResult::ignored();
        }

        /*
         * A valid interaction may still contain an Empty preview, for example
         * when the pointer returns to the operation's neutral parameter. Treat
         * confirmation in that state as a successful no-op instead of creating
         * an unnecessary history entry.
         */
        if (!meshSession_.has_ready_preview()) {
            clear_operation_state();

            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Mesh operation completed without changes.");
        }

        const MeshToolTarget target =
            meshSession_.target();

        ToolResult result =
            commit_mesh_operation(
                context,
                target);

        if (result.failed()) {
            return result;
        }

        clear_operation_state();

        result.dirtyFlags |=
            EditorDirtyFlags::Render;

        if (result.message.empty()) {
            result.message =
                "Mesh operation committed.";
        }

        return result;
    }

    ToolResult MeshDragOperationTool::on_cancel_drag(
        ToolContext& context,
        ToolCancelReason reason)
    {
        (void)context;

        if (!meshSession_.is_active()) {
            clear_mesh_operation();
            return ToolResult::ignored();
        }

        ToolResult result =
            meshSession_.cancel(
                cancellation_message(reason));

        clear_mesh_operation();

        return result;
    }

    ToolResult
        MeshDragOperationTool::rebuild_operation_preview(
            ToolContext& context)
    {
        if (!meshSession_.is_active()) {
            return ToolResult::fail(
                "Cannot rebuild a mesh preview without an active session.");
        }

        std::unique_ptr<kernel::modeling::IOperation> operation =
            build_preview_operation(
                context,
                meshSession_.target());

        if (!operation) {
            return ToolResult::fail(
                "The mesh tool could not create its preview operation.",
                EditorDirtyFlags::Render);
        }

        return meshSession_.rebuild_preview(
            context,
            *operation);
    }

    void MeshDragOperationTool::clear_operation_state()
    {
        meshSession_.clear();
        clear_mesh_operation();
    }

    ToolResult MeshDragOperationTool::combine_results(
        ToolResult primary,
        const ToolResult& secondary)
    {
        primary.dirtyFlags |=
            secondary.dirtyFlags;

        if (primary.message.empty()) {
            primary.message =
                secondary.message;
        }

        return primary;
    }

    const char*
        MeshDragOperationTool::cancellation_message(
            ToolCancelReason reason)
    {
        switch (reason) {
        case ToolCancelReason::UserRequest:
            return "Mesh operation cancelled by the user.";

        case ToolCancelReason::FocusLost:
            return "Mesh operation cancelled after focus loss.";

        case ToolCancelReason::ToolSwitch:
            return "Mesh operation cancelled after switching tools.";

        case ToolCancelReason::ToolDeactivated:
            return "Mesh operation cancelled during tool deactivation.";

        case ToolCancelReason::InvalidState:
            return "Mesh operation cancelled because its state became "
                "invalid.";
        }

        return "Mesh operation cancelled.";
    }

} // namespace locus::editor