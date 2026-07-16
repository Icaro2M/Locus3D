/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/edge/EdgeSlideTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/edge/EdgeSlideOp.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        constexpr float minimumVisualScale =
            0.000001f;

    } // namespace

    EdgeSlideTool::EdgeSlideTool()
        : EdgeSlideTool(
            EdgeSlideToolOptions{})
    {
    }

    EdgeSlideTool::EdgeSlideTool(
        EdgeSlideToolOptions options)
        : MeshDragOperationTool(
            make_descriptor(),
            SelectionGranularity::Edge,
            DragCompletionPolicy::ConfirmOnRelease),
        options_(
            sanitize_options(
                std::move(options)))
    {
    }

    const EdgeSlideToolOptions&
        EdgeSlideTool::options() const
    {
        return options_;
    }

    bool EdgeSlideTool::set_options(
        const EdgeSlideToolOptions& options)
    {
        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ =
            sanitize_options(options);

        return true;
    }

    float EdgeSlideTool::distance() const
    {
        return distance_;
    }

    ToolResult EdgeSlideTool::begin_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;

        if (!target.targets_edges()) {
            return ToolResult::fail(
                "Edge slide requires at least one selected edge.");
        }

        startPosition_ =
            event.pointer.viewportPosition;

        interactionVisualScale_ =
            event.pointer.visualScale > minimumVisualScale
            ? event.pointer.visualScale
            : 1.0f;

        distance_ = 0.0f;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Edge slide interaction started.");
    }

    ToolResult EdgeSlideTool::update_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;
        (void)target;

        const float previousDistance =
            distance_;

        distance_ =
            calculate_distance(event);

        if (std::abs(
            distance_ - previousDistance) <=
            options_.distanceEpsilon) {
            return ToolResult::ignored();
        }

        return ToolResult::updated(
            EditorDirtyFlags::None,
            "Edge slide distance updated.");
    }

    std::unique_ptr<kernel::modeling::IOperation>
        EdgeSlideTool::build_preview_operation(
            const ToolContext& context,
            const MeshToolTarget& target) const
    {
        (void)context;

        if (!target.targets_edges()) {
            return nullptr;
        }

        auto operation =
            std::make_unique<
            kernel::modeling::EdgeSlideOp>(
                target.edges,
                distance_);

        operation->set_exclude_target_edges_from_rails(
            options_.excludeTargetEdgesFromRails);

        return operation;
    }

    ToolResult EdgeSlideTool::commit_mesh_operation(
        ToolContext& context,
        const MeshToolTarget& target)
    {
        if (!target.targets_edges()) {
            return ToolResult::fail(
                "Cannot commit edge slide without a valid edge target.");
        }

        if (!has_effective_distance()) {
            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Edge slide completed without changes.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Cannot commit edge slide because command services are "
                "unavailable.");
        }

        const SceneNodeId nodeId =
            target.nodeId;

        const std::vector<kernel::geometry::EdgeHandle> edges =
            target.edges;

        const float committedDistance =
            distance_;

        const bool excludeTargetEdgesFromRails =
            options_.excludeTargetEdgesFromRails;

        const bool validateAfterExecute =
            options_.validateAfterExecute;

        const bool rebuildNormals =
            options_.rebuildNormals;

        const bool allowNonManifold =
            options_.allowNonManifold;

        auto command =
            std::make_unique<
            ApplyMeshOperationCommand>(
                nodeId,
                [
                    edges,
                    committedDistance,
                    excludeTargetEdgesFromRails,
                    validateAfterExecute,
                    rebuildNormals,
                    allowNonManifold
                ](
                    kernel::geometry::LEMEditor& editor)
                {
                    kernel::modeling::EdgeSlideOp operation{
                        edges,
                        committedDistance
                    };

                    operation
                        .set_exclude_target_edges_from_rails(
                            excludeTargetEdgesFromRails);

                    kernel::modeling::OperationContext
                        operationContext{};

                    operationContext.mesh =
                        &editor.mesh();

                    operationContext.validateAfterExecute =
                        validateAfterExecute;

                    operationContext.rebuildNormals =
                        rebuildNormals;

                    operationContext.allowNonManifold =
                        allowNonManifold;

                    const kernel::modeling::OperationResult
                        operationResult =
                        operation.execute(
                            operationContext);

                    return operationResult.is_success() &&
                        operationResult.changed();
                },
                "Slide Edges");

        const CommandResult commandResult =
            context.execute_command(
                std::move(command));

        if (!commandResult.success) {
            return ToolResult::fail(
                commandResult.message.empty()
                ? "Edge slide command failed."
                : commandResult.message,
                commandResult.dirtyFlags);
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            commandResult.message.empty()
            ? "Edges slid successfully."
            : commandResult.message);
    }

    void EdgeSlideTool::clear_mesh_operation()
    {
        startPosition_ =
            glm::vec2{ 0.0f };

        interactionVisualScale_ =
            1.0f;

        distance_ =
            0.0f;
    }

    ToolDescriptor EdgeSlideTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{
                std::string{
                    Id
                }
            },
            "Slide Edges",
            "Slides selected mesh edges along adjacent edge directions.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::Modal
        };
    }

    EdgeSlideToolOptions
        EdgeSlideTool::sanitize_options(
            EdgeSlideToolOptions options)
    {
        options.distancePerPixel =
            std::max(
                0.0f,
                options.distancePerPixel);

        options.distanceEpsilon =
            std::max(
                0.0f,
                options.distanceEpsilon);

        return options;
    }

    float EdgeSlideTool::calculate_distance(
        const ToolEvent& event) const
    {
        float pixelDistance =
            event.pointer.viewportPosition.x -
            startPosition_.x;

        if (options_.invertDragDirection) {
            pixelDistance =
                -pixelDistance;
        }

        return pixelDistance *
            options_.distancePerPixel *
            interactionVisualScale_;
    }

    bool EdgeSlideTool::has_effective_distance() const
    {
        return std::abs(distance_) >
            options_.distanceEpsilon;
    }

} // namespace locus::editor