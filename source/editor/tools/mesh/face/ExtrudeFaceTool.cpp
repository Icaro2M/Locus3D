/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/face/ExtrudeFaceTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/ExtrudeFaceOp.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        constexpr float minimumVisualScale =
            0.000001f;

    } // namespace

    ExtrudeFaceTool::ExtrudeFaceTool()
        : ExtrudeFaceTool(
            ExtrudeFaceToolOptions{})
    {
    }

    ExtrudeFaceTool::ExtrudeFaceTool(
        ExtrudeFaceToolOptions options)
        : MeshDragOperationTool(
            make_descriptor(),
            SelectionGranularity::Face,
            DragCompletionPolicy::ConfirmOnRelease),
        options_(std::move(options))
    {
        options_.distancePerPixel =
            std::max(
                0.0f,
                options_.distancePerPixel);

        options_.distanceEpsilon =
            std::max(
                0.0f,
                options_.distanceEpsilon);
    }

    const ExtrudeFaceToolOptions&
        ExtrudeFaceTool::options() const
    {
        return options_;
    }

    bool ExtrudeFaceTool::set_options(
        const ExtrudeFaceToolOptions& options)
    {
        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ = options;

        options_.distancePerPixel =
            std::max(
                0.0f,
                options_.distancePerPixel);

        options_.distanceEpsilon =
            std::max(
                0.0f,
                options_.distanceEpsilon);

        return true;
    }

    float ExtrudeFaceTool::distance() const
    {
        return distance_;
    }

    ToolResult ExtrudeFaceTool::begin_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;

        if (!target.targets_faces()) {
            return ToolResult::fail(
                "Face extrusion requires at least one selected face.");
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
            "Face extrusion interaction started.");
    }

    ToolResult ExtrudeFaceTool::update_mesh_operation(
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
            "Face extrusion distance updated.");
    }

    std::unique_ptr<kernel::modeling::IOperation>
        ExtrudeFaceTool::build_preview_operation(
            const ToolContext& context,
            const MeshToolTarget& target) const
    {
        (void)context;

        if (!target.targets_faces()) {
            return nullptr;
        }

        auto operation =
            std::make_unique<
            kernel::modeling::ExtrudeFaceOp>(
                target.faces,
                distance_);

        operation->set_keep_source_face(
            options_.keepSourceFace);

        return operation;
    }

    ToolResult ExtrudeFaceTool::commit_mesh_operation(
        ToolContext& context,
        const MeshToolTarget& target)
    {
        if (!target.targets_faces()) {
            return ToolResult::fail(
                "Cannot commit face extrusion without a valid face target.");
        }

        if (!has_effective_distance()) {
            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Face extrusion completed without changes.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Cannot commit face extrusion because command services are "
                "unavailable.");
        }

        const SceneNodeId nodeId =
            target.nodeId;

        const std::vector<kernel::geometry::FaceHandle> faces =
            target.faces;

        const float committedDistance =
            distance_;

        const bool keepSourceFace =
            options_.keepSourceFace;

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
                    committedDistance,
                    keepSourceFace,
                    validateAfterExecute,
                    rebuildNormals,
                    allowNonManifold
                ](
                    kernel::geometry::LEMEditor& editor)
                {
                    kernel::modeling::ExtrudeFaceOp operation{
                        faces,
                        committedDistance
                    };

                    operation.set_keep_source_face(
                        keepSourceFace);

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
                "Extrude Faces");

        const CommandResult commandResult =
            context.execute_command(
                std::move(command));

        if (!commandResult.success) {
            return ToolResult::fail(
                commandResult.message.empty()
                ? "Face extrusion command failed."
                : commandResult.message,
                commandResult.dirtyFlags);
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            commandResult.message.empty()
            ? "Faces extruded successfully."
            : commandResult.message);
    }

    void ExtrudeFaceTool::clear_mesh_operation()
    {
        startPosition_ = glm::vec2{ 0.0f };
        interactionVisualScale_ = 1.0f;
        distance_ = 0.0f;
    }

    ToolDescriptor ExtrudeFaceTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{
                std::string{
                    Id
                }
            },
            "Extrude Faces",
            "Extrudes selected mesh faces along their normals.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::Modal
        };
    }

    float ExtrudeFaceTool::calculate_distance(
        const ToolEvent& event) const
    {
        /*
         * Viewport Y normally grows downward. Subtracting the current Y from
         * the initial Y makes upward dragging produce a positive extrusion.
         */
        float pixelDistance =
            startPosition_.y -
            event.pointer.viewportPosition.y;

        if (options_.invertDragDirection) {
            pixelDistance = -pixelDistance;
        }

        return pixelDistance *
            options_.distancePerPixel *
            interactionVisualScale_;
    }

    bool ExtrudeFaceTool::has_effective_distance() const
    {
        return std::abs(distance_) >
            options_.distanceEpsilon;
    }

} // namespace locus::editor