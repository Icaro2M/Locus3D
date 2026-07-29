/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/face/ExtrudeFaceTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/ExtrudeFaceOp.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace locus::editor {

    namespace {

        constexpr float minimumVisualScale =
            0.000001f;

        constexpr float axisEpsilon =
            0.000001f;

        [[nodiscard]]
        glm::vec3 safe_normalize(
            const glm::vec3& value,
            const glm::vec3& fallback)
        {
            const float length =
                glm::length(value);

            if (length <= axisEpsilon) {
                return fallback;
            }

            return value / length;
        }

        [[nodiscard]]
        glm::vec3 ray_direction(
            const ToolPointerRay& ray)
        {
            return safe_normalize(
                ray.direction,
                glm::vec3{ 0.0f, 0.0f, -1.0f });
        }

        [[nodiscard]]
        bool closest_point_on_axis_from_ray(
            const ToolPointerRay& ray,
            const glm::vec3& axisOrigin,
            const glm::vec3& axisDirection,
            glm::vec3& outPoint)
        {
            const glm::vec3 rayDirection =
                ray_direction(ray);
            const glm::vec3 axis =
                safe_normalize(
                    axisDirection,
                    glm::vec3{ 0.0f, 1.0f, 0.0f });

            const glm::vec3 w =
                ray.origin - axisOrigin;
            const float a =
                glm::dot(rayDirection, rayDirection);
            const float b =
                glm::dot(rayDirection, axis);
            const float c =
                glm::dot(axis, axis);
            const float d =
                glm::dot(rayDirection, w);
            const float e =
                glm::dot(axis, w);
            const float denominator =
                (a * c) - (b * b);

            if (std::abs(denominator) <= axisEpsilon) {
                return false;
            }

            const float axisT =
                ((a * e) - (b * d)) / denominator;

            outPoint =
                axisOrigin + axis * axisT;

            return true;
        }

        [[nodiscard]]
        glm::vec3 transform_point(
            const glm::mat4& transform,
            const glm::vec3& point)
        {
            return glm::vec3{
                transform *
                glm::vec4{ point, 1.0f }
            };
        }

        [[nodiscard]]
        glm::vec3 transform_vector(
            const glm::mat4& transform,
            const glm::vec3& vector)
        {
            return glm::vec3{
                transform *
                glm::vec4{ vector, 0.0f }
            };
        }

        [[nodiscard]]
        bool face_center(
            const kernel::geometry::LEM& mesh,
            kernel::geometry::FaceHandle face,
            glm::vec3& outCenter)
        {
            const std::vector<kernel::geometry::VertexHandle> vertices =
                kernel::geometry::TopologyTraversal::face_vertices(
                    mesh,
                    face);

            if (vertices.empty()) {
                return false;
            }

            glm::vec3 center{ 0.0f };

            for (const kernel::geometry::VertexHandle vertex : vertices) {
                if (!mesh.is_valid(vertex)) {
                    return false;
                }

                center +=
                    mesh.vertex(vertex).position;
            }

            outCenter =
                center / static_cast<float>(vertices.size());

            return true;
        }

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

        if (!initialize_axis_drag(
            context,
            event,
            target)) {
            return ToolResult::fail(
                "Face extrusion could not initialize a stable drag axis.");
        }

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
        extrusionAxisWorld_ = glm::vec3{ 0.0f, 1.0f, 0.0f };
        axisOriginWorld_ = glm::vec3{ 0.0f };
        startAxisPointWorld_ = glm::vec3{ 0.0f };
        fallbackScreenAxis_ = glm::vec2{ 0.0f, -1.0f };
        interactionVisualScale_ = 1.0f;
        worldDistanceToLocalDistance_ = 1.0f;
        axisDragReady_ = false;
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
        if (axisDragReady_) {
            glm::vec3 currentAxisPoint{ 0.0f };

            if (closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                extrusionAxisWorld_,
                currentAxisPoint)) {
                const float worldDistance =
                    glm::dot(
                        currentAxisPoint - startAxisPointWorld_,
                        extrusionAxisWorld_);

                return worldDistance *
                    worldDistanceToLocalDistance_;
            }
        }

        const glm::vec2 pointerDelta =
            event.pointer.viewportPosition -
            startPosition_;

        float pixelDistance =
            glm::dot(
                pointerDelta,
                fallbackScreenAxis_);

        if (options_.invertDragDirection) {
            pixelDistance = -pixelDistance;
        }

        return pixelDistance *
            options_.distancePerPixel *
            interactionVisualScale_;
    }

    bool ExtrudeFaceTool::initialize_axis_drag(
        const ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        const MeshNode* node =
            context.scene().find_mesh(
                target.nodeId);

        if (node == nullptr || target.faces.empty()) {
            return false;
        }

        const kernel::geometry::LEM& mesh =
            node->mesh();
        const kernel::geometry::FaceHandle face =
            target.faces.front();

        if (!mesh.is_valid(face)) {
            return false;
        }

        glm::vec3 centerLocal{ 0.0f };
        if (!face_center(
            mesh,
            face,
            centerLocal)) {
            return false;
        }

        const glm::vec3 normalLocal =
            safe_normalize(
                kernel::geometry::NormalBuilder::face_normal(
                    mesh,
                    face),
                glm::vec3{ 0.0f, 1.0f, 0.0f });

        const glm::mat4 nodeTransform =
            node->transform().matrix();

        const glm::vec3 worldAxisVector =
            transform_vector(
                nodeTransform,
                normalLocal);
        const float worldAxisScale =
            glm::length(worldAxisVector);

        if (worldAxisScale <= axisEpsilon) {
            return false;
        }

        extrusionAxisWorld_ =
            worldAxisVector / worldAxisScale;
        axisOriginWorld_ =
            transform_point(
                nodeTransform,
                centerLocal);
        worldDistanceToLocalDistance_ =
            1.0f / worldAxisScale;

        fallbackScreenAxis_ =
            glm::vec2{
                glm::dot(
                    extrusionAxisWorld_,
                    event.pointer.viewRight),
                -glm::dot(
                    extrusionAxisWorld_,
                    event.pointer.viewUp)
            };

        const float screenAxisLength =
            glm::length(fallbackScreenAxis_);

        if (screenAxisLength > axisEpsilon) {
            fallbackScreenAxis_ /=
                screenAxisLength;
        }
        else {
            fallbackScreenAxis_ =
                glm::vec2{ 0.0f, -1.0f };
        }

        axisDragReady_ =
            closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                extrusionAxisWorld_,
                startAxisPointWorld_);

        if (!axisDragReady_) {
            startAxisPointWorld_ =
                axisOriginWorld_;
        }

        return true;
    }

    bool ExtrudeFaceTool::has_effective_distance() const
    {
        return std::abs(distance_) >
            options_.distanceEpsilon;
    }

} // namespace locus::editor
