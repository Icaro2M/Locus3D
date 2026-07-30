/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/edge/EdgeSlideTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/edge/EdgeSlideOp.h"

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
            const glm::vec3 direction =
                ray_direction(ray);
            const glm::vec3 axis =
                safe_normalize(
                    axisDirection,
                    glm::vec3{ 1.0f, 0.0f, 0.0f });

            const glm::vec3 w =
                ray.origin - axisOrigin;
            const float a =
                glm::dot(direction, direction);
            const float b =
                glm::dot(direction, axis);
            const float c =
                glm::dot(axis, axis);
            const float d =
                glm::dot(direction, w);
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

        template<typename HandleT>
        [[nodiscard]]
        bool contains_handle(
            const std::vector<HandleT>& handles,
            HandleT handle)
        {
            return std::find(
                handles.begin(),
                handles.end(),
                handle) != handles.end();
        }

        [[nodiscard]]
        std::vector<kernel::geometry::VertexHandle> edge_vertices(
            const kernel::geometry::LEM& mesh,
            const std::vector<kernel::geometry::EdgeHandle>& edges)
        {
            std::vector<kernel::geometry::VertexHandle> vertices;

            for (const kernel::geometry::EdgeHandle edge : edges) {
                if (!mesh.is_valid(edge)) {
                    continue;
                }

                const kernel::geometry::Edge& edgeElement =
                    mesh.edge(edge);

                if (mesh.is_valid(edgeElement.vertexA) &&
                    !contains_handle(vertices, edgeElement.vertexA)) {
                    vertices.push_back(edgeElement.vertexA);
                }

                if (mesh.is_valid(edgeElement.vertexB) &&
                    !contains_handle(vertices, edgeElement.vertexB)) {
                    vertices.push_back(edgeElement.vertexB);
                }
            }

            return vertices;
        }

        [[nodiscard]]
        glm::vec3 slide_direction_for_vertex(
            const kernel::geometry::LEM& mesh,
            kernel::geometry::VertexHandle vertex,
            const std::vector<kernel::geometry::EdgeHandle>& targetEdges,
            bool excludeTargetEdges)
        {
            if (!mesh.is_valid(vertex)) {
                return glm::vec3{ 0.0f };
            }

            const glm::vec3 origin =
                mesh.vertex(vertex).position;
            const std::vector<kernel::geometry::EdgeHandle> incidentEdges =
                kernel::geometry::TopologyTraversal::vertex_edges(
                    mesh,
                    vertex);

            glm::vec3 accumulated{ 0.0f };
            std::vector<glm::vec3> railDirections;

            for (const kernel::geometry::EdgeHandle edge : incidentEdges) {
                if (!mesh.is_valid(edge)) {
                    continue;
                }

                if (excludeTargetEdges &&
                    contains_handle(targetEdges, edge)) {
                    continue;
                }

                const kernel::geometry::Edge& edgeElement =
                    mesh.edge(edge);

                kernel::geometry::VertexHandle otherVertex{};

                if (edgeElement.vertexA == vertex) {
                    otherVertex =
                        edgeElement.vertexB;
                }
                else if (edgeElement.vertexB == vertex) {
                    otherVertex =
                        edgeElement.vertexA;
                }
                else {
                    continue;
                }

                if (!mesh.is_valid(otherVertex)) {
                    continue;
                }

                const glm::vec3 delta =
                    mesh.vertex(otherVertex).position -
                    origin;
                const float length =
                    glm::length(delta);

                if (length <= axisEpsilon) {
                    continue;
                }

                const glm::vec3 direction =
                    delta / length;
                railDirections.push_back(direction);
                accumulated += direction;
            }

            const float accumulatedLength =
                glm::length(accumulated);

            if (accumulatedLength > axisEpsilon) {
                return accumulated / accumulatedLength;
            }

            if (!railDirections.empty()) {
                return railDirections.front();
            }

            return glm::vec3{ 0.0f };
        }

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

        if (!initialize_slide_drag(
            context,
            event,
            target)) {
            return ToolResult::fail(
                "Edge slide could not initialize a stable slide rail.");
        }

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

        slideAxisWorld_ =
            glm::vec3{ 1.0f, 0.0f, 0.0f };

        axisOriginWorld_ =
            glm::vec3{ 0.0f };

        startAxisPointWorld_ =
            glm::vec3{ 0.0f };

        fallbackScreenAxis_ =
            glm::vec2{ 1.0f, 0.0f };

        interactionVisualScale_ =
            1.0f;

        worldDistanceToLocalDistance_ =
            1.0f;

        axisDragReady_ =
            false;

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
        if (axisDragReady_) {
            glm::vec3 currentAxisPoint{ 0.0f };

            if (closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                slideAxisWorld_,
                currentAxisPoint)) {
                float worldDistance =
                    glm::dot(
                        currentAxisPoint - startAxisPointWorld_,
                        slideAxisWorld_);

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

    bool EdgeSlideTool::initialize_slide_drag(
        const ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        const MeshNode* node =
            context.scene().find_mesh(
                target.nodeId);

        if (node == nullptr || target.edges.empty()) {
            return false;
        }

        const kernel::geometry::LEM& mesh =
            node->mesh();
        const std::vector<kernel::geometry::VertexHandle> vertices =
            edge_vertices(
                mesh,
                target.edges);

        if (vertices.empty()) {
            return false;
        }

        glm::vec3 originLocal{ 0.0f };
        glm::vec3 axisLocal{ 0.0f };

        for (const kernel::geometry::VertexHandle vertex : vertices) {
            if (!mesh.is_valid(vertex)) {
                return false;
            }

            originLocal +=
                mesh.vertex(vertex).position;

            axisLocal +=
                slide_direction_for_vertex(
                    mesh,
                    vertex,
                    target.edges,
                    options_.excludeTargetEdgesFromRails);
        }

        originLocal /=
            static_cast<float>(vertices.size());

        axisLocal =
            safe_normalize(
                axisLocal,
                glm::vec3{ 0.0f });

        if (glm::length(axisLocal) <= axisEpsilon) {
            return false;
        }

        const glm::mat4 nodeTransform =
            node->transform().matrix();

        const glm::vec3 worldAxisVector =
            transform_vector(
                nodeTransform,
                axisLocal);
        const float worldAxisScale =
            glm::length(worldAxisVector);

        if (worldAxisScale <= axisEpsilon) {
            return false;
        }

        slideAxisWorld_ =
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
                    slideAxisWorld_,
                    event.pointer.viewRight),
                -glm::dot(
                    slideAxisWorld_,
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
                glm::vec2{ 1.0f, 0.0f };
        }

        axisDragReady_ =
            closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                slideAxisWorld_,
                startAxisPointWorld_);

        if (!axisDragReady_) {
            startAxisPointWorld_ =
                axisOriginWorld_;
        }

        return true;
    }

    bool EdgeSlideTool::has_effective_distance() const
    {
        return std::abs(distance_) >
            options_.distanceEpsilon;
    }

} // namespace locus::editor
