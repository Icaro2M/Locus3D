/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/edge/BevelTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/edge/BevelOp.h"

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

    BevelTool::BevelTool()
        : BevelTool(
            BevelToolOptions{})
    {
    }

    BevelTool::BevelTool(
        BevelToolOptions options)
        : MeshDragOperationTool(
            make_descriptor(),
            SelectionGranularity::Edge,
            DragCompletionPolicy::ConfirmOnRelease),
        options_(
            sanitize_options(
                std::move(options)))
    {
    }

    const BevelToolOptions&
        BevelTool::options() const
    {
        return options_;
    }

    bool BevelTool::set_options(
        const BevelToolOptions& options)
    {
        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ =
            sanitize_options(options);

        return true;
    }

    float BevelTool::width() const
    {
        return width_;
    }

    ToolResult BevelTool::begin_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;

        if (!target.targets_edges()) {
            return ToolResult::fail(
                "Edge bevel requires at least one selected edge.");
        }

        startPosition_ =
            event.pointer.viewportPosition;

        interactionVisualScale_ =
            event.pointer.visualScale > minimumVisualScale
            ? event.pointer.visualScale
            : 1.0f;

        width_ = 0.0f;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Edge bevel interaction started.");
    }

    ToolResult BevelTool::update_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;
        (void)target;

        const float previousWidth =
            width_;

        width_ =
            calculate_width(event);

        if (std::abs(
            width_ - previousWidth) <=
            options_.widthEpsilon) {
            return ToolResult::ignored();
        }

        return ToolResult::updated(
            EditorDirtyFlags::None,
            "Edge bevel width updated.");
    }

    std::unique_ptr<kernel::modeling::IOperation>
        BevelTool::build_preview_operation(
            const ToolContext& context,
            const MeshToolTarget& target) const
    {
        (void)context;

        if (!target.targets_edges()) {
            return nullptr;
        }

        return std::make_unique<
            kernel::modeling::BevelOp>(
                target.edges,
                width_);
    }

    ToolResult BevelTool::commit_mesh_operation(
        ToolContext& context,
        const MeshToolTarget& target)
    {
        if (!target.targets_edges()) {
            return ToolResult::fail(
                "Cannot commit edge bevel without a valid edge target.");
        }

        if (!has_effective_width()) {
            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Edge bevel completed without changes.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Cannot commit edge bevel because command services are "
                "unavailable.");
        }

        const SceneNodeId nodeId =
            target.nodeId;

        const std::vector<kernel::geometry::EdgeHandle> edges =
            target.edges;

        const float committedWidth =
            width_;

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
                    committedWidth,
                    validateAfterExecute,
                    rebuildNormals,
                    allowNonManifold
                ](
                    kernel::geometry::LEMEditor& editor)
                {
                    kernel::modeling::BevelOp operation{
                        edges,
                        committedWidth
                    };

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
                "Bevel Edges");

        const CommandResult commandResult =
            context.execute_command(
                std::move(command));

        if (!commandResult.success) {
            return ToolResult::fail(
                commandResult.message.empty()
                ? "Edge bevel command failed."
                : commandResult.message,
                commandResult.dirtyFlags);
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            commandResult.message.empty()
            ? "Edges beveled successfully."
            : commandResult.message);
    }

    void BevelTool::clear_mesh_operation()
    {
        startPosition_ =
            glm::vec2{ 0.0f };

        interactionVisualScale_ =
            1.0f;

        width_ =
            0.0f;
    }

    ToolDescriptor BevelTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{
                std::string{
                    Id
                }
            },
            "Bevel Edges",
            "Creates single-segment chamfers on selected mesh edges.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::Modal
        };
    }

    BevelToolOptions BevelTool::sanitize_options(
        BevelToolOptions options)
    {
        options.widthPerPixel =
            std::max(
                0.0f,
                options.widthPerPixel);

        options.widthEpsilon =
            std::max(
                0.0f,
                options.widthEpsilon);

        options.maximumWidth =
            std::max(
                0.0f,
                options.maximumWidth);

        return options;
    }

    float BevelTool::calculate_width(
        const ToolEvent& event) const
    {
        float pixelDistance =
            event.pointer.viewportPosition.x -
            startPosition_.x;

        if (options_.invertDragDirection) {
            pixelDistance =
                -pixelDistance;
        }

        float width =
            pixelDistance *
            options_.widthPerPixel *
            interactionVisualScale_;

        /*
         * BevelOp accepts only a positive width. Pointer movement opposite to
         * the configured direction therefore returns to the neutral value.
         */
        width =
            std::max(
                0.0f,
                width);

        if (options_.maximumWidth > 0.0f) {
            width =
                std::min(
                    width,
                    options_.maximumWidth);
        }

        return width;
    }

    bool BevelTool::has_effective_width() const
    {
        return width_ >
            options_.widthEpsilon;
    }

} // namespace locus::editor