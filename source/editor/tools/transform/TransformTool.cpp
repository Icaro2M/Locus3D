/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/transform/TransformTool.h"

#include "editor/selection/ObjectSelection.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/tools/transform/MeshTransformTargetResolver.h"
#include "editor/transform/TransformPivotResolver.h"

#include <glm/geometric.hpp>

#include <string>

namespace locus::editor {

    namespace {

        constexpr float orientationEpsilon = 0.000001f;

        [[nodiscard]] bool has_object_transform_context(
            const ToolContext& context)
        {
            return context.selection().scope() == SelectionScope::Scene &&
                context.selection().granularity() ==
                SelectionGranularity::Object &&
                !context.selection().objects().empty();
        }

        [[nodiscard]] bool has_mesh_transform_context_impl(
            const ToolContext& context)
        {
            const SelectionGranularity granularity =
                context.selection().granularity();

            if (context.selection().scope() != SelectionScope::ActiveMesh ||
                (granularity != SelectionGranularity::Vertex &&
                    granularity != SelectionGranularity::Edge &&
                    granularity != SelectionGranularity::Face)) {
                return false;
            }

            return MeshTransformTargetResolver::resolve(
                context.scene(),
                context.selection()).success;
        }

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

        objectSession_.controller().state().mode = mode_;
        objectSession_.controller().state().clear_hover();
        meshSession_.controller().state().mode = mode_;
        meshSession_.controller().state().clear_hover();

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

        meshSession_
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

        meshSession_
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

    MeshTransformToolSession&
        TransformTool::mesh_session() {

        return meshSession_;
    }

    const MeshTransformToolSession&
        TransformTool::mesh_session() const {

        return meshSession_;
    }

    const GizmoState&
        TransformTool::gizmo_state() const {

        return presentation_controller()
            .state();
    }

    void TransformTool::refresh_gizmo_state(
        const ToolContext& context) {

        if (state() == ToolState::Interacting) {
            return;
        }

        refresh_gizmo_presentation(context);
    }

    bool TransformTool::can_activate_tool(
        const ToolContext& context) const {

        return has_object_transform_context(context) ||
            has_mesh_transform_context(context);
    }

    ToolResult TransformTool::on_activate(
        ToolContext& context) {

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
        meshSession_.clear();
        presentationSession_ = nullptr;
        activeSession_ = nullptr;

        return ToolResult::consumed(
            EditorDirtyFlags::Render,
            "Transform tool deactivated.");
    }

    ToolResult TransformTool::on_pointer_hover(
        ToolContext& context,
        const ToolEvent& event) {

        const bool objectContext =
            has_object_transform_context(context);
        const bool meshContext =
            has_mesh_transform_context(context);

        refresh_gizmo_presentation(context);

        GizmoController& controller =
            presentation_controller();

        const GizmoHit previous =
            controller.state().hovered;

        if (!objectContext && !meshContext) {

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
            objectContext
            ? resolve_object_pivot(context)
            : resolve_mesh_pivot(context);

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

        if (has_object_transform_context(context)) {
            presentationSession_ = &objectSession_;
            activeSession_ = &objectSession_;
        }
        else if (has_mesh_transform_context(context)) {
            presentationSession_ = &meshSession_;
            activeSession_ = &meshSession_;
        }
        else if (context.selection().scope() == SelectionScope::ActiveMesh) {
            return ToolResult::fail(
                "Mesh transform requires a valid component selection context.");
        }
        else {
            return ToolResult::ignored();
        }

        refresh_gizmo_presentation(context);

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

    bool TransformTool::has_mesh_transform_context(
        const ToolContext& context) const {

        return has_mesh_transform_context_impl(context);
    }

    glm::vec3 TransformTool::resolve_mesh_pivot(
        const ToolContext& context) const {

        const MeshTransformTargetResolveResult result =
            MeshTransformTargetResolver::resolve(
                context.scene(),
                context.selection());

        if (!result.success) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        return result.target.pivot;
    }

    GizmoController& TransformTool::presentation_controller() {
        if (activeSession_ == &meshSession_) {
            return meshSession_.controller();
        }

        if (presentationSession_ == &meshSession_) {
            return meshSession_.controller();
        }

        return objectSession_.controller();
    }

    const GizmoController& TransformTool::presentation_controller() const {
        if (activeSession_ == &meshSession_) {
            return meshSession_.controller();
        }

        if (presentationSession_ == &meshSession_) {
            return meshSession_.controller();
        }

        return objectSession_.controller();
    }

    void TransformTool::refresh_gizmo_presentation(
        const ToolContext& context) {

        const bool objectContext =
            has_object_transform_context(context);
        const bool meshContext =
            has_mesh_transform_context(context);

        presentationSession_ =
            meshContext
            ? static_cast<ITransformToolSession*>(&meshSession_)
            : objectContext
                ? static_cast<ITransformToolSession*>(&objectSession_)
                : nullptr;

        GizmoState& gizmoState =
            presentation_controller().state();

        gizmoState.enabled = true;
        gizmoState.visible =
            objectContext || meshContext;
        gizmoState.mode = mode_;
        gizmoState.space = options_.space;
        gizmoState.orientation = orientation_;

        if (gizmoState.visible) {
            gizmoState.pivot =
                objectContext
                ? resolve_object_pivot(context)
                : resolve_mesh_pivot(context);
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
