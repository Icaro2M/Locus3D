/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/transform/ObjectTransformToolSession.h"

#include "editor/command/transform/SetNodeTransformsCommand.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/transform/TransformTarget.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        constexpr const char* NoGizmoHandleHitMessage =
            "No gizmo handle was hit.";

    } // namespace

    ObjectTransformToolSession::ObjectTransformToolSession(
        GizmoController controller)
        : controller_(std::move(controller)) {
    }

    bool ObjectTransformToolSession::is_active() const {
        return controller_.session().is_active();
    }

    ToolResult ObjectTransformToolSession::begin(
        ToolContext& context,
        const TransformToolSessionBeginInput& input) {

        if (is_active()) {
            return ToolResult::fail(
                "An object transform session is already active.");
        }

        if (context.selection().scope() != SelectionScope::Scene ||
            context.selection().granularity() !=
            SelectionGranularity::Object) {
            return ToolResult::fail(
                "Object transforms require object selection context.");
        }

        if (context.selection().objects().empty()) {
            return ToolResult::fail(
                "Cannot transform objects without an object selection.");
        }

        GizmoBeginDragInput beginInput{};
        beginInput.scene = &context.scene();
        beginInput.selection = &context.selection();
        beginInput.mode = input.mode;
        beginInput.orientation = input.orientation;
        beginInput.sessionOptions = input.options;
        beginInput.pointer = input.pointer;
        beginInput.snapSettings =
            &context.snap_settings();

        beginInput.snapSolver =
            input.snapSolver;

        const GizmoControllerResult result =
            controller_.begin_drag(beginInput);

        if (!result.success) {
            if (result.message != nullptr
                && std::string{ result.message }
                    == NoGizmoHandleHitMessage) {
                return ToolResult::ignored();
            }

            return ToolResult::fail(
                result.message != nullptr
                ? std::string{ result.message }
                : std::string{
                    "Could not begin object transform."
                });
        }

        if (!controller_.session().is_active()) {
            return ToolResult::fail(
                "The gizmo did not create an active object transform session.");
        }

        return ToolResult::started(
            EditorDirtyFlags::Render,
            result.message != nullptr
            ? std::string{ result.message }
            : std::string{
                "Object transform started."
            });
    }

    ToolResult ObjectTransformToolSession::update(
        ToolContext& context,
        const TransformToolSessionUpdateInput& input) {

        if (!is_active()) {
            return ToolResult::ignored();
        }

        GizmoDragInput dragInput{};
        dragInput.pointer = input.pointer;
        dragInput.snapSettings =
            &context.snap_settings();

        dragInput.snapSolver =
            input.snapSolver;

        const GizmoControllerResult result =
            controller_.update_drag(
                context.scene(),
                dragInput);

        if (!result.success) {
            return ToolResult::fail(
                result.message != nullptr
                ? std::string{ result.message }
                : std::string{
                    "Could not update object transform."
                });
        }

        if (!result.changed) {
            return ToolResult::consumed(
                EditorDirtyFlags::None,
                result.message != nullptr
                ? std::string{ result.message }
                : std::string{
                    "Object transform did not change."
                });
        }

        return ToolResult::updated(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            result.message != nullptr
            ? std::string{ result.message }
            : std::string{
                "Object transform preview updated."
            });
    }

    ToolResult ObjectTransformToolSession::confirm(
        ToolContext& context) {

        if (!is_active()) {
            return ToolResult::ignored();
        }

        if (!context.has_command_services()) {
            controller_.cancel_drag(
                context.scene());

            controller_.clear();

            return ToolResult::fail(
                "Object transform command services are not available.",
                EditorDirtyFlags::Scene |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        if (!controller_.session().has_changes()) {
            const bool ended =
                controller_.end_drag();

            controller_.clear();

            if (!ended) {
                return ToolResult::fail(
                    "Could not finish the unchanged object transform session.");
            }

            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Object transform finished without changes.");
        }

        std::vector<NodeTransformChange> changes =
            build_changes();

        if (changes.empty()) {
            controller_.cancel_drag(
                context.scene());

            controller_.clear();

            return ToolResult::fail(
                "The object transform session produced no valid changes.",
                EditorDirtyFlags::Scene |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        /*
         * Execute while TransformSession is still active. If command execution
         * fails, cancel_drag can still restore every captured initial transform.
         */
        CommandResult commandResult =
            context.execute_command(
                std::make_unique<
                SetNodeTransformsCommand>(
                    std::move(changes)));

        if (!commandResult.success) {
            controller_.cancel_drag(
                context.scene());

            controller_.clear();

            ToolResult failure =
                from_command_result(
                    std::move(commandResult));

            failure.dirtyFlags =
                failure.dirtyFlags |
                EditorDirtyFlags::Scene |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking;

            return failure;
        }

        const bool ended =
            controller_.end_drag();

        if (!ended) {
            /*
             * This should not occur after a successful command while the session
             * remains active. Do not remove the history entry here because the
             * command has already been executed successfully.
             */
            controller_.clear();

            return ToolResult::fail(
                "Object transforms were committed, but the gizmo session could not "
                "be finalized.",
                commandResult.dirtyFlags);
        }

        controller_.clear();

        return from_command_result(
            std::move(commandResult));
    }

    ToolResult ObjectTransformToolSession::cancel(
        ToolContext& context,
        ToolCancelReason reason) {

        (void)reason;

        if (!is_active()) {
            return ToolResult::ignored();
        }

        const bool restored =
            controller_.cancel_drag(
                context.scene());

        controller_.clear();

        if (!restored) {
            return ToolResult::fail(
                "Could not restore the object transform session.");
        }

        return ToolResult::cancelled(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Object transform cancelled.");
    }

    void ObjectTransformToolSession::clear() {
        controller_.clear();
    }

    GizmoController&
        ObjectTransformToolSession::controller() {

        return controller_;
    }

    const GizmoController&
        ObjectTransformToolSession::controller() const {

        return controller_;
    }

    std::vector<NodeTransformChange>
        ObjectTransformToolSession::build_changes() const {

        std::vector<NodeTransformChange> changes{};

        const std::vector<TransformTarget>& targets =
            controller_.session().targets();

        changes.reserve(targets.size());

        for (const TransformTarget& target : targets) {
            if (!target.has_transform_change()) {
                continue;
            }

            NodeTransformChange change{};
            change.node = target.node();
            change.previous =
                target.initial_transform();

            change.next =
                target.preview_transform();

            changes.push_back(std::move(change));
        }

        return changes;
    }

    ToolResult
        ObjectTransformToolSession::from_command_result(
            CommandResult result) {

        if (!result.success) {
            return ToolResult::fail(
                std::move(result.message),
                result.dirtyFlags);
        }

        std::string message =
            std::move(result.message);

        if (message.empty()) {
            message =
                "Object transforms committed.";
        }

        return ToolResult::confirmed(
            result.dirtyFlags,
            std::move(message));
    }

} // namespace locus::editor
