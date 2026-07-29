/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/gizmo/GizmoController.h"

#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionState.h"
#include "editor/snapping/SnapSettings.h"

#include <algorithm>
#include <cmath>

namespace locus::editor {
    namespace {

        constexpr float epsilon = 0.000001f;

        [[nodiscard]] glm::vec3 safe_divide_scale(
            const glm::vec3& current,
            const glm::vec3& previous)
        {
            glm::vec3 result{ 1.0f, 1.0f, 1.0f };

            result.x = std::abs(previous.x) <= epsilon ? 1.0f : current.x / previous.x;
            result.y = std::abs(previous.y) <= epsilon ? 1.0f : current.y / previous.y;
            result.z = std::abs(previous.z) <= epsilon ? 1.0f : current.z / previous.z;

            return result;
        }

        [[nodiscard]] glm::vec3 safe_normalize(
            const glm::vec3& value,
            const glm::vec3& fallback)
        {
            const float length = glm::length(value);
            if (length <= epsilon) {
                return fallback;
            }

            return value / length;
        }

        [[nodiscard]] glm::vec3 rotation_axis_for_hit(
            const GizmoHit& hit,
            const glm::quat& orientation,
            const glm::vec3& viewDirection)
        {
            if (is_gizmo_single_axis(hit.axis)) {
                return GizmoConstraint::axis_vector(hit.axis, orientation);
            }

            return GizmoConstraint::plane_normal(hit.axis, orientation, viewDirection);
        }

    } // namespace

    GizmoController::GizmoController(const TransformGizmo& gizmo)
        : gizmo_(gizmo)
    {
    }

    const GizmoState& GizmoController::state() const
    {
        return state_;
    }

    GizmoState& GizmoController::state()
    {
        return state_;
    }

    const TransformSession& GizmoController::session() const
    {
        return session_;
    }

    TransformSession& GizmoController::session()
    {
        return session_;
    }

    const TransformGizmo& GizmoController::gizmo() const
    {
        return gizmo_;
    }

    TransformGizmo& GizmoController::gizmo()
    {
        return gizmo_;
    }

    GizmoHit GizmoController::update_hover(const GizmoHoverInput& input)
    {
        state_.mode = input.mode;
        state_.pivot = input.pivot;
        state_.orientation = input.orientation;
        state_.visualScale = input.pointer.visualScale;

        if (!state_.can_interact() || state_.dragging || input.mode == GizmoMode::None) {
            state_.clear_hover();
            return state_.hovered;
        }

        TransformGizmoHitTestInput hitInput{};
        hitInput.mode = input.mode;
        hitInput.pivot = input.pivot;
        hitInput.orientation = input.orientation;
        hitInput.ray = input.pointer.ray;
        hitInput.viewDirection = input.pointer.viewDirection;
        hitInput.visualScale = input.pointer.visualScale;

        state_.hovered = gizmo_.hit_test(hitInput);
        return state_.hovered;
    }

    GizmoControllerResult GizmoController::begin_drag(const GizmoBeginDragInput& input)
    {
        if (!input.scene) {
            return { false, false, {}, {}, "Missing scene." };
        }

        if (!input.selection) {
            return { false, false, {}, {}, "Missing selection." };
        }

        if (!state_.can_interact() || input.mode == GizmoMode::None) {
            return { false, false, {}, {}, "Gizmo is not interactive." };
        }

        TransformGizmoHitTestInput hitInput{};
        hitInput.mode = input.mode;
        hitInput.pivot = state_.pivot;
        hitInput.orientation = input.orientation;
        hitInput.ray = input.pointer.ray;
        hitInput.viewDirection = input.pointer.viewDirection;
        hitInput.visualScale = input.pointer.visualScale;

        const GizmoHit hit = gizmo_.hit_test(hitInput);
        return begin_drag_from_hit(*input.scene, hit, input);
    }

    GizmoControllerResult GizmoController::begin_drag(const GizmoBeginDragTargetsInput& input)
    {
        if (!input.scene) {
            return { false, false, {}, {}, "Missing scene." };
        }

        if (input.targets.empty()) {
            return { false, false, {}, {}, "Missing transform targets." };
        }

        if (!state_.can_interact() || input.mode == GizmoMode::None) {
            return { false, false, {}, {}, "Gizmo is not interactive." };
        }

        TransformGizmoHitTestInput hitInput{};
        hitInput.mode = input.mode;
        hitInput.pivot = state_.pivot;
        hitInput.orientation = input.orientation;
        hitInput.ray = input.pointer.ray;
        hitInput.viewDirection = input.pointer.viewDirection;
        hitInput.visualScale = input.pointer.visualScale;

        const GizmoHit hit = gizmo_.hit_test(hitInput);
        return begin_drag_from_hit(*input.scene, hit, input);
    }

    GizmoControllerResult GizmoController::begin_drag_from_hit(
        EditorScene& scene,
        const GizmoHit& hit,
        const GizmoBeginDragInput& input)
    {
        if (!hit.is_valid()) {
            return { false, false, hit, {}, "No gizmo handle was hit." };
        }

        if (!session_.begin(scene, *input.selection, input.sessionOptions)) {
            return { false, false, hit, {}, "Could not start transform session." };
        }

        state_.mode = hit.mode;
        state_.pivot = session_.pivot();
        state_.orientation = input.orientation;
        state_.visualScale = input.pointer.visualScale;
        state_.active = hit;
        state_.hovered = hit;
        state_.dragging = true;

        startRay_ = input.pointer.ray;
        startPoint_ = hit.worldPosition;
        startPivot_ = session_.pivot();

        activeSnapSettings_ = input.snapSettings;
        activeSnapSolver_ = input.snapSolver;

        reset_incremental_state();

        return { true, false, hit, {}, "Gizmo drag started." };
    }

    GizmoControllerResult GizmoController::begin_drag_from_hit(
        EditorScene& scene,
        const GizmoHit& hit,
        const GizmoBeginDragTargetsInput& input)
    {
        if (!hit.is_valid()) {
            return { false, false, hit, {}, "No gizmo handle was hit." };
        }

        if (!session_.begin(scene, input.targets, input.active, input.sessionOptions)) {
            return { false, false, hit, {}, "Could not start transform session." };
        }

        state_.mode = hit.mode;
        state_.pivot = session_.pivot();
        state_.orientation = input.orientation;
        state_.visualScale = input.pointer.visualScale;
        state_.active = hit;
        state_.hovered = hit;
        state_.dragging = true;

        startRay_ = input.pointer.ray;
        startPoint_ = hit.worldPosition;
        startPivot_ = session_.pivot();

        activeSnapSettings_ = input.snapSettings;
        activeSnapSolver_ = input.snapSolver;

        reset_incremental_state();

        return { true, false, hit, {}, "Gizmo drag started." };
    }

    GizmoControllerResult GizmoController::update_drag(
        EditorScene& scene,
        const GizmoDragInput& input)
    {
        if (!state_.dragging || !state_.active.is_valid() || !session_.is_active()) {
            return { false, false, state_.active, {}, "No active gizmo drag." };
        }

        const GizmoConstraintResult constraint = solve_constraint(input.pointer);
        if (!constraint.is_valid()) {
            return { false, false, state_.active, constraint, "Could not solve gizmo constraint." };
        }

        const SnapSettings* snapSettings = input.snapSettings ? input.snapSettings : activeSnapSettings_;
        const SnapSolver* snapSolver = input.snapSolver ? input.snapSolver : activeSnapSolver_;

        return apply_constraint(scene, constraint, snapSettings, snapSolver);
    }

    bool GizmoController::end_drag()
    {
        if (!state_.dragging) {
            return false;
        }

        const bool confirmed = session_.confirm();

        state_.clear_active();
        reset_incremental_state();
        activeSnapSettings_ = nullptr;
        activeSnapSolver_ = nullptr;

        return confirmed;
    }

    bool GizmoController::cancel_drag(EditorScene& scene)
    {
        if (!state_.dragging && !session_.is_active()) {
            return false;
        }

        const bool restored = session_.cancel(scene);

        state_.clear_active();
        reset_incremental_state();
        activeSnapSettings_ = nullptr;
        activeSnapSolver_ = nullptr;

        return restored;
    }

    void GizmoController::clear()
    {
        state_ = GizmoState{};
        session_.clear();
        startRay_ = {};
        startPoint_ = glm::vec3{ 0.0f, 0.0f, 0.0f };
        startPivot_ = glm::vec3{ 0.0f, 0.0f, 0.0f };
        activeSnapSettings_ = nullptr;
        activeSnapSolver_ = nullptr;
        reset_incremental_state();
    }

    GizmoConstraintInput GizmoController::make_constraint_input(
        const GizmoPointerInput& pointer) const
    {
        GizmoConstraintInput input{};
        input.mode = state_.active.mode;
        input.axis = state_.active.axis;
        input.pivot = startPivot_;
        input.orientation = state_.orientation;
        input.startPoint = startPoint_;
        input.startRay = startRay_;
        input.currentRay = pointer.ray;
        input.viewDirection = pointer.viewDirection;
        input.viewRight = pointer.viewRight;
        input.viewUp = pointer.viewUp;
        return input;
    }

    GizmoConstraintResult GizmoController::solve_constraint(
        const GizmoPointerInput& pointer) const
    {
        const GizmoConstraintInput input = make_constraint_input(pointer);

        switch (state_.active.mode) {
        case GizmoMode::Translate:
            return GizmoConstraint::solve_translation(input);
        case GizmoMode::Rotate:
            return GizmoConstraint::solve_rotation(input);
        case GizmoMode::Scale:
            return GizmoConstraint::solve_scale(input);
        case GizmoMode::Universal:
        case GizmoMode::None:
        default:
            return GizmoConstraintResult::none();
        }
    }

    GizmoControllerResult GizmoController::apply_constraint(
        EditorScene& scene,
        const GizmoConstraintResult& constraint,
        const SnapSettings* snapSettings,
        const SnapSolver* snapSolver)
    {
        bool changed = false;

        switch (state_.active.mode) {
        case GizmoMode::Translate: {
            glm::vec3 absoluteTranslation = constraint.translation;

            if (snapSettings && snapSolver) {
                GizmoSnapRequest request{};
                request.scene = &scene;
                request.settings = snapSettings;
                request.solver = snapSolver;
                request.originalPosition = startPivot_;
                request.candidatePosition = startPivot_ + constraint.translation;
                request.referenceOrigin = startPivot_;
                request.viewDirection = glm::vec3{ 0.0f, 0.0f, -1.0f };
                request.activeNode = {};
                request.space = session_.space();

                const GizmoSnapResult snap = GizmoSnap::snap_position(request);
                absoluteTranslation = snap.delta;
            }

            const glm::vec3 incremental = absoluteTranslation - lastTranslation_;
            changed = session_.translate(scene, incremental);
            lastTranslation_ = absoluteTranslation;

            if (changed) {
                state_.pivot = startPivot_ + lastTranslation_;
            }

            break;
        }

        case GizmoMode::Rotate: {
            glm::quat absoluteRotation = constraint.rotation;

            if (snapSettings) {
                const float snappedAngle = GizmoSnap::snap_angle(constraint.angle, *snapSettings);
                const glm::vec3 axis = safe_normalize(
                    rotation_axis_for_hit(state_.active, state_.orientation, glm::vec3{ 0.0f, 0.0f, -1.0f }),
                    glm::vec3{ 0.0f, 1.0f, 0.0f });

                absoluteRotation = glm::angleAxis(snappedAngle, axis);
            }

            const glm::quat incremental = glm::normalize(absoluteRotation * glm::inverse(lastRotation_));
            changed = session_.rotate(scene, incremental);
            lastRotation_ = absoluteRotation;
            break;
        }

        case GizmoMode::Scale: {
            const glm::vec3 absoluteScale = constraint.scale;
            const glm::vec3 incremental = safe_divide_scale(absoluteScale, lastScale_);

            changed = session_.scale(scene, incremental);
            lastScale_ = absoluteScale;
            break;
        }

        case GizmoMode::Universal:
        case GizmoMode::None:
        default:
            return { false, false, state_.active, constraint, "Unsupported gizmo mode." };
        }

        return {
            true,
            changed,
            state_.active,
            constraint,
            changed ? "Gizmo drag updated." : "Gizmo drag produced no scene change."
        };
    }

    void GizmoController::reset_incremental_state()
    {
        lastTranslation_ = glm::vec3{ 0.0f, 0.0f, 0.0f };
        lastScale_ = glm::vec3{ 1.0f, 1.0f, 1.0f };
        lastRotation_ = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
    }

} // namespace locus::editor
