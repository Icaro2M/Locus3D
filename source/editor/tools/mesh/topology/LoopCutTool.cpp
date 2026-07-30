/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/topology/LoopCutTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/MeshNode.h"
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

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace locus::editor {

    namespace {

        constexpr float minimumVisualScale =
            0.000001f;

        constexpr float kernelMinimumFactor =
            0.0001f;

        constexpr float kernelMaximumFactor =
            0.9999f;

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

        if (uses_interactive_factor() &&
            !initialize_edge_drag(
                context,
                event,
                target)) {
            return ToolResult::fail(
                "Loop cut could not initialize a stable target edge drag.");
        }

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

        cutAxisWorld_ =
            glm::vec3{ 1.0f, 0.0f, 0.0f };

        axisOriginWorld_ =
            glm::vec3{ 0.0f };

        startAxisPointWorld_ =
            glm::vec3{ 0.0f };

        fallbackScreenAxis_ =
            glm::vec2{ 1.0f, 0.0f };

        interactionVisualScale_ =
            1.0f;

        worldDistanceToFactor_ =
            1.0f;

        edgeDragReady_ =
            false;

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
        if (edgeDragReady_) {
            glm::vec3 currentAxisPoint{ 0.0f };

            if (closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                cutAxisWorld_,
                currentAxisPoint)) {
                const float worldDistance =
                    glm::dot(
                        currentAxisPoint - startAxisPointWorld_,
                        cutAxisWorld_);

                float rawFactor =
                    options_.initialFactor +
                    worldDistance *
                    worldDistanceToFactor_;

                if (options_.invertDragDirection) {
                    rawFactor =
                        options_.initialFactor -
                        worldDistance *
                        worldDistanceToFactor_;
                }

                return std::clamp(
                    rawFactor,
                    options_.minimumFactor,
                    options_.maximumFactor);
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

    bool LoopCutTool::initialize_edge_drag(
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
        const kernel::geometry::EdgeHandle edge =
            target.edges.front();

        if (!mesh.is_valid(edge)) {
            return false;
        }

        const kernel::geometry::Edge& edgeElement =
            mesh.edge(edge);

        if (!mesh.is_valid(edgeElement.vertexA) ||
            !mesh.is_valid(edgeElement.vertexB)) {
            return false;
        }

        const glm::vec3 vertexA =
            mesh.vertex(edgeElement.vertexA).position;
        const glm::vec3 vertexB =
            mesh.vertex(edgeElement.vertexB).position;
        const glm::vec3 axisLocal =
            vertexB - vertexA;
        const float localLength =
            glm::length(axisLocal);

        if (localLength <= axisEpsilon) {
            return false;
        }

        const glm::mat4 nodeTransform =
            node->transform().matrix();

        const glm::vec3 worldAxisVector =
            transform_vector(
                nodeTransform,
                axisLocal / localLength);
        const float worldAxisLength =
            glm::length(worldAxisVector);

        if (worldAxisLength <= axisEpsilon) {
            return false;
        }

        cutAxisWorld_ =
            worldAxisVector / worldAxisLength;
        axisOriginWorld_ =
            transform_point(
                nodeTransform,
                vertexA + axisLocal * options_.initialFactor);
        worldDistanceToFactor_ =
            1.0f / (localLength * worldAxisLength);

        fallbackScreenAxis_ =
            glm::vec2{
                glm::dot(
                    cutAxisWorld_,
                    event.pointer.viewRight),
                -glm::dot(
                    cutAxisWorld_,
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

        edgeDragReady_ =
            closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                cutAxisWorld_,
                startAxisPointWorld_);

        if (!edgeDragReady_) {
            startAxisPointWorld_ =
                axisOriginWorld_;
        }

        return true;
    }

    bool LoopCutTool::uses_interactive_factor() const
    {
        return options_.cuts == 1 &&
            !options_.evenSpacing;
    }

} // namespace locus::editor
