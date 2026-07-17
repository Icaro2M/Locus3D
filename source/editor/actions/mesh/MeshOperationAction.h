/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/IEditorAction.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"

#include <cstddef>
#include <functional>
#include <string>

namespace locus::editor {

    class MeshNode;

    /**
     * @brief Generic immediate action that applies an operation to the active
     * editable mesh.
     *
     * MeshOperationAction validates the current mesh-editing context, captures
     * the selected mesh components as a stable MeshToolTarget, and executes an
     * ApplyMeshOperationCommand.
     *
     * Concrete action registration code supplies the operation callback. Mesh
     * algorithms and topology mutations remain implemented by the geometry
     * kernel and modeling operation subsystems.
     *
     * Interactive operations requiring pointer movement, previews, or modal
     * confirmation belong to the tools subsystem instead.
     */
    class MeshOperationAction final : public IEditorAction {
    public:
        /**
         * @brief Factory creating the command operation callback.
         *
         * The supplied target is a stable value snapshot of the active mesh
         * node and selected component handles. The returned callback must own
         * every handle and parameter needed during command execution.
         *
         * @param target Captured mesh operation target.
         * @return Operation callback accepted by ApplyMeshOperationCommand.
         */
        using OperationFactory = std::function<
            ApplyMeshOperationCommand::MeshOperation(
                const MeshToolTarget& target)>;

        /**
         * @brief Optional additional availability predicate.
         *
         * Standard editor mode, active mesh, granularity, component count, and
         * handle checks are performed before this predicate.
         *
         * @param node Active editable mesh node.
         * @param target Captured component target.
         * @return True when action-specific requirements are satisfied.
         */
        using AvailabilityPredicate = std::function<
            bool(
                const MeshNode& node,
                const MeshToolTarget& target)>;

        /**
         * @brief Creates a generic immediate mesh operation action.
         *
         * @param descriptor Static action metadata.
         * @param requiredGranularity Required mesh component granularity.
         * @param minimumSelectionCount Minimum selected component count.
         * @param operationFactory Factory creating the command callback.
         * @param commandLabel Human-readable history entry label.
         * @param availability Optional additional availability predicate.
         */
        MeshOperationAction(
            ActionDescriptor descriptor,
            SelectionGranularity requiredGranularity,
            std::size_t minimumSelectionCount,
            OperationFactory operationFactory,
            std::string commandLabel,
            AvailabilityPredicate availability = {});

        /**
         * @brief Returns static action metadata.
         *
         * @return Action descriptor.
         */
        [[nodiscard]] const ActionDescriptor&
            descriptor() const override;

        /**
         * @brief Checks whether this operation can execute.
         *
         * @param context Current action context.
         * @return True when the current mesh target is valid.
         */
        [[nodiscard]] bool can_execute(
            const ActionContext& context) const override;

        /**
         * @brief Executes the configured operation through an undoable command.
         *
         * @param context Current action context.
         * @return Action execution result.
         */
        ActionResult execute(
            ActionContext& context) override;

        /**
         * @brief Returns the required component granularity.
         *
         * @return Required selection granularity.
         */
        [[nodiscard]] SelectionGranularity
            required_granularity() const;

        /**
         * @brief Returns the minimum required selection count.
         *
         * @return Minimum component count.
         */
        [[nodiscard]] std::size_t
            minimum_selection_count() const;

        /**
         * @brief Returns the command history label.
         *
         * @return Human-readable command label.
         */
        [[nodiscard]] const std::string&
            command_label() const;

    private:
        /**
         * @brief Captures the current mesh selection as a stable target.
         *
         * @param context Current action context.
         * @return Captured target.
         */
        [[nodiscard]] MeshToolTarget capture_target(
            const ActionContext& context) const;

        /**
         * @brief Resolves the mesh node referenced by a target.
         *
         * @param context Current action context.
         * @param target Target to resolve.
         * @return Mesh node, or null when unavailable.
         */
        [[nodiscard]] const MeshNode* find_target_node(
            const ActionContext& context,
            const MeshToolTarget& target) const;

        /**
         * @brief Validates a captured target against the current mesh.
         *
         * @param node Resolved mesh node.
         * @param target Captured target.
         * @return True when every captured handle remains active.
         */
        [[nodiscard]] bool validate_target_handles(
            const MeshNode& node,
            const MeshToolTarget& target) const;

        /**
         * @brief Performs all common availability checks.
         *
         * @param context Current action context.
         * @param target Captured target.
         * @param node Receives the resolved mesh node on success.
         * @return True when the operation is available.
         */
        [[nodiscard]] bool validate_context(
            const ActionContext& context,
            const MeshToolTarget& target,
            const MeshNode*& node) const;

        ActionDescriptor descriptor_{};
        SelectionGranularity requiredGranularity_ =
            SelectionGranularity::Object;
        std::size_t minimumSelectionCount_ = 1u;
        OperationFactory operationFactory_{};
        std::string commandLabel_{};
        AvailabilityPredicate availability_{};
    };

} // namespace locus::editor