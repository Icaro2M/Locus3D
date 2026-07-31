/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/edge/BridgeEdgeTool.h"

#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/topology/BridgeEdgeOp.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        [[nodiscard]]
        kernel::modeling::BridgeEdgeOp make_bridge_operation(
            kernel::geometry::EdgeHandle firstEdge,
            kernel::geometry::EdgeHandle secondEdge,
            const BridgeEdgeToolOptions& options)
        {
            kernel::modeling::BridgeEdgeOp operation =
                kernel::modeling::BridgeEdgeOp::edges(
                    std::vector<kernel::geometry::EdgeHandle>{
                        firstEdge },
                    std::vector<kernel::geometry::EdgeHandle>{
                        secondEdge });

            operation.set_closed(options.closed);
            operation.set_flip_second_cycle(
                options.flipSecondCycle);
            operation.set_twist_offset(
                options.twistOffset);

            return operation;
        }

        [[nodiscard]]
        std::string bridge_failure_message(
            const kernel::modeling::OperationResult& result)
        {
            if (result.is_failure()) {
                if (!result.error().message.empty()) {
                    return result.error().message;
                }

                return "Bridge Edge operation failed.";
            }

            if (!result.message().empty()) {
                return result.message();
            }

            return "Bridge Edge operation did not modify the mesh.";
        }

        [[nodiscard]]
        ToolResult validate_bridge_input(
            const kernel::geometry::LEM& mesh,
            kernel::geometry::EdgeHandle firstEdge,
            kernel::geometry::EdgeHandle secondEdge,
            const BridgeEdgeToolOptions& options)
        {
            if (!mesh.is_valid(firstEdge) ||
                !mesh.is_valid(secondEdge)) {
                return ToolResult::fail(
                    "Bridge Edge selection contains an invalid edge.");
            }

            if (firstEdge == secondEdge) {
                return ToolResult::fail(
                    "Bridge Edge requires two distinct selected edges.");
            }

            if (!kernel::geometry::TopologyTraversal::is_boundary_edge(
                    mesh,
                    firstEdge) ||
                !kernel::geometry::TopologyTraversal::is_boundary_edge(
                    mesh,
                    secondEdge)) {
                return ToolResult::fail(
                    "Bridge Edge requires two boundary edges. Closed cube "
                    "edges cannot be bridged directly.");
            }

            kernel::geometry::LEM previewMesh = mesh;
            kernel::modeling::BridgeEdgeOp operation =
                make_bridge_operation(
                    firstEdge,
                    secondEdge,
                    options);

            kernel::modeling::OperationContext operationContext{};
            operationContext.mesh = &previewMesh;
            operationContext.validateAfterExecute =
                options.validateAfterExecute;
            operationContext.rebuildNormals =
                options.rebuildNormals;
            operationContext.allowNonManifold =
                options.allowNonManifold;

            const kernel::modeling::OperationResult result =
                operation.execute(operationContext);

            if (!result.is_success() || !result.changed()) {
                return ToolResult::fail(
                    bridge_failure_message(result));
            }

            return ToolResult::consumed();
        }

        [[nodiscard]]
        ToolResult from_command_result(
            CommandResult result,
            std::string successMessage)
        {
            if (!result.success) {
                return ToolResult::fail(
                    std::move(result.message),
                    result.dirtyFlags);
            }

            std::string message =
                std::move(result.message);

            if (message.empty()) {
                message = std::move(successMessage);
            }

            return ToolResult::confirmed(
                result.dirtyFlags,
                std::move(message));
        }

    } // namespace

    BridgeEdgeTool::BridgeEdgeTool()
        : BridgeEdgeTool(
            BridgeEdgeToolOptions{}) {
    }

    BridgeEdgeTool::BridgeEdgeTool(
        BridgeEdgeToolOptions options)
        : ModalTool(make_descriptor())
        , options_(options) {
    }

    const BridgeEdgeToolOptions& BridgeEdgeTool::options() const {
        return options_;
    }

    bool BridgeEdgeTool::set_options(
        const BridgeEdgeToolOptions& options) {

        if (state() != ToolState::Inactive &&
            state() != ToolState::Ready) {
            return false;
        }

        options_ = options;
        return true;
    }

    bool BridgeEdgeTool::can_activate_tool(
        const ToolContext& context) const {

        const SelectionState& selection =
            context.selection();

        const MeshSelection& meshSelection =
            selection.mesh();

        return
            context.has_command_services() &&
            meshSelection.active_mesh().is_valid() &&
            selection.granularity() ==
                SelectionGranularity::Edge &&
            meshSelection.edges().size() == 2u;
    }

    ToolResult BridgeEdgeTool::on_activate(
        ToolContext& context) {

        const MeshToolTarget target =
            MeshToolTarget::capture(
                context.selection().mesh(),
                SelectionGranularity::Edge);

        return commit_bridge(
            context,
            target);
    }

    ToolResult BridgeEdgeTool::on_event(
        ToolContext& context,
        const ToolEvent& event) {

        (void)context;
        (void)event;

        return ToolResult::ignored();
    }

    ToolResult BridgeEdgeTool::on_confirm(
        ToolContext& context) {

        (void)context;

        return ToolResult::ignored();
    }

    ToolResult BridgeEdgeTool::on_cancel(
        ToolContext& context,
        ToolCancelReason reason) {

        (void)context;
        (void)reason;

        return ToolResult::ignored();
    }

    ToolResult BridgeEdgeTool::commit_bridge(
        ToolContext& context,
        const MeshToolTarget& target) const {

        if (!target.is_valid() ||
            !target.targets_edges()) {
            return ToolResult::fail(
                "Bridge Edge requires an active edge selection.");
        }

        if (target.edges.size() != 2u) {
            return ToolResult::fail(
                "Bridge Edge requires exactly two selected edges.");
        }

        MeshNode* node =
            context.scene().find_mesh(target.nodeId);

        if (node == nullptr) {
            return ToolResult::fail(
                "Bridge Edge could not find the active mesh.");
        }

        const kernel::geometry::EdgeHandle firstEdge =
            target.edges[0];
        const kernel::geometry::EdgeHandle secondEdge =
            target.edges[1];
        const BridgeEdgeToolOptions options = options_;

        const ToolResult validationResult =
            validate_bridge_input(
                node->mesh(),
                firstEdge,
                secondEdge,
                options);

        if (validationResult.failed()) {
            return validationResult;
        }

        auto command =
            std::make_unique<ApplyMeshOperationCommand>(
                target.nodeId,
                [firstEdge, secondEdge, options](
                    kernel::geometry::LEMEditor& editor) {

                    kernel::modeling::BridgeEdgeOp operation =
                        make_bridge_operation(
                            firstEdge,
                            secondEdge,
                            options);

                    kernel::modeling::OperationContext operationContext{};
                    operationContext.mesh = &editor.mesh();
                    operationContext.validateAfterExecute =
                        options.validateAfterExecute;
                    operationContext.rebuildNormals =
                        options.rebuildNormals;
                    operationContext.allowNonManifold =
                        options.allowNonManifold;

                    const kernel::modeling::OperationResult result =
                        operation.execute(operationContext);

                    return result.is_success() && result.changed();
                },
                "Bridge Edge");

        return from_command_result(
            context.execute_command(std::move(command)),
            "Edges bridged.");
    }

    ToolDescriptor BridgeEdgeTool::make_descriptor() {
        return ToolDescriptor{
            ToolId{ std::string{ Id } },
            "Bridge Edge",
            "Creates a face bridge between two selected boundary edges.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection
        };
    }

} // namespace locus::editor
