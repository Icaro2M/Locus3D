/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/topology/LoopCutTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/topology/LoopCutOp.h"

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

        constexpr float kernelMinimumFactor =
            0.0001f;

        constexpr float kernelMaximumFactor =
            0.9999f;

    } // namespace

    LoopCutTool::LoopCutTool()
        : LoopCutTool(
            LoopCutToolOptions{})
    {
    }

    LoopCutTool::LoopCutTool(
        LoopCutToolOptions options)
        : MeshDragOperationTool(
            make_descriptor(),
            SelectionGranularity::Edge,
            DragCompletionPolicy::ConfirmOnRelease),
        options_(
            sanitize_options(
                std::move(options))),
        factor_(
            options_.initialFactor)
    {
    }

    const LoopCutToolOptions&
        LoopCutTool::options() const
    {
        return options_;
    }

    bool LoopCutTool::set_options(
        const LoopCutToolOptions& options)
    {
        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ =
            sanitize_options(options);

        factor_ =
            options_.initialFactor;

        return true;
    }

    float LoopCutTool::factor() const
    {
        return factor_;
    }

    std::size_t LoopCutTool::cuts() const
    {
        return options_.cuts;
    }

    ToolResult LoopCutTool::begin_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;

        if (!target.targets_edges()) {
            return ToolResult::fail(
                "Loop cut requires at least one selected edge.");
        }

        startPosition_ =
            event.pointer.viewportPosition;

        interactionVisualScale_ =
            event.pointer.visualScale > minimumVisualScale
            ? event.pointer.visualScale
            : 1.0f;

        factor_ =
            options_.initialFactor;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            uses_interactive_factor()
            ? "Loop cut interaction started."
            : "Evenly spaced loop cut interaction started.");
    }

    ToolResult LoopCutTool::update_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;
        (void)target;

        /*
         * The current LoopCutOp always spaces multiple cuts uniformly. Pointer
         * movement therefore only controls the position of one non-even cut.
         */
        if (!uses_interactive_factor()) {
            return ToolResult::ignored();
        }

        const float previousFactor =
            factor_;

        factor_ =
            calculate_factor(event);

        if (std::abs(
            factor_ - previousFactor) <=
            options_.factorEpsilon) {
            return ToolResult::ignored();
        }

        return ToolResult::updated(
            EditorDirtyFlags::None,
            "Loop cut factor updated.");
    }

    std::unique_ptr<kernel::modeling::IOperation>
        LoopCutTool::build_preview_operation(
            const ToolContext& context,
            const MeshToolTarget& target) const
    {
        (void)context;

        if (!target.targets_edges()) {
            return nullptr;
        }

        auto operation =
            std::make_unique<
            kernel::modeling::LoopCutOp>(
                target.edges);

        operation->set_cuts(
            options_.cuts);

        operation->set_factor(
            factor_);

        operation->set_even_spacing(
            options_.evenSpacing);

        return operation;
    }

    ToolResult LoopCutTool::commit_mesh_operation(
        ToolContext& context,
        const MeshToolTarget& target)
    {
        if (!target.targets_edges()) {
            return ToolResult::fail(
                "Cannot commit loop cut without a valid edge target.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Cannot commit loop cut because command services are "
                "unavailable.");
        }

        const SceneNodeId nodeId =
            target.nodeId;

        const std::vector<kernel::geometry::EdgeHandle> edges =
            target.edges;

        const std::size_t committedCuts =
            options_.cuts;

        const float committedFactor =
            factor_;

        const bool evenSpacing =
            options_.evenSpacing;

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
                    committedCuts,
                    committedFactor,
                    evenSpacing,
                    validateAfterExecute,
                    rebuildNormals,
                    allowNonManifold
                ](
                    kernel::geometry::LEMEditor& editor)
                {
                    kernel::modeling::LoopCutOp operation{
                        edges
                    };

                    operation.set_cuts(
                        committedCuts);

                    operation.set_factor(
                        committedFactor);

                    operation.set_even_spacing(
                        evenSpacing);

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
                "Loop Cut");

        const CommandResult commandResult =
            context.execute_command(
                std::move(command));

        if (!commandResult.success) {
            return ToolResult::fail(
                commandResult.message.empty()
                ? "Loop cut command failed."
                : commandResult.message,
                commandResult.dirtyFlags);
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            commandResult.message.empty()
            ? "Loop cut completed successfully."
            : commandResult.message);
    }

    void LoopCutTool::clear_mesh_operation()
    {
        startPosition_ =
            glm::vec2{ 0.0f };

        interactionVisualScale_ =
            1.0f;

        factor_ =
            options_.initialFactor;
    }

    ToolDescriptor LoopCutTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{
                std::string{
                    Id
                }
            },
            "Loop Cut",
            "Cuts selected edges and connects matching cuts across faces.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::Modal
        };
    }

    LoopCutToolOptions
        LoopCutTool::sanitize_options(
            LoopCutToolOptions options)
    {
        options.factorPerPixel =
            std::max(
                0.0f,
                options.factorPerPixel);

        options.factorEpsilon =
            std::max(
                0.0f,
                options.factorEpsilon);

        options.minimumFactor =
            std::clamp(
                options.minimumFactor,
                kernelMinimumFactor,
                kernelMaximumFactor);

        options.maximumFactor =
            std::clamp(
                options.maximumFactor,
                options.minimumFactor,
                kernelMaximumFactor);

        options.initialFactor =
            std::clamp(
                options.initialFactor,
                options.minimumFactor,
                options.maximumFactor);

        options.cuts =
            std::max<std::size_t>(
                1,
                options.cuts);

        return options;
    }

    float LoopCutTool::calculate_factor(
        const ToolEvent& event) const
    {
        float pixelDistance =
            event.pointer.viewportPosition.x -
            startPosition_.x;

        if (options_.invertDragDirection) {
            pixelDistance =
                -pixelDistance;
        }

        const float rawFactor =
            options_.initialFactor +
            pixelDistance *
            options_.factorPerPixel *
            interactionVisualScale_;

        return std::clamp(
            rawFactor,
            options_.minimumFactor,
            options_.maximumFactor);
    }

    bool LoopCutTool::uses_interactive_factor() const
    {
        return options_.cuts == 1 &&
            !options_.evenSpacing;
    }

} // namespace locus::editor