/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/gizmo/GizmoController.h"
#include "editor/tools/transform/ITransformToolSession.h"
#include "editor/command/transform/NodeTransformChange.h"
#include "editor/command/CommandResult.h"

#include <vector>

namespace locus::editor {

    /**
     * @brief Interactive transform session for complete editor scene nodes.
     *
     * Live preview is delegated to GizmoController and TransformSession. Final
     * transforms are committed as one SetNodeTransformsCommand, so any number of
     * transformed objects produces one undo history entry.
     */
    class ObjectTransformToolSession final
        : public ITransformToolSession {
    public:
        /**
         * @brief Creates an object transform session.
         */
        ObjectTransformToolSession() = default;

        /**
         * @brief Creates a session with an explicit gizmo controller.
         *
         * @param controller Controller to store in the session.
         */
        explicit ObjectTransformToolSession(
            GizmoController controller);

        /**
         * @brief Checks whether the gizmo owns an active transform session.
         *
         * @return True when the underlying TransformSession is active.
         */
        [[nodiscard]]
        bool is_active() const override;

        /**
         * @brief Begins transformation from the current object selection.
         *
         * @param context Tool runtime context.
         * @param input Transform begin input.
         * @return Session start result.
         */
        ToolResult begin(
            ToolContext& context,
            const TransformToolSessionBeginInput& input) override;

        /**
         * @brief Updates object transform preview.
         *
         * @param context Tool runtime context.
         * @param input Transform update input.
         * @return Preview update result.
         */
        ToolResult update(
            ToolContext& context,
            const TransformToolSessionUpdateInput& input) override;

        /**
         * @brief Commits all object transforms as one command.
         *
         * @param context Tool runtime context.
         * @return Commit result.
         */
        ToolResult confirm(
            ToolContext& context) override;

        /**
         * @brief Restores every initial object transform.
         *
         * @param context Tool runtime context.
         * @param reason Cancellation reason.
         * @return Cancellation result.
         */
        ToolResult cancel(
            ToolContext& context,
            ToolCancelReason reason) override;

        /**
         * @brief Clears the underlying gizmo controller.
         */
        void clear() override;

        /**
         * @brief Returns mutable gizmo controller access.
         *
         * TransformTool uses this access to update hover state before a drag begins.
         *
         * @return Gizmo controller.
         */
        [[nodiscard]]
        GizmoController& controller();

        /**
         * @brief Returns read-only gizmo controller access.
         *
         * @return Gizmo controller.
         */
        [[nodiscard]]
        const GizmoController& controller() const;

    private:
        /**
         * @brief Builds absolute transform changes from captured targets.
         *
         * @return One change for every target whose preview differs from its
         * initial transform.
         */
        [[nodiscard]]
        std::vector<NodeTransformChange>
            build_changes() const;

        /**
         * @brief Converts a command result into a tool result.
         *
         * @param result Command execution result.
         * @return Converted result.
         */
        [[nodiscard]]
        static ToolResult from_command_result(
            CommandResult result);

        GizmoController controller_{};
    };

} // namespace locus::editor