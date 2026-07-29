/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/transform/TransformTool.h"

#include "editor/selection/ObjectSelection.h"
#include "editor/transform/TransformPivotResolver.h"

#include <glm/geometric.hpp>

#include <string>

namespace locus::editor {

    namespace {

        constexpr float orientationEpsilon = 0.000001f;

    } // namespace

    TransformTool::TransformTool()
        : TransformTool(GizmoMode::Translate) {
    }

    TransformTool::TransformTool(GizmoMode mode)
        : DragTool(
            make_descriptor(),
            DragCompletionPolicy::ConfirmOnRelease),
        mode_(
            mode == GizmoMode::None
            ? GizmoMode::Translate
            : mode) {
    }

    ToolDescriptor TransformTool::make_descriptor() {
        return ToolDescriptor{
            ToolId{ Id },
            "Transform",
            "Translates, rotates, and scales editor objects and mesh components.",
            ToolCategory::Transform,
            ToolCapabilities::ObjectMode |
                ToolCapabilities::MeshMode |
                ToolCapabilities::RequiresSelection |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesGizmo |
                ToolCapabilities::UsesSnapping |
                ToolCapabilities::Modal
        };
    }

    GizmoMode TransformTool::mode() const {
        return mode_;
    }

    bool TransformTool::set_mode(GizmoMode mode) {
        if (state() == ToolState::Interacting ||
            mode == GizmoMode::None) {

            return false;
        }

        mode_ = mode;

        GizmoState& gizmoState =
            objectSession_
            .controller()
            .state();

        gizmoState.mode = mode_;
        gizmoState.clear_hover();

        return true;
    }

    const TransformSessionOptions&
        TransformTool::options() const {

        return options_;
    }

    bool TransformTool::set_options(
        const TransformSessionOptions& options) {

        if (state() == ToolState::Interacting) {
            return false;
        }

        options_ = options;

        objectSession_
            .controller()
            .state()
            .space = options_.space;

        return true;
    }

    const glm::quat&
        TransformTool::orientation() const {

        return orientation_;
    }

    bool TransformTool::set_orientation(
        const glm::quat& orientation) {

        if (state() == ToolState::Interacting) {
            return false;
        }

        const float length =
            glm::length(orientation);

        if (length <= orientationEpsilon) {
            return false;
        }

        orientation_ =
            glm::normalize(orientation);

        objectSession_
            .controller()
            .state()
            .orientation = orientation_;

        return true;
    }

    ObjectTransformToolSession&
        TransformTool::object_session() {

        return objectSession_;
    }

    const ObjectTransformToolSession&
        TransformTool::object_session() const {

        return objectSession_;
    }

    const GizmoState&
        TransformTool::gizmo_state() const {

        return objectSession_
            .controller()
            .state();
    }

    bool TransformTool::can_activate_tool(
        const ToolContext& context) const {

        return context.mode() == EditorMode::Object;
    }

    ToolResult TransformTool::on_activate(
        ToolContext& context) {

        (void)context;

        GizmoState& gizmoState =
            objectSession_
            .controller()
            .state();

        gizmoState.enabled = true;
        gizmoState.visible = true;
        gizmoState.mode = mode_;
        gizmoState.space = options_.space;
        gizmoState.orientation = orientation_;
        gizmoState.clear_hover();
        gizmoState.clear_active();

        activeSession_ = nullptr;
        refresh_gizmo_presentation(context);

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Transform tool activated.");
    }

    ToolResult TransformTool::on_deactivate(
        ToolContext& context) {

        (void)context;

        objectSession_.clear();
        activeSession_ = nullptr;

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Transform tool deactivated.");
    }

    ToolResult TransformTool::on_pointer_hover(
        ToolContext& context,
        const ToolEvent& event) {

        GizmoController& controller =
            objectSession_.controller();

        const GizmoHit previous =
            controller.state().hovered;

        if (context.mode() != EditorMode::Object ||
            context.selection().objects().empty()) {

            controller.state().clear_hover();

            if (previous.is_valid()) {
                return ToolResult::consumed(
                    EditorDirtyFlags::Render,
                    "Transform gizmo hover cleared.");
            }

            return ToolResult::ignored();
        }

        GizmoHoverInput input{};
        input.mode = mode_;
        input.pivot =
            resolve_object_pivot(context);

        input.orientation = orientation_;
        input.pointer =
            make_gizmo_pointer(event);

        const GizmoHit hovered =
            controller.update_hover(input);

        const bool changed =
            hovered.is_valid() != previous.is_valid() ||
            hovered.mode != previous.mode ||
            hovered.axis != previous.axis;

        if (!changed) {
            return ToolResult::ignored();
        }

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            hovered.is_valid()
            ? "Transform gizmo handle hovered."
            : "Transform gizmo hover cleared.");
    }

    ToolResult TransformTool::on_begin_drag(
        ToolContext& context,
        const ToolEvent& event) {

        if (context.mode() != EditorMode::Object) {
            return ToolResult::fail(
                "Object transform requires object editor mode.");
        }

        if (context.selection().objects().empty()) {
            return ToolResult::ignored();
        }

        activeSession_ = &objectSession_;

        TransformToolSessionBeginInput input{};
        input.mode = mode_;
        input.orientation = orientation_;
        input.options = options_;
        input.pointer =
            make_gizmo_pointer(event);

        ToolResult result =
            activeSession_->begin(
                context,
                input);

        if (result.failed() ||
            !result.was_consumed()) {

            activeSession_ = nullptr;
        }

        return result;
    }

    ToolResult TransformTool::on_update_drag(
        ToolContext& context,
        const ToolEvent& event) {

        if (!activeSession_ ||
            !activeSession_->is_active()) {

            return ToolResult::ignored();
        }

        TransformToolSessionUpdateInput input{};
        input.pointer =
            make_gizmo_pointer(event);

        return activeSession_->update(
            context,
            input);
    }

    ToolResult TransformTool::on_release_drag(
        ToolContext& context,
        const ToolEvent& event) {

        (void)context;
        (void)event;

        if (!activeSession_ ||
            !activeSession_->is_active()) {

            return ToolResult::ignored();
        }

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Transform pointer released.");
    }

    ToolResult TransformTool::on_confirm_drag(
        ToolContext& context) {

        if (!activeSession_) {
            return ToolResult::ignored();
        }

        ToolResult result =
            activeSession_->confirm(context);

        if (!result.failed()) {
            activeSession_ = nullptr;
            refresh_gizmo_presentation(context);
        }

        return result;
    }

    ToolResult TransformTool::on_cancel_drag(
        ToolContext& context,
        ToolCancelReason reason) {

        if (!activeSession_) {
            return ToolResult::ignored();
        }

        ToolResult result =
            activeSession_->cancel(
                context,
                reason);

        if (!result.failed()) {
            activeSession_ = nullptr;
            refresh_gizmo_presentation(context);
        }

        return result;
    }

    GizmoPointerInput
        TransformTool::make_gizmo_pointer(
            const ToolEvent& event) {

        GizmoPointerInput input{};

        input.ray.origin =
            event.pointer.worldRay.origin;

        input.ray.direction =
            event.pointer.worldRay.direction;

        input.viewDirection =
            event.pointer.viewDirection;

        input.viewRight =
            event.pointer.viewRight;

        input.viewUp =
            event.pointer.viewUp;

        input.visualScale =
            event.pointer.visualScale;

        return input;
    }

    glm::vec3 TransformTool::resolve_object_pivot(
        const ToolContext& context) const {

        const ObjectSelection& objects =
            context.selection().objects();

        return TransformPivotResolver::resolve(
            context.scene(),
            objects.selected(),
            objects.active(),
            options_.pivotMode,
            options_.customPivot);
    }

    void TransformTool::refresh_gizmo_presentation(
        const ToolContext& context) {

        GizmoState& gizmoState =
            objectSession_
            .controller()
            .state();

        gizmoState.enabled = true;
        gizmoState.visible =
            context.mode() == EditorMode::Object &&
            !context.selection().objects().empty();
        gizmoState.mode = mode_;
        gizmoState.space = options_.space;
        gizmoState.orientation = orientation_;

        if (gizmoState.visible) {
            gizmoState.pivot =
                resolve_object_pivot(context);
        }
        else {
            gizmoState.clear_hover();
        }

        gizmoState.clear_active();
    }

    ITransformToolSession*
        TransformTool::active_session() {

        return activeSession_;
    }

    const ITransformToolSession*
        TransformTool::active_session() const {

        return activeSession_;
    }

} // namespace locus::editor
