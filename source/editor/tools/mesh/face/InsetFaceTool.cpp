/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/face/InsetFaceTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/InsetFaceOp.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace locus::editor {

    namespace {

        constexpr float minimumVisualScale =
            0.000001f;

        constexpr float planeEpsilon =
            0.000001f;

        constexpr float maximumInsetLimit =
            0.999999f;

        [[nodiscard]]
        glm::vec3 safe_normalize(
            const glm::vec3& value,
            const glm::vec3& fallback)
        {
            const float length =
                glm::length(value);

            if (length <= planeEpsilon) {
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
        bool intersect_ray_plane(
            const ToolPointerRay& ray,
            const glm::vec3& planeOrigin,
            const glm::vec3& planeNormal,
            glm::vec3& outPoint)
        {
            const glm::vec3 direction =
                ray_direction(ray);
            const glm::vec3 normal =
                safe_normalize(
                    planeNormal,
                    glm::vec3{ 0.0f, 0.0f, 1.0f });
            const float denominator =
                glm::dot(direction, normal);

            if (std::abs(denominator) <= planeEpsilon) {
                return false;
            }

            const float distance =
                glm::dot(planeOrigin - ray.origin, normal) /
                denominator;

            outPoint =
                ray.origin + direction * distance;

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
        bool face_points(
            const kernel::geometry::LEM& mesh,
            kernel::geometry::FaceHandle face,
            std::vector<glm::vec3>& outPoints,
            glm::vec3& outCenter)
        {
            const std::vector<kernel::geometry::VertexHandle> vertices =
                kernel::geometry::TopologyTraversal::face_vertices(
                    mesh,
                    face);

            if (vertices.size() < 3u) {
                return false;
            }

            outPoints.clear();
            outPoints.reserve(vertices.size());

            glm::vec3 center{ 0.0f };

            for (const kernel::geometry::VertexHandle vertex : vertices) {
                if (!mesh.is_valid(vertex)) {
                    return false;
                }

                const glm::vec3 position =
                    mesh.vertex(vertex).position;
                outPoints.push_back(position);
                center += position;
            }

            outCenter =
                center / static_cast<float>(outPoints.size());

            return true;
        }

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

        if (!initialize_plane_drag(
            context,
            event,
            target)) {
            return ToolResult::fail(
                "Face inset could not initialize a stable face-plane drag.");
        }

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

        planeOriginWorld_ =
            glm::vec3{ 0.0f };

        planeNormalWorld_ =
            glm::vec3{ 0.0f, 0.0f, 1.0f };

        startPlanePointWorld_ =
            glm::vec3{ 0.0f };

        inwardDragDirectionWorld_ =
            glm::vec3{ 1.0f, 0.0f, 0.0f };

        fallbackScreenDirection_ =
            glm::vec2{ 1.0f, 0.0f };

        interactionVisualScale_ =
            1.0f;

        worldDistanceToFactor_ =
            1.0f;

        planeDragReady_ =
            false;

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
        if (planeDragReady_) {
            glm::vec3 currentPlanePoint{ 0.0f };

            if (intersect_ray_plane(
                event.pointer.worldRay,
                planeOriginWorld_,
                planeNormalWorld_,
                currentPlanePoint)) {
                const float inwardWorldDistance =
                    glm::dot(
                        currentPlanePoint - startPlanePointWorld_,
                        inwardDragDirectionWorld_);

                return std::clamp(
                    inwardWorldDistance * worldDistanceToFactor_,
                    0.0f,
                    options_.maximumFactor);
            }
        }

        const glm::vec2 pointerDelta =
            event.pointer.viewportPosition -
            startPosition_;

        float pixelDistance =
            glm::dot(
                pointerDelta,
                fallbackScreenDirection_);

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

    bool InsetFaceTool::initialize_plane_drag(
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

        std::vector<glm::vec3> facePointsLocal;
        glm::vec3 centerLocal{ 0.0f };

        if (!face_points(
            mesh,
            face,
            facePointsLocal,
            centerLocal)) {
            return false;
        }

        const glm::mat4 nodeTransform =
            node->transform().matrix();

        planeOriginWorld_ =
            transform_point(
                nodeTransform,
                centerLocal);

        const glm::vec3 normalLocal =
            safe_normalize(
                kernel::geometry::NormalBuilder::face_normal(
                    mesh,
                    face),
                glm::vec3{ 0.0f, 0.0f, 1.0f });

        planeNormalWorld_ =
            safe_normalize(
                transform_vector(
                    nodeTransform,
                    normalLocal),
                glm::vec3{ 0.0f, 0.0f, 1.0f });

        float maximumWorldRadius =
            0.0f;

        for (const glm::vec3& pointLocal : facePointsLocal) {
            const glm::vec3 pointWorld =
                transform_point(
                    nodeTransform,
                    pointLocal);

            maximumWorldRadius =
                std::max(
                    maximumWorldRadius,
                    glm::length(
                        pointWorld - planeOriginWorld_));
        }

        if (maximumWorldRadius <= planeEpsilon) {
            return false;
        }

        worldDistanceToFactor_ =
            1.0f / maximumWorldRadius;

        planeDragReady_ =
            intersect_ray_plane(
                event.pointer.worldRay,
                planeOriginWorld_,
                planeNormalWorld_,
                startPlanePointWorld_);

        if (!planeDragReady_) {
            startPlanePointWorld_ =
                planeOriginWorld_;
        }

        glm::vec3 outwardDirection =
            startPlanePointWorld_ -
            planeOriginWorld_;

        if (glm::length(outwardDirection) <= planeEpsilon) {
            outwardDirection =
                transform_point(
                    nodeTransform,
                    facePointsLocal.front()) -
                planeOriginWorld_;
        }

        outwardDirection =
            safe_normalize(
                outwardDirection,
                glm::vec3{ 1.0f, 0.0f, 0.0f });

        inwardDragDirectionWorld_ =
            -outwardDirection;

        fallbackScreenDirection_ =
            glm::vec2{
                glm::dot(
                    inwardDragDirectionWorld_,
                    event.pointer.viewRight),
                -glm::dot(
                    inwardDragDirectionWorld_,
                    event.pointer.viewUp)
            };

        const float screenDirectionLength =
            glm::length(fallbackScreenDirection_);

        if (screenDirectionLength > planeEpsilon) {
            fallbackScreenDirection_ /=
                screenDirectionLength;
        }
        else {
            fallbackScreenDirection_ =
                glm::vec2{ 0.0f, -1.0f };
        }

        return true;
    }

    bool InsetFaceTool::has_effective_factor() const
    {
        return factor_ >
            options_.factorEpsilon &&
            factor_ <
            1.0f - options_.factorEpsilon;
    }

} // namespace locus::editor
