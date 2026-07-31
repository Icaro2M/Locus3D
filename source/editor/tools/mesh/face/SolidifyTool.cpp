/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/mesh/face/SolidifyTool.h"

#include "editor/command/CommandResult.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/SolidifyOp.h"

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
        bool contains_face(
            const std::vector<kernel::geometry::FaceHandle>& faces,
            kernel::geometry::FaceHandle face)
        {
            return std::find(
                faces.begin(),
                faces.end(),
                face) != faces.end();
        }

        [[nodiscard]]
        bool region_has_external_adjacent_faces(
            const kernel::geometry::LEM& mesh,
            const std::vector<kernel::geometry::FaceHandle>& faces)
        {
            for (const kernel::geometry::FaceHandle face : faces) {
                if (!mesh.is_valid(face)) {
                    return true;
                }

                const std::vector<kernel::geometry::LoopHandle> loops =
                    mesh.face_loops(face);

                for (const kernel::geometry::LoopHandle loop : loops) {
                    if (!mesh.is_valid(loop)) {
                        return true;
                    }

                    const kernel::geometry::EdgeHandle edge =
                        mesh.loop(loop).edge;

                    if (!mesh.is_valid(edge)) {
                        return true;
                    }

                    const std::vector<kernel::geometry::LoopHandle>
                        radialLoops =
                        kernel::geometry::TopologyTraversal::edge_loops(
                            mesh,
                            edge);

                    for (const kernel::geometry::LoopHandle radialLoop :
                        radialLoops) {
                        if (!mesh.is_valid(radialLoop)) {
                            return true;
                        }

                        const kernel::geometry::FaceHandle adjacentFace =
                            mesh.loop(radialLoop).face;

                        if (mesh.is_valid(adjacentFace) &&
                            !contains_face(
                                faces,
                                adjacentFace)) {
                            return true;
                        }
                    }
                }
            }

            return false;
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

    SolidifyTool::SolidifyTool()
        : SolidifyTool(
            SolidifyToolOptions{})
    {
    }

    SolidifyTool::SolidifyTool(
        SolidifyToolOptions options)
        : MeshDragOperationTool(
            make_descriptor(),
            SelectionGranularity::Face,
            DragCompletionPolicy::ConfirmOnRelease),
        options_(
            sanitize_options(
                std::move(options)))
    {
    }

    const SolidifyToolOptions&
        SolidifyTool::options() const
    {
        return options_;
    }

    bool SolidifyTool::set_options(
        const SolidifyToolOptions& options)
    {
        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ =
            sanitize_options(options);

        return true;
    }

    float SolidifyTool::thickness() const
    {
        return thickness_;
    }

    ToolResult SolidifyTool::begin_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        if (!target.targets_faces()) {
            return ToolResult::fail(
                "Solidify requires at least one selected face.");
        }

        startPosition_ =
            event.pointer.viewportPosition;

        interactionVisualScale_ =
            event.pointer.visualScale > minimumVisualScale
            ? event.pointer.visualScale
            : 1.0f;

        thickness_ =
            0.0f;

        const MeshNode* node =
            context.scene().find_mesh(
                target.nodeId);

        if (node == nullptr) {
            return ToolResult::fail(
                "Solidify requires a valid mesh node.");
        }

        keepSourceFacesForOperation_ =
            options_.keepSourceFaces &&
            !region_has_external_adjacent_faces(
                node->mesh(),
                target.faces);

        if (!initialize_axis_drag(
            context,
            event,
            target)) {
            return ToolResult::fail(
                "Solidify could not initialize a stable normal drag axis.");
        }

        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Solidify interaction started.");
    }

    ToolResult SolidifyTool::update_mesh_operation(
        ToolContext& context,
        const ToolEvent& event,
        const MeshToolTarget& target)
    {
        (void)context;
        (void)target;

        const float previousThickness =
            thickness_;

        thickness_ =
            calculate_thickness(event);

        if (std::abs(
            thickness_ - previousThickness) <=
            options_.thicknessEpsilon) {
            return ToolResult::ignored();
        }

        return ToolResult::updated(
            EditorDirtyFlags::None,
            "Solidify thickness updated.");
    }

    std::unique_ptr<kernel::modeling::IOperation>
        SolidifyTool::build_preview_operation(
            const ToolContext& context,
            const MeshToolTarget& target) const
    {
        (void)context;

        if (!target.targets_faces()) {
            return nullptr;
        }

        auto operation =
            std::make_unique<
            kernel::modeling::SolidifyOp>(
                target.faces,
                thickness_);

        operation->set_keep_source_faces(
            keepSourceFacesForOperation_);
        operation->set_create_caps(
            options_.createCaps);
        operation->set_create_rims(
            options_.createRims);
        operation->set_flip_caps(
            options_.flipCaps);
        operation->set_flip_rims(
            options_.flipRims);

        return operation;
    }

    ToolResult SolidifyTool::commit_mesh_operation(
        ToolContext& context,
        const MeshToolTarget& target)
    {
        if (!target.targets_faces()) {
            return ToolResult::fail(
                "Cannot commit Solidify without a valid face target.");
        }

        if (!has_effective_thickness()) {
            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Solidify completed without changes.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Cannot commit Solidify because command services are "
                "unavailable.");
        }

        const SceneNodeId nodeId =
            target.nodeId;

        const std::vector<kernel::geometry::FaceHandle> faces =
            target.faces;

        const float committedThickness =
            thickness_;

        const bool keepSourceFaces =
            keepSourceFacesForOperation_;

        const bool createCaps =
            options_.createCaps;

        const bool createRims =
            options_.createRims;

        const bool flipCaps =
            options_.flipCaps;

        const bool flipRims =
            options_.flipRims;

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
                    committedThickness,
                    keepSourceFaces,
                    createCaps,
                    createRims,
                    flipCaps,
                    flipRims,
                    validateAfterExecute,
                    rebuildNormals,
                    allowNonManifold
                ](
                    kernel::geometry::LEMEditor& editor)
                {
                    kernel::modeling::SolidifyOp operation{
                        faces,
                        committedThickness
                    };

                    operation.set_keep_source_faces(
                        keepSourceFaces);
                    operation.set_create_caps(
                        createCaps);
                    operation.set_create_rims(
                        createRims);
                    operation.set_flip_caps(
                        flipCaps);
                    operation.set_flip_rims(
                        flipRims);

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
                "Solidify Faces");

        const CommandResult commandResult =
            context.execute_command(
                std::move(command));

        if (!commandResult.success) {
            return ToolResult::fail(
                commandResult.message.empty()
                ? "Solidify command failed."
                : commandResult.message,
                commandResult.dirtyFlags);
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            commandResult.message.empty()
            ? "Faces solidified successfully."
            : commandResult.message);
    }

    void SolidifyTool::clear_mesh_operation()
    {
        startPosition_ =
            glm::vec2{ 0.0f };

        solidifyAxisWorld_ =
            glm::vec3{ 0.0f, 1.0f, 0.0f };

        axisOriginWorld_ =
            glm::vec3{ 0.0f };

        startAxisPointWorld_ =
            glm::vec3{ 0.0f };

        fallbackScreenAxis_ =
            glm::vec2{ 0.0f, -1.0f };

        interactionVisualScale_ =
            1.0f;

        worldDistanceToLocalThickness_ =
            1.0f;

        axisDragReady_ =
            false;

        keepSourceFacesForOperation_ =
            true;

        thickness_ =
            0.0f;
    }

    ToolDescriptor SolidifyTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{
                std::string{
                    Id
                }
            },
            "Solidify Faces",
            "Gives selected mesh faces thickness along their normals.",
            ToolCategory::Mesh,
            ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::Modal
        };
    }

    SolidifyToolOptions
        SolidifyTool::sanitize_options(
            SolidifyToolOptions options)
    {
        options.thicknessPerPixel =
            std::max(
                0.0f,
                options.thicknessPerPixel);

        options.thicknessEpsilon =
            std::max(
                0.0f,
                options.thicknessEpsilon);

        return options;
    }

    float SolidifyTool::calculate_thickness(
        const ToolEvent& event) const
    {
        if (axisDragReady_) {
            glm::vec3 currentAxisPoint{ 0.0f };

            if (closest_point_on_axis_from_ray(
                event.pointer.worldRay,
                axisOriginWorld_,
                solidifyAxisWorld_,
                currentAxisPoint)) {
                const float worldDistance =
                    glm::dot(
                        currentAxisPoint - startAxisPointWorld_,
                        solidifyAxisWorld_);

                return worldDistance *
                    worldDistanceToLocalThickness_;
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
            options_.thicknessPerPixel *
            interactionVisualScale_;
    }

    bool SolidifyTool::initialize_axis_drag(
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

        solidifyAxisWorld_ =
            worldAxisVector / worldAxisScale;
        axisOriginWorld_ =
            transform_point(
                nodeTransform,
                centerLocal);
        worldDistanceToLocalThickness_ =
            1.0f / worldAxisScale;

        fallbackScreenAxis_ =
            glm::vec2{
                glm::dot(
                    solidifyAxisWorld_,
                    event.pointer.viewRight),
                -glm::dot(
                    solidifyAxisWorld_,
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
                solidifyAxisWorld_,
                startAxisPointWorld_);

        if (!axisDragReady_) {
            startAxisPointWorld_ =
                axisOriginWorld_;
        }

        return true;
    }

    bool SolidifyTool::has_effective_thickness() const
    {
        return std::abs(thickness_) >
            options_.thicknessEpsilon;
    }

} // namespace locus::editor
