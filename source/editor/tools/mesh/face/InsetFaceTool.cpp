/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/face/InsetFaceTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/InsetFaceOp.h"

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

        constexpr float maximumInsetLimit =
            0.999999f;

    } // namespace

    InsetFaceTool::InsetFaceTool()
        : InsetFaceTool(
            InsetFaceToolOptions{})
    {
    }

    InsetFaceTool::InsetFaceTool(
        InsetFaceToolOptions options)
        : MeshDragOperationTool(
            make_descriptor(),
            SelectionGranularity::Face,
            DragCompletionPolicy::ConfirmOnRelease),
        options_(
            sanitize_options(
                std::move(options)))
    {
    }

    const InsetFaceToolOptions&
        InsetFaceTool::options() const
    {
        return options_;
    }

    bool InsetFaceTool::set_options(
        const InsetFaceToolOptions& options)
    {
        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ =
            sanitize_options(options);

        return true;
    }

    float InsetFaceTool::factor() const
    {
        return factor_;
    }

    ToolResult InsetFaceTool::begin_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;

        if (!target.targets_faces()) {
            return ToolResult::fail(
                "Face inset requires at least one selected face.");
        }

        startPosition_ =
            event.pointer.viewportPosition;

        interactionVisualScale_ =
            event.pointer.visualScale > minimumVisualScale
            ? event.pointer.visualScale
            : 1.0f;

        factor_ = 0.0f;

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Face inset interaction started.");
    }

    ToolResult InsetFaceTool::update_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;
        (void)target;

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
            "Face inset factor updated.");
    }

    std::unique_ptr<kernel::modeling::IOperation>
        InsetFaceTool::build_preview_operation(
            const ToolContext& context,
            const MeshToolTarget& target) const
    {
        (void)context;

        if (!target.targets_faces()) {
            return nullptr;
        }

        return std::make_unique<
            kernel::modeling::InsetFaceOp>(
                target.faces,
                factor_);
    }

    ToolResult InsetFaceTool::commit_mesh_operation(
        ToolContext& context,
        const MeshToolTarget& target)
    {
        if (!target.targets_faces()) {
            return ToolResult::fail(
                "Cannot commit face inset without a valid face target.");
        }

        if (!has_effective_factor()) {
            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Face inset completed without changes.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Cannot commit face inset because command services are "
                "unavailable.");
        }

        const SceneNodeId nodeId =
            target.nodeId;

        const std::vector<kernel::geometry::FaceHandle> faces =
            target.faces;

        const float committedFactor =
            factor_;

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
                    faces,
                    committedFactor,
                    validateAfterExecute,
                    rebuildNormals,
                    allowNonManifold
                ](
                    kernel::geometry::LEMEditor& editor)
                {
                    kernel::modeling::InsetFaceOp operation{
                        faces,
                        committedFactor
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
                "Inset Faces");

        const CommandResult commandResult =
            context.execute_command(
                std::move(command));

        if (!commandResult.success) {
            return ToolResult::fail(
                commandResult.message.empty()
                ? "Face inset command failed."
                : commandResult.message,
                commandResult.dirtyFlags);
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            commandResult.message.empty()
            ? "Faces inset successfully."
            : commandResult.message);
    }

    void InsetFaceTool::clear_mesh_operation()
    {
        startPosition_ =
            glm::vec2{ 0.0f };

        interactionVisualScale_ =
            1.0f;

        factor_ =
            0.0f;
    }

    ToolDescriptor InsetFaceTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{
                std::string{
                    Id
                }
            },
            "Inset Faces",
            "Insets selected mesh faces toward their centers.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::Modal
        };
    }

    InsetFaceToolOptions
        InsetFaceTool::sanitize_options(
            InsetFaceToolOptions options)
    {
        options.factorPerPixel =
            std::max(
                0.0f,
                options.factorPerPixel);

        options.factorEpsilon =
            std::clamp(
                options.factorEpsilon,
                0.0f,
                maximumInsetLimit);

        options.maximumFactor =
            std::clamp(
                options.maximumFactor,
                options.factorEpsilon,
                maximumInsetLimit);

        return options;
    }

    float InsetFaceTool::calculate_factor(
        const ToolEvent& event) const
    {
        /*
         * Viewport Y normally increases downward. Subtracting current Y from
         * initial Y makes upward dragging increase the inset factor.
         */
        float pixelDistance =
            startPosition_.y -
            event.pointer.viewportPosition.y;

        if (options_.invertDragDirection) {
            pixelDistance =
                -pixelDistance;
        }

        const float rawFactor =
            pixelDistance *
            options_.factorPerPixel *
            interactionVisualScale_;

        return std::clamp(
            rawFactor,
            0.0f,
            options_.maximumFactor);
    }

    bool InsetFaceTool::has_effective_factor() const
    {
        return factor_ >
            options_.factorEpsilon &&
            factor_ <
            1.0f - options_.factorEpsilon;
    }

} // namespace locus::editor