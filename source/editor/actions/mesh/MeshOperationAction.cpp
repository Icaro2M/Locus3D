/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/mesh/MeshOperationAction.h"

#include "editor/actions/core/ActionContext.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/MeshSelection.h"
#include "editor/selection/SelectionState.h"

#include <memory>
#include <utility>

namespace locus::editor {

    MeshOperationAction::MeshOperationAction(
        ActionDescriptor descriptor,
        SelectionGranularity requiredGranularity,
        std::size_t minimumSelectionCount,
        OperationFactory operationFactory,
        std::string commandLabel,
        AvailabilityPredicate availability)
        : descriptor_(std::move(descriptor)),
        requiredGranularity_(requiredGranularity),
        minimumSelectionCount_(minimumSelectionCount),
        operationFactory_(std::move(operationFactory)),
        commandLabel_(std::move(commandLabel)),
        availability_(std::move(availability)) {
        if (minimumSelectionCount_ == 0u) {
            minimumSelectionCount_ = 1u;
        }

        if (commandLabel_.empty()) {
            commandLabel_ = descriptor_.name;
        }

        if (commandLabel_.empty()) {
            commandLabel_ = "Apply Mesh Operation";
        }
    }

    const ActionDescriptor&
        MeshOperationAction::descriptor() const {
        return descriptor_;
    }

    bool MeshOperationAction::can_execute(
        const ActionContext& context) const {
        const MeshToolTarget target =
            capture_target(context);

        const MeshNode* node = nullptr;

        return validate_context(
            context,
            target,
            node);
    }

    ActionResult MeshOperationAction::execute(
        ActionContext& context) {
        const MeshToolTarget target =
            capture_target(context);

        const MeshNode* node = nullptr;

        if (!validate_context(
            context,
            target,
            node)) {
            return ActionResult::unavailable(
                "The mesh operation is not available for the current "
                "selection.");
        }

        ApplyMeshOperationCommand::MeshOperation operation =
            operationFactory_(target);

        if (!operation) {
            return ActionResult::fail(
                "The mesh action did not create an operation callback.");
        }

        CommandResult result =
            context.execute_command(
                std::make_unique<ApplyMeshOperationCommand>(
                    target.nodeId,
                    std::move(operation),
                    commandLabel_));

        return ActionResult::from_command(
            std::move(result));
    }

    SelectionGranularity
        MeshOperationAction::required_granularity() const {
        return requiredGranularity_;
    }

    std::size_t
        MeshOperationAction::minimum_selection_count() const {
        return minimumSelectionCount_;
    }

    const std::string&
        MeshOperationAction::command_label() const {
        return commandLabel_;
    }

    MeshToolTarget MeshOperationAction::capture_target(
        const ActionContext& context) const {
        return MeshToolTarget::capture(
            context.selection().mesh(),
            requiredGranularity_);
    }

    const MeshNode*
        MeshOperationAction::find_target_node(
            const ActionContext& context,
            const MeshToolTarget& target) const {
        if (!target.has_node()) {
            return nullptr;
        }

        return context.scene().find_mesh(target.nodeId);
    }

    bool MeshOperationAction::validate_target_handles(
        const MeshNode& node,
        const MeshToolTarget& target) const {
        const kernel::geometry::LEM& mesh =
            node.mesh();

        switch (target.granularity) {
        case SelectionGranularity::Vertex:
            for (const kernel::geometry::VertexHandle handle
                : target.vertices) {
                if (!mesh.is_valid(handle)) {
                    return false;
                }
            }
            return !target.vertices.empty();

        case SelectionGranularity::Edge:
            for (const kernel::geometry::EdgeHandle handle
                : target.edges) {
                if (!mesh.is_valid(handle)) {
                    return false;
                }
            }
            return !target.edges.empty();

        case SelectionGranularity::Loop:
            for (const kernel::geometry::LoopHandle handle
                : target.loops) {
                if (!mesh.is_valid(handle)) {
                    return false;
                }
            }
            return !target.loops.empty();

        case SelectionGranularity::Face:
            for (const kernel::geometry::FaceHandle handle
                : target.faces) {
                if (!mesh.is_valid(handle)) {
                    return false;
                }
            }
            return !target.faces.empty();

        case SelectionGranularity::Object:
            return false;
        }

        return false;
    }

    bool MeshOperationAction::validate_context(
        const ActionContext& context,
        const MeshToolTarget& target,
        const MeshNode*& node) const {
        node = nullptr;

        if (!descriptor_.is_valid()
            || !operationFactory_
            || !is_mesh_granularity(requiredGranularity_)
            || context.selection().granularity()
            != requiredGranularity_
            || target.granularity
            != requiredGranularity_
            || !target.is_valid()
            || target.component_count()
            < minimumSelectionCount_) {
            return false;
        }

        node = find_target_node(
            context,
            target);

        if (!node
            || !validate_target_handles(
                *node,
                target)) {
            node = nullptr;
            return false;
        }

        if (availability_
            && !availability_(
                *node,
                target)) {
            node = nullptr;
            return false;
        }

        return true;
    }

} // namespace locus::editor
