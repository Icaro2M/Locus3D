/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/interaction/DragTool.h"
#include "editor/tools/mesh/core/MeshOperationSession.h"

#include <memory>

namespace locus::kernel::modeling {

    class IOperation;

} // namespace locus::kernel::modeling

namespace locus::editor {

    /**
     * @brief Base class for drag-driven interactive mesh operations.
     *
     * MeshDragOperationTool connects the generic DragTool lifecycle to a
     * non-destructive MeshOperationSession. Concrete tools remain responsible
     * for interpreting pointer movement, constructing kernel operations, and
     * committing the final result through the command and history systems.
     */
    class MeshDragOperationTool : public DragTool {
    public:
        /**
         * @brief Creates a drag-driven mesh operation tool.
         *
         * @param descriptor Static tool descriptor.
         * @param targetGranularity Mesh component granularity required by the
         * tool.
         * @param completionPolicy Pointer release completion policy.
         */
        MeshDragOperationTool(
            ToolDescriptor descriptor,
            SelectionGranularity targetGranularity,
            DragCompletionPolicy completionPolicy =
            DragCompletionPolicy::ConfirmOnRelease);

        /**
         * @brief Destroys the mesh drag operation tool.
         */
        ~MeshDragOperationTool() override = default;

        MeshDragOperationTool(
            const MeshDragOperationTool&) = delete;

        MeshDragOperationTool& operator=(
            const MeshDragOperationTool&) = delete;

        MeshDragOperationTool(
            MeshDragOperationTool&&) = default;

        MeshDragOperationTool& operator=(
            MeshDragOperationTool&&) = default;

        /**
         * @brief Returns the component granularity required by the tool.
         *
         * @return Required target granularity.
         */
        [[nodiscard]]
        SelectionGranularity target_granularity() const;

        /**
         * @brief Returns the current non-destructive mesh operation session.
         *
         * @return Read-only operation session.
         */
        [[nodiscard]]
        const MeshOperationSession& mesh_session() const;

        /**
         * @brief Returns the current operation preview.
         *
         * This is a convenience accessor for render and synchronization code.
         *
         * @return Current kernel operation preview.
         */
        [[nodiscard]]
        const kernel::modeling::OperationPreview&
            operation_preview() const;

        /**
         * @brief Checks whether a displayable preview is available.
         *
         * @return True when the mesh session owns a ready preview.
         */
        [[nodiscard]]
        bool has_operation_preview() const;

    protected:
        /**
         * @brief Returns mutable access to the mesh operation session.
         *
         * Concrete tools may configure preview generation before an interaction
         * starts. They should not clear or replace an active session directly.
         *
         * @return Mutable operation session.
         */
        [[nodiscard]]
        MeshOperationSession& mutable_mesh_session();

        /**
         * @brief Checks additional concrete-tool activation requirements.
         *
         * The base class already requires mesh editor mode.
         *
         * @param context Tool context.
         * @return True when the concrete tool may activate.
         */
        [[nodiscard]]
        virtual bool can_activate_mesh_tool(
            const ToolContext& context) const;

        /**
         * @brief Performs concrete activation work.
         *
         * @param context Tool context.
         * @return Activation result.
         */
        virtual ToolResult on_mesh_tool_activate(
            ToolContext& context);

        /**
         * @brief Performs concrete deactivation work.
         *
         * The shared mesh session has already been cleared when this hook is
         * called.
         *
         * @param context Tool context.
         * @return Deactivation result.
         */
        virtual ToolResult on_mesh_tool_deactivate(
            ToolContext& context);

        /**
         * @brief Captures the target used by a new interaction.
         *
         * The default implementation captures the active mesh selection using
         * target_granularity().
         *
         * @param context Tool context.
         * @param event Pointer press event.
         * @return Captured mesh target.
         */
        [[nodiscard]]
        virtual MeshToolTarget capture_mesh_target(
            const ToolContext& context,
            const ToolEvent& event) const;

        /**
         * @brief Checks concrete requirements for beginning an interaction.
         *
         * The supplied target is already structurally valid.
         *
         * @param context Tool context.
         * @param event Pointer press event.
         * @param target Candidate mesh target.
         * @return True when the concrete operation may begin.
         */
        [[nodiscard]]
        virtual bool can_begin_mesh_operation(
            const ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) const;

        /**
         * @brief Initializes concrete operation parameters.
         *
         * This hook runs after MeshOperationSession::begin() succeeds and before
         * the first preview operation is constructed.
         *
         * @param context Tool context.
         * @param event Pointer press event.
         * @param target Captured operation target.
         * @return Initialization result.
         */
        virtual ToolResult begin_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) = 0;

        /**
         * @brief Updates concrete operation parameters from pointer movement.
         *
         * Returning Ignored skips preview reconstruction for the event. Any
         * consumed non-failed result causes the base class to rebuild the
         * preview.
         *
         * @param context Tool context.
         * @param event Pointer movement event.
         * @param target Captured operation target.
         * @return Parameter update result.
         */
        virtual ToolResult update_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) = 0;

        /**
         * @brief Creates a kernel operation for the current preview parameters.
         *
         * The returned operation is executed against a ghost copy of the LEM.
         *
         * @param context Tool context.
         * @param target Captured operation target.
         * @return Owned configured operation, or null on construction failure.
         */
        [[nodiscard]]
        virtual std::unique_ptr<kernel::modeling::IOperation>
            build_preview_operation(
                const ToolContext& context,
                const MeshToolTarget& target) const = 0;

        /**
         * @brief Commits the current operation through command history.
         *
         * Implementations must not mutate the authoritative LEM directly.
         *
         * @param context Tool context.
         * @param target Captured operation target.
         * @return Commit result.
         */
        virtual ToolResult commit_mesh_operation(
            ToolContext& context,
            const MeshToolTarget& target) = 0;

        /**
         * @brief Clears concrete temporary parameters.
         *
         * This hook is called after confirmation, cancellation, failed startup,
         * or deactivation.
         */
        virtual void clear_mesh_operation();

    private:
        [[nodiscard]]
        bool can_activate_tool(
            const ToolContext& context) const final;

        ToolResult on_activate(
            ToolContext& context) final;

        ToolResult on_deactivate(
            ToolContext& context) final;

        [[nodiscard]]
        bool can_begin_drag(
            const ToolContext& context,
            const ToolEvent& event) const final;

        ToolResult on_begin_drag(
            ToolContext& context,
            const ToolEvent& event) final;

        ToolResult on_update_drag(
            ToolContext& context,
            const ToolEvent& event) final;

        ToolResult on_release_drag(
            ToolContext& context,
            const ToolEvent& event) final;

        ToolResult on_confirm_drag(
            ToolContext& context) final;

        ToolResult on_cancel_drag(
            ToolContext& context,
            ToolCancelReason reason) final;

        /**
         * @brief Rebuilds the preview using the current concrete parameters.
         *
         * @param context Tool context.
         * @return Preview reconstruction result.
         */
        ToolResult rebuild_operation_preview(
            ToolContext& context);

        /**
         * @brief Clears common and concrete interaction state.
         */
        void clear_operation_state();

        /**
         * @brief Clears common selection hover while mesh operation tools own feedback.
         *
         * @param context Tool context.
         * @return Dirty flags produced by hover clearing.
         */
        [[nodiscard]]
        EditorDirtyFlags clear_common_hover(
            ToolContext& context);

        /**
         * @brief Combines dirty flags and diagnostics from two results.
         *
         * The result code from primary is preserved.
         *
         * @param primary Primary result.
         * @param secondary Additional result.
         * @return Combined result.
         */
        [[nodiscard]]
        static ToolResult combine_results(
            ToolResult primary,
            const ToolResult& secondary);

        /**
         * @brief Returns a diagnostic for a cancellation reason.
         *
         * @param reason Cancellation reason.
         * @return Human-readable cancellation message.
         */
        [[nodiscard]]
        static const char* cancellation_message(
            ToolCancelReason reason);

        SelectionGranularity targetGranularity_ =
            SelectionGranularity::Face;

        MeshOperationSession meshSession_{};
    };

} // namespace locus::editor
