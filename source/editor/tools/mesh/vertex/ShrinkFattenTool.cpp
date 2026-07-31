/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/vertex/ShrinkFattenTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/transform/ShrinkFattenOp.h"

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
        glm::vec3 vertex_normal(
            const kernel::geometry::LEM& mesh,
            kernel::geometry::VertexHandle vertex)
        {
            if (!mesh.is_valid(vertex)) {
                return glm::vec3{ 0.0f };
            }

            glm::vec3 normal{ 0.0f };

            for (const kernel::geometry::FaceHandle face :
                kernel::geometry::TopologyTraversal::vertex_faces(
                    mesh,
                    vertex)) {
                if (mesh.is_valid(face)) {
                    normal +=
                        kernel::geometry::NormalBuilder::face_normal(
                            mesh,
                            face);
                }
            }

            return safe_normalize(
                normal,
                glm::vec3{ 0.0f });
        }

    } // namespace

    ShrinkFattenTool::ShrinkFattenTool()
        : ShrinkFattenTool(
            ShrinkFattenToolOptions{})
    {
    }

    ShrinkFattenTool::ShrinkFattenTool(
        ShrinkFattenToolOptions options)
        : MeshDragOperationTool(
            make_descriptor(),
            SelectionGranularity::Vertex,
            DragCompletionPolicy::ConfirmOnRelease),
        options_(
            sanitize_options(
                std::move(options)))
    {
    }

    const ShrinkFattenToolOptions&
        ShrinkFattenTool::options() const
    {
        return options_;
    }

    bool ShrinkFattenTool::set_options(
        const ShrinkFattenToolOptions& options)
    {
        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ =
            sanitize_options(options);

        return true;
    }

    float ShrinkFattenTool::distance() const
    {
        return distance_;
    }

    ToolResult ShrinkFattenTool::begin_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        if (!target.targets_vertices()) {
            return ToolResult::fail(
                "Shrink/fatten requires at least one selected vertex.");
        }

        startPosition_ =
            event.pointer.viewportPosition;

        interactionVisualScale_ =
            event.pointer.visualScale > minimumVisualScale
            ? event.pointer.visualScale
            : 1.0f;

        distance_ =
            0.0f;

        if (!initialize_normal_drag(
            context,
            event,
            target)) {
            return ToolResult::fail(
                "Shrink/fatten could not initialize a stable normal drag "
                "axis.");
        }

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Shrink/fatten interaction started.");
    }

    ToolResult ShrinkFattenTool::update_mesh_operation(
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
            "Shrink/fatten distance updated.");
    }

    std::unique_ptr<kernel::modeling::IOperation>
        ShrinkFattenTool::build_preview_operation(
            const ToolContext& context,
            const MeshToolTarget& target) const
    {
        (void)context;

        if (!target.targets_vertices()) {
            return nullptr;
        }

        return std::make_unique<
            kernel::modeling::ShrinkFattenOp>(
                target.vertices,
                distance_);
    }

    ToolResult ShrinkFattenTool::commit_mesh_operation(
        ToolContext& context,
        const MeshToolTarget& target)
    {
        if (!target.targets_vertices()) {
            return ToolResult::fail(
                "Cannot commit shrink/fatten without a valid vertex "
                "target.");
        }

        if (!has_effective_distance()) {
            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Shrink/fatten completed without changes.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Cannot commit shrink/fatten because command services are "
                "unavailable.");
        }

        const SceneNodeId nodeId =
            target.nodeId;

        const std::vector<kernel::geometry::VertexHandle> vertices =
            target.vertices;

        const float committedDistance =
            distance_;

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
                    vertices,
                    committedDistance,
                    validateAfterExecute,
                    rebuildNormals,
                    allowNonManifold
                ](
                    kernel::geometry::LEMEditor& editor)
                {
                    kernel::modeling::ShrinkFattenOp operation{
                        vertices,
                        committedDistance
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
                "Shrink/Fatten Vertices");

        const CommandResult commandResult =
            context.execute_command(
                std::move(command));

        if (!commandResult.success) {
            return ToolResult::fail(
                commandResult.message.empty()
                ? "Shrink/fatten command failed."
                : commandResult.message,
                commandResult.dirtyFlags);
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            commandResult.message.empty()
            ? "Vertices shrink/fattened successfully."
            : commandResult.message);
    }

    void ShrinkFattenTool::clear_mesh_operation()
    {
        startPosition_ =
            glm::vec2{ 0.0f };

        normalAxisWorld_ =
            glm::vec3{ 0.0f, 1.0f, 0.0f };

        axisOriginWorld_ =
            glm::vec3{ 0.0f };

        startAxisPointWorld_ =
            glm::vec3{ 0.0f };

        fallbackScreenAxis_ =
            glm::vec2{ 0.0f, -1.0f };

        interactionVisualScale_ =
            1.0f;

        worldDistanceToLocalDistance_ =
            1.0f;

        axisDragReady_ =
            false;

        distance_ =
            0.0f;
    }

    ToolDescriptor ShrinkFattenTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{
                std::string{
                    Id
                }
            },
            "Shrink/Fatten",
            "Moves selected mesh vertices along their averaged normals.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::Modal
        };
    }

    ShrinkFattenToolOptions
        ShrinkFattenTool::sanitize_options(
            ShrinkFattenToolOptions options)
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

    float ShrinkFattenTool::calculate_distance(
        const ToolEvent& event) const
    {
        if (axisDragReady_) {
            glm::vec3 currentAxisPoint{ 0.0f };

            if (closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                normalAxisWorld_,
                currentAxisPoint)) {
                float worldDistance =
                    glm::dot(
                        currentAxisPoint - startAxisPointWorld_,
                        normalAxisWorld_);

                if (options_.invertDragDirection) {
                    worldDistance =
                        -worldDistance;
                }

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
            pixelDistance =
                -pixelDistance;
        }

        return pixelDistance *
            options_.distancePerPixel *
            interactionVisualScale_;
    }

    bool ShrinkFattenTool::initialize_normal_drag(
        const ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        const MeshNode* node =
            context.scene().find_mesh(
                target.nodeId);

        if (node == nullptr || target.vertices.empty()) {
            return false;
        }

        const kernel::geometry::LEM& mesh =
            node->mesh();

        glm::vec3 originLocal{ 0.0f };
        glm::vec3 normalLocal{ 0.0f };

        for (const kernel::geometry::VertexHandle vertex :
            target.vertices) {
            if (!mesh.is_valid(vertex)) {
                return false;
            }

            originLocal +=
                mesh.vertex(vertex).position;

            normalLocal +=
                vertex_normal(
                    mesh,
                    vertex);
        }

        originLocal /=
            static_cast<float>(target.vertices.size());

        normalLocal =
            safe_normalize(
                normalLocal,
                glm::vec3{ 0.0f });

        if (glm::length(normalLocal) <= axisEpsilon) {
            return false;
        }

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

        normalAxisWorld_ =
            worldAxisVector / worldAxisScale;
        axisOriginWorld_ =
            transform_point(
                nodeTransform,
                originLocal);
        worldDistanceToLocalDistance_ =
            1.0f / worldAxisScale;

        fallbackScreenAxis_ =
            glm::vec2{
                glm::dot(
                    normalAxisWorld_,
                    event.pointer.viewRight),
                -glm::dot(
                    normalAxisWorld_,
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
                normalAxisWorld_,
                startAxisPointWorld_);

        if (!axisDragReady_) {
            startAxisPointWorld_ =
                axisOriginWorld_;
        }

        return true;
    }

    bool ShrinkFattenTool::has_effective_distance() const
    {
        return std::abs(distance_) >
            options_.distanceEpsilon;
    }

} // namespace locus::editor
