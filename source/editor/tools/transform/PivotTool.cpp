/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/transform/PivotTool.h"

#include "editor/command/transform/SetNodePivotCommand.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/transform/TransformPivotResolver.h"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace locus::editor {

    namespace {

        constexpr float Epsilon = 0.000001f;

        [[nodiscard]] bool same_pivot(
            const NodePivot& a,
            const NodePivot& b) noexcept
        {
            return a.custom == b.custom &&
                glm::length(a.offset - b.offset) <= 0.00001f;
        }

    } // namespace

    PivotTool::PivotTool()
        : DragTool(
            make_descriptor(),
            DragCompletionPolicy::ConfirmOnRelease) {
    }

    ToolDescriptor PivotTool::make_descriptor()
    {
        return ToolDescriptor{
            ToolId{ Id },
            "Pivot",
            "Edits the active object's transform pivot.",
            ToolCategory::Transform,
            ToolCapabilities::ObjectMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesPreview |
                ToolCapabilities::UsesSnapping |
                ToolCapabilities::Modal
        };
    }

    bool PivotTool::hovered() const noexcept
    {
        return hovered_;
    }

    bool PivotTool::dragging() const noexcept
    {
        return dragging_;
    }

    SceneNodeId PivotTool::active_node() const noexcept
    {
        return activeNode_;
    }

    bool PivotTool::can_activate_tool(
        const ToolContext& context) const
    {
        return resolve_active_object(context).is_valid();
    }

    ToolResult PivotTool::on_activate(
        ToolContext& context)
    {
        activeNode_ = resolve_active_object(context);
        hovered_ = false;
        dragging_ = false;
        changed_ = false;

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Pivot tool activated.");
    }

    ToolResult PivotTool::on_deactivate(
        ToolContext& context)
    {
        if (dragging_) {
            restore_initial(context);
        }

        clear_state();

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Pivot tool deactivated.");
    }

    ToolResult PivotTool::on_pointer_hover(
        ToolContext& context,
        const ToolEvent& event)
    {
        activeNode_ = resolve_active_object(context);
        const bool previousHovered = hovered_;
        hovered_ = activeNode_.is_valid() && hit_test(context, event);

        if (hovered_ == previousHovered) {
            return ToolResult::ignored();
        }

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            hovered_
            ? "Pivot marker hovered."
            : "Pivot marker hover cleared.");
    }

    ToolResult PivotTool::on_begin_drag(
        ToolContext& context,
        const ToolEvent& event)
    {
        activeNode_ = resolve_active_object(context);
        if (activeNode_.is_invalid()) {
            return ToolResult::ignored();
        }

        SceneNode* node = context.scene().find_node(activeNode_);
        if (node == nullptr) {
            return ToolResult::ignored();
        }

        if (!hit_test(context, event)) {
            hovered_ = false;
            return ToolResult::ignored();
        }

        const float normalLength = glm::length(event.pointer.viewDirection);
        if (normalLength <= Epsilon) {
            return ToolResult::fail(
                "Pivot drag requires a valid camera view direction.");
        }

        initialPivot_ = node->pivot();
        previewPivot_ = initialPivot_;
        initialPivotWorld_ =
            TransformPivotResolver::node_pivot_position(
                context.scene(),
                activeNode_);
        dragPlaneNormal_ =
            glm::normalize(event.pointer.viewDirection);
        hovered_ = true;
        dragging_ = true;
        changed_ = false;

        return ToolResult::started(
            EditorDirtyFlags::Render,
            "Pivot drag started.");
    }

    ToolResult PivotTool::on_update_drag(
        ToolContext& context,
        const ToolEvent& event)
    {
        if (!dragging_ || activeNode_.is_invalid()) {
            return ToolResult::ignored();
        }

        glm::vec3 worldPosition{};
        if (!intersect_drag_plane(
                event,
                initialPivotWorld_,
                dragPlaneNormal_,
                worldPosition)) {
            return ToolResult::ignored();
        }

        NodePivot nextPivot =
            pivot_from_world_position(
                context,
                worldPosition);

        if (same_pivot(nextPivot, previewPivot_)) {
            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Pivot preview unchanged.");
        }

        apply_preview(context, nextPivot);
        changed_ = !same_pivot(previewPivot_, initialPivot_);

        return ToolResult::updated(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Pivot preview updated.");
    }

    ToolResult PivotTool::on_release_drag(
        ToolContext& context,
        const ToolEvent& event)
    {
        (void)context;
        (void)event;

        if (!dragging_) {
            return ToolResult::ignored();
        }

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Pivot pointer released.");
    }

    ToolResult PivotTool::on_confirm_drag(
        ToolContext& context)
    {
        if (!dragging_ || activeNode_.is_invalid()) {
            return ToolResult::ignored();
        }

        const SceneNodeId nodeId = activeNode_;
        const NodePivot finalPivot = previewPivot_;

        restore_initial(context);

        if (!changed_ || same_pivot(finalPivot, initialPivot_)) {
            clear_state();
            return ToolResult::confirmed(
                EditorDirtyFlags::Render,
                "Pivot drag finished without changes.");
        }

        if (!context.has_command_services()) {
            clear_state();
            return ToolResult::fail(
                "Pivot command services are not available.",
                EditorDirtyFlags::Scene |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        CommandResult commandResult =
            context.execute_command(
                std::make_unique<SetNodePivotCommand>(
                    nodeId,
                    finalPivot));

        clear_state();

        if (!commandResult.success) {
            return ToolResult::fail(
                std::move(commandResult.message),
                commandResult.dirtyFlags);
        }

        std::string message =
            std::move(commandResult.message);
        if (message.empty()) {
            message = "Pivot changed.";
        }

        return ToolResult::confirmed(
            commandResult.dirtyFlags,
            std::move(message));
    }

    ToolResult PivotTool::on_cancel_drag(
        ToolContext& context,
        ToolCancelReason reason)
    {
        (void)reason;

        if (!dragging_) {
            hovered_ = false;
            return ToolResult::ignored();
        }

        restore_initial(context);
        clear_state();

        return ToolResult::cancelled(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Pivot drag cancelled.");
    }

    SceneNodeId PivotTool::resolve_active_object(
        const ToolContext& context)
    {
        if (context.selection().scope() != SelectionScope::Scene ||
            context.selection().granularity() !=
            SelectionGranularity::Object) {
            return {};
        }

        const SceneNodeId active =
            context.selection().objects().active();
        if (active.is_valid() &&
            context.selection().objects().contains(active) &&
            context.scene().find_node(active) != nullptr) {
            return active;
        }

        if (context.selection().objects().size() == 1u) {
            const SceneNodeId selected =
                context.selection().objects().selected().front();
            if (context.scene().find_node(selected) != nullptr) {
                return selected;
            }
        }

        return {};
    }

    bool PivotTool::project_to_viewport(
        const ToolEvent& event,
        const glm::vec3& worldPosition,
        glm::vec2& screenPosition)
    {
        if (event.pointer.viewportSize.x <= 0.0f ||
            event.pointer.viewportSize.y <= 0.0f) {
            return false;
        }

        const glm::vec4 clip =
            event.pointer.viewProjection *
            glm::vec4{ worldPosition, 1.0f };

        if (clip.w <= Epsilon) {
            return false;
        }

        const glm::vec3 ndc =
            glm::vec3{ clip } / clip.w;

        screenPosition = glm::vec2{
            (ndc.x * 0.5f + 0.5f) * event.pointer.viewportSize.x,
            (ndc.y * 0.5f + 0.5f) * event.pointer.viewportSize.y
        };

        return
            ndc.x >= -1.0f && ndc.x <= 1.0f &&
            ndc.y >= -1.0f && ndc.y <= 1.0f &&
            ndc.z >= -1.0f && ndc.z <= 1.0f;
    }

    bool PivotTool::intersect_drag_plane(
        const ToolEvent& event,
        const glm::vec3& planePoint,
        const glm::vec3& planeNormal,
        glm::vec3& intersection)
    {
        const glm::vec3 rayDirection =
            event.pointer.worldRay.direction;
        const float denominator =
            glm::dot(rayDirection, planeNormal);

        if (std::abs(denominator) <= Epsilon) {
            return false;
        }

        const float distance =
            glm::dot(
                planePoint - event.pointer.worldRay.origin,
                planeNormal) /
            denominator;

        if (!std::isfinite(distance)) {
            return false;
        }

        intersection =
            event.pointer.worldRay.origin +
            rayDirection * distance;
        return true;
    }

    bool PivotTool::hit_test(
        const ToolContext& context,
        const ToolEvent& event) const
    {
        const SceneNodeId node = resolve_active_object(context);
        if (node.is_invalid()) {
            return false;
        }

        const glm::vec3 pivotWorld =
            TransformPivotResolver::node_pivot_position(
                context.scene(),
                node);

        glm::vec2 screenPosition{};
        if (!project_to_viewport(event, pivotWorld, screenPosition)) {
            return false;
        }

        return glm::length(
            event.pointer.viewportPosition - screenPosition) <=
            hitRadiusPixels_;
    }

    NodePivot PivotTool::pivot_from_world_position(
        const ToolContext& context,
        const glm::vec3& worldPosition) const
    {
        NodePivot pivot{};
        pivot.offset =
            TransformPivotResolver::node_local_offset_from_world(
                context.scene(),
                activeNode_,
                worldPosition);
        pivot.custom = glm::length(pivot.offset) > 0.00001f;
        return pivot;
    }

    void PivotTool::apply_preview(
        ToolContext& context,
        const NodePivot& pivot)
    {
        SceneNode* node =
            context.scene().find_node(activeNode_);
        if (node == nullptr) {
            return;
        }

        node->pivot() = pivot;
        node->mark_dirty(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking);
        context.mark_dirty(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking);
        previewPivot_ = pivot;
    }

    void PivotTool::restore_initial(
        ToolContext& context)
    {
        SceneNode* node =
            context.scene().find_node(activeNode_);
        if (node == nullptr) {
            return;
        }

        node->pivot() = initialPivot_;
        node->mark_dirty(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking);
        context.mark_dirty(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking);
    }

    void PivotTool::clear_state() noexcept
    {
        activeNode_ = {};
        initialPivot_ = {};
        previewPivot_ = {};
        initialPivotWorld_ = glm::vec3{ 0.0f, 0.0f, 0.0f };
        dragPlaneNormal_ = glm::vec3{ 0.0f, 0.0f, -1.0f };
        hovered_ = false;
        dragging_ = false;
        changed_ = false;
    }

} // namespace locus::editor
