/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/transform/MeshTransformToolSession.h"

#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "editor/tools/transform/MeshTransformTargetResolver.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <string>
#include <utility>
#include <cstddef>

namespace locus::editor {

    namespace {

        constexpr const char* NoGizmoHandleHitMessage =
            "No gizmo handle was hit.";

        [[nodiscard]] glm::mat4 node_world_matrix(
            const EditorScene& scene,
            SceneNodeId node)
        {
            const SceneNode* sceneNode = scene.find_node(node);
            if (!sceneNode) {
                return glm::mat4{ 1.0f };
            }

            const glm::mat4 local = sceneNode->transform().matrix();
            if (sceneNode->parent().is_invalid()) {
                return local;
            }

            return node_world_matrix(scene, sceneNode->parent()) * local;
        }

        [[nodiscard]] glm::vec3 transform_point(
            const glm::mat4& matrix,
            const glm::vec3& point)
        {
            return glm::vec3{ matrix * glm::vec4{ point, 1.0f } };
        }

        [[nodiscard]] glm::vec3 rotate_around_pivot(
            const glm::vec3& point,
            const glm::vec3& pivot,
            const glm::quat& rotation)
        {
            return pivot + glm::normalize(rotation) * (point - pivot);
        }

        [[nodiscard]] glm::vec3 scale_around_pivot(
            const glm::vec3& point,
            const glm::vec3& pivot,
            const glm::vec3& scale)
        {
            return pivot + ((point - pivot) * scale);
        }

    } // namespace

    MeshTransformToolSession::MeshTransformToolSession(
        GizmoController controller)
        : controller_(std::move(controller)) {
    }

    bool MeshTransformToolSession::is_active() const {
        return active_;
    }

    ToolResult MeshTransformToolSession::begin(
        ToolContext& context,
        const TransformToolSessionBeginInput& input) {

        if (is_active()) {
            return ToolResult::fail(
                "A mesh transform session is already active.");
        }

        const MeshTransformTargetResolveResult resolved =
            MeshTransformTargetResolver::resolve(
                context.scene(),
                context.selection());

        if (!resolved.success) {
            return ToolResult::fail(resolved.message);
        }

        target_ = resolved.target;

        if (!capture_original_positions(context)) {
            target_ = {};
            return ToolResult::fail(
                "Could not capture mesh transform vertices.");
        }

        GizmoBeginDragPivotInput beginInput{};
        beginInput.mode = input.mode;
        beginInput.pivot = target_.pivot;
        beginInput.orientation = input.orientation;
        beginInput.sessionOptions = input.options;
        beginInput.pointer = input.pointer;
        beginInput.snapSettings = &context.snap_settings();
        beginInput.snapSolver = input.snapSolver;

        const GizmoControllerResult result =
            controller_.begin_drag_at_pivot(beginInput);

        if (!result.success) {
            vertices_.clear();
            target_ = {};

            if (result.message != nullptr &&
                std::string{ result.message } ==
                NoGizmoHandleHitMessage) {
                return ToolResult::ignored();
            }

            return ToolResult::fail(
                result.message != nullptr
                ? std::string{ result.message }
                : std::string{ "Could not begin mesh transform." });
        }

        active_ = true;

        return ToolResult::started(
            EditorDirtyFlags::Render,
            result.message != nullptr
            ? std::string{ result.message }
            : std::string{ "Mesh transform started." });
    }

    ToolResult MeshTransformToolSession::update(
        ToolContext& context,
        const TransformToolSessionUpdateInput& input) {

        if (!is_active()) {
            return ToolResult::ignored();
        }

        GizmoDragInput dragInput{};
        dragInput.pointer = input.pointer;
        dragInput.snapSettings = &context.snap_settings();
        dragInput.snapSolver = input.snapSolver;

        const GizmoControllerResult result =
            controller_.update_drag_constraint(
                context.scene(),
                dragInput);

        if (!result.success) {
            return ToolResult::fail(
                result.message != nullptr
                ? std::string{ result.message }
                : std::string{ "Could not update mesh transform." });
        }

        bool changed = false;

        switch (result.hit.mode) {
        case GizmoMode::Translate:
            changed = preview_translate(context, result.constraint.translation);
            break;
        case GizmoMode::Rotate:
            changed = preview_rotate(context, result.constraint.rotation);
            break;
        case GizmoMode::Scale:
            changed = preview_scale(context, result.constraint.scale);
            break;
        case GizmoMode::Universal:
        case GizmoMode::None:
        default:
            return ToolResult::fail("Unsupported mesh transform mode.");
        }

        if (!changed) {
            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Mesh transform did not change.");
        }

        return ToolResult::updated(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh transform preview updated.");
    }

    ToolResult MeshTransformToolSession::confirm(
        ToolContext& context) {

        if (!is_active()) {
            return ToolResult::ignored();
        }

        if (!has_changes()) {
            const bool ended = controller_.end_drag_at_pivot();
            clear();

            if (!ended) {
                return ToolResult::fail(
                    "Could not finish the unchanged mesh transform session.");
            }

            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Mesh transform finished without changes.");
        }

        if (!context.has_command_services()) {
            (void)restore_original_positions(context);
            controller_.cancel_drag_at_pivot();
            clear();

            return ToolResult::fail(
                "Mesh transform command services are not available.",
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        const SceneNodeId targetNode = target_.node;
        const std::vector<kernel::geometry::VertexHandle> handles =
            target_.vertices;
        const std::vector<glm::vec3> nextPositions =
            final_positions();

        if (!restore_original_positions(context)) {
            controller_.cancel_drag_at_pivot();
            clear();
            return ToolResult::fail(
                "Could not restore the original mesh state before commit.",
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        CommandResult commandResult =
            context.execute_command(
                std::make_unique<ApplyMeshOperationCommand>(
                    targetNode,
                    [handles, nextPositions](
                        kernel::geometry::LEMEditor& editor) {
                        if (handles.size() != nextPositions.size()) {
                            return false;
                        }

                        bool changed = false;
                        for (std::size_t index = 0; index < handles.size(); ++index) {
                            changed = editor.set_vertex_position(
                                handles[index],
                                nextPositions[index]) || changed;
                        }

                        return changed;
                    },
                    "Transform Mesh Components"));

        if (!commandResult.success) {
            controller_.cancel_drag_at_pivot();
            clear();

            ToolResult failure =
                from_command_result(std::move(commandResult));
            failure.dirtyFlags =
                failure.dirtyFlags |
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking;

            return failure;
        }

        const bool ended = controller_.end_drag_at_pivot();
        clear();

        if (!ended) {
            return ToolResult::fail(
                "Mesh transform was committed, but the gizmo session could not be finalized.",
                commandResult.dirtyFlags);
        }

        return from_command_result(std::move(commandResult));
    }

    ToolResult MeshTransformToolSession::cancel(
        ToolContext& context,
        ToolCancelReason reason) {

        (void)reason;

        if (!is_active()) {
            return ToolResult::ignored();
        }

        const bool restored =
            restore_original_positions(context);

        controller_.cancel_drag_at_pivot();
        clear();

        if (!restored) {
            return ToolResult::fail(
                "Could not restore the mesh transform session.");
        }

        return ToolResult::cancelled(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh transform cancelled.");
    }

    void MeshTransformToolSession::clear() {
        controller_.clear();
        target_ = {};
        vertices_.clear();
        active_ = false;
    }

    GizmoController& MeshTransformToolSession::controller() {
        return controller_;
    }

    const GizmoController& MeshTransformToolSession::controller() const {
        return controller_;
    }

    const MeshTransformTarget& MeshTransformToolSession::target() const {
        return target_;
    }

    bool MeshTransformToolSession::preview_translate(
        ToolContext& context,
        const glm::vec3& worldTranslation) {

        const glm::mat4 world =
            node_world_matrix(context.scene(), target_.node);
        const glm::mat4 inverseWorld =
            glm::inverse(world);

        std::vector<glm::vec3> positions{};
        positions.reserve(vertices_.size());

        for (const VertexSnapshot& snapshot : vertices_) {
            const glm::vec3 originalWorld =
                transform_point(world, snapshot.originalPosition);
            positions.push_back(
                transform_point(inverseWorld, originalWorld + worldTranslation));
        }

        return apply_preview_positions(context, positions);
    }

    bool MeshTransformToolSession::preview_rotate(
        ToolContext& context,
        const glm::quat& worldRotation) {

        const glm::mat4 world =
            node_world_matrix(context.scene(), target_.node);
        const glm::mat4 inverseWorld =
            glm::inverse(world);

        std::vector<glm::vec3> positions{};
        positions.reserve(vertices_.size());

        for (const VertexSnapshot& snapshot : vertices_) {
            const glm::vec3 originalWorld =
                transform_point(world, snapshot.originalPosition);
            const glm::vec3 candidateWorld =
                rotate_around_pivot(
                    originalWorld,
                    target_.pivot,
                    worldRotation);

            positions.push_back(
                transform_point(inverseWorld, candidateWorld));
        }

        return apply_preview_positions(context, positions);
    }

    bool MeshTransformToolSession::preview_scale(
        ToolContext& context,
        const glm::vec3& worldScale) {

        const glm::mat4 world =
            node_world_matrix(context.scene(), target_.node);
        const glm::mat4 inverseWorld =
            glm::inverse(world);

        std::vector<glm::vec3> positions{};
        positions.reserve(vertices_.size());

        for (const VertexSnapshot& snapshot : vertices_) {
            const glm::vec3 originalWorld =
                transform_point(world, snapshot.originalPosition);
            const glm::vec3 candidateWorld =
                scale_around_pivot(
                    originalWorld,
                    target_.pivot,
                    worldScale);

            positions.push_back(
                transform_point(inverseWorld, candidateWorld));
        }

        return apply_preview_positions(context, positions);
    }

    bool MeshTransformToolSession::capture_original_positions(
        ToolContext& context) {

        MeshNode* node =
            context.scene().find_mesh(target_.node);
        if (!node) {
            return false;
        }

        const kernel::geometry::LEM& mesh = node->mesh();
        vertices_.clear();
        vertices_.reserve(target_.vertices.size());

        for (kernel::geometry::VertexHandle vertex : target_.vertices) {
            if (!mesh.is_valid(vertex)) {
                vertices_.clear();
                return false;
            }

            VertexSnapshot snapshot{};
            snapshot.vertex = vertex;
            snapshot.originalPosition = mesh.vertex(vertex).position;
            snapshot.previewPosition = snapshot.originalPosition;
            vertices_.push_back(snapshot);
        }

        return !vertices_.empty();
    }

    bool MeshTransformToolSession::apply_preview_positions(
        ToolContext& context,
        const std::vector<glm::vec3>& positions) {

        if (positions.size() != vertices_.size()) {
            return false;
        }

        MeshNode* node =
            context.scene().find_mesh(target_.node);
        if (!node) {
            return false;
        }

        kernel::geometry::LEMEditor editor(node->mesh());
        bool changed = false;

        for (std::size_t index = 0; index < vertices_.size(); ++index) {
            if (editor.set_vertex_position(
                vertices_[index].vertex,
                positions[index])) {

                changed =
                    changed ||
                    glm::length(vertices_[index].previewPosition - positions[index])
                    > 0.00001f;
                vertices_[index].previewPosition = positions[index];
            }
        }

        if (changed) {
            node->bump_mesh_revision();
            node->mark_dirty(
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
            context.mark_dirty(
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        return changed;
    }

    bool MeshTransformToolSession::restore_original_positions(
        ToolContext& context) {

        MeshNode* node =
            context.scene().find_mesh(target_.node);
        if (!node) {
            return false;
        }

        kernel::geometry::LEMEditor editor(node->mesh());
        bool touched = false;

        for (const VertexSnapshot& snapshot : vertices_) {
            if (!editor.set_vertex_position(
                snapshot.vertex,
                snapshot.originalPosition)) {
                return false;
            }

            touched = true;
        }

        if (touched) {
            for (VertexSnapshot& snapshot : vertices_) {
                snapshot.previewPosition = snapshot.originalPosition;
            }

            node->bump_mesh_revision();
            node->mark_dirty(
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
            context.mark_dirty(
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        return touched;
    }

    bool MeshTransformToolSession::has_changes(float epsilon) const {
        for (const VertexSnapshot& snapshot : vertices_) {
            if (glm::length(snapshot.previewPosition - snapshot.originalPosition)
                > epsilon) {
                return true;
            }
        }

        return false;
    }

    std::vector<glm::vec3> MeshTransformToolSession::final_positions() const {
        std::vector<glm::vec3> positions{};
        positions.reserve(vertices_.size());

        for (const VertexSnapshot& snapshot : vertices_) {
            positions.push_back(snapshot.previewPosition);
        }

        return positions;
    }

    ToolResult MeshTransformToolSession::from_command_result(
        CommandResult result) {

        if (!result.success) {
            return ToolResult::fail(
                std::move(result.message),
                result.dirtyFlags);
        }

        std::string message = std::move(result.message);
        if (message.empty()) {
            message = "Mesh transform committed.";
        }

        return ToolResult::confirmed(
            result.dirtyFlags,
            std::move(message));
    }

} // namespace locus::editor
