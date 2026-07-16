/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ToolResult.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/modeling/preview/GhostMeshBuilder.h"
#include "kernel/modeling/preview/OperationPreview.h"

#include <string>

namespace locus::kernel::modeling {

    class IOperation;

} // namespace locus::kernel::modeling

namespace locus::editor {

    class MeshNode;
    class ToolContext;

    /**
     * @brief Lifecycle state of an interactive mesh operation session.
     */
    enum class MeshOperationSessionState {
        /**
         * @brief No mesh interaction is currently active.
         */
        Inactive,

        /**
         * @brief A target is captured but no ready preview exists yet.
         */
        Active,

        /**
         * @brief A displayable operation preview is available.
         */
        PreviewReady,

        /**
         * @brief The most recent preview generation failed.
         */
        Failed
    };

    /**
     * @brief Manages the non-destructive preview state of a mesh tool.
     *
     * MeshOperationSession captures a stable MeshToolTarget and executes kernel
     * modeling operations against ghost copies of the authoritative LEM.
     *
     * The session does not own editor scene nodes, mutate the original mesh, or
     * execute undoable commands. Concrete tools remain responsible for creating
     * and executing the final command when an interaction is confirmed.
     */
    class MeshOperationSession {
    public:
        /**
         * @brief Creates an inactive mesh operation session.
         */
        MeshOperationSession() = default;

        /**
         * @brief Returns the current session state.
         *
         * @return Current lifecycle state.
         */
        [[nodiscard]]
        MeshOperationSessionState state() const;

        /**
         * @brief Checks whether a target is currently captured.
         *
         * @return True when the session is active.
         */
        [[nodiscard]]
        bool is_active() const;

        /**
         * @brief Checks whether a displayable preview is available.
         *
         * @return True when preview().is_ready().
         */
        [[nodiscard]]
        bool has_ready_preview() const;

        /**
         * @brief Checks whether the most recent preview generation failed.
         *
         * @return True when the session is in the Failed state.
         */
        [[nodiscard]]
        bool failed() const;

        /**
         * @brief Returns the captured mesh target.
         *
         * @return Read-only target reference.
         */
        [[nodiscard]]
        const MeshToolTarget& target() const;

        /**
         * @brief Returns the current operation preview.
         *
         * @return Read-only preview reference.
         */
        [[nodiscard]]
        const kernel::modeling::OperationPreview& preview() const;

        /**
         * @brief Returns the current ghost-mesh build options.
         *
         * @return Read-only preview options.
         */
        [[nodiscard]]
        const kernel::modeling::GhostMeshBuildOptions&
            preview_options() const;

        /**
         * @brief Changes the ghost-mesh build options.
         *
         * Changing these options invalidates any previously generated preview.
         *
         * @param options New preview build options.
         */
        void set_preview_options(
            kernel::modeling::GhostMeshBuildOptions options);

        /**
         * @brief Starts a session for a captured mesh target.
         *
         * The target is resolved against the current editor scene and every
         * stored component handle is checked against the authoritative LEM.
         *
         * @param context Tool runtime context.
         * @param target Target captured for the operation.
         * @return Started result or failure diagnostic.
         */
        ToolResult begin(
            const ToolContext& context,
            MeshToolTarget target);

        /**
         * @brief Rebuilds the non-destructive operation preview.
         *
         * The operation is executed against a temporary copy created by
         * GhostMeshBuilder. The authoritative MeshNode remains unchanged.
         *
         * @param context Tool runtime context.
         * @param operation Configured kernel operation.
         * @return Updated result or failure diagnostic.
         */
        ToolResult rebuild_preview(
            const ToolContext& context,
            kernel::modeling::IOperation& operation);

        /**
         * @brief Invalidates the current preview while keeping the target.
         *
         * @param message Optional invalidation reason.
         * @return Updated result requesting render synchronization.
         */
        ToolResult invalidate_preview(
            std::string message = {});

        /**
         * @brief Ends the interaction and discards all temporary state.
         *
         * No command or undo entry is produced.
         *
         * @param message Optional cancellation diagnostic.
         * @return Cancelled tool result.
         */
        ToolResult cancel(
            std::string message = {});

        /**
         * @brief Clears all session state without producing a ToolResult.
         */
        void clear();

    private:
        /**
         * @brief Resolves and validates the current target node.
         *
         * @param context Tool runtime context.
         * @param message Failure diagnostic output.
         * @return Resolved mesh node, or null on failure.
         */
        [[nodiscard]]
        const MeshNode* resolve_target(
            const ToolContext& context,
            std::string& message) const;

        /**
         * @brief Validates all captured handles against a LEM.
         *
         * @param node Resolved mesh node.
         * @param message Failure diagnostic output.
         * @return True when every captured handle remains active.
         */
        [[nodiscard]]
        bool validate_handles(
            const MeshNode& node,
            std::string& message) const;

        MeshOperationSessionState state_ =
            MeshOperationSessionState::Inactive;

        MeshToolTarget target_{};

        kernel::modeling::GhostMeshBuildOptions previewOptions_{};

        kernel::modeling::OperationPreview preview_{};
    };

} // namespace locus::editor