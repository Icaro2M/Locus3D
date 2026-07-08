/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/gizmo/GizmoSnap.h"

#include "editor/snapping/SnapSolver.h"

#include <cmath>

namespace locus::editor {
    namespace {

        [[nodiscard]] float snap_scalar(float value, float increment)
        {
            if (increment <= 0.0f) {
                return value;
            }

            return std::round(value / increment) * increment;
        }

    } // namespace

    GizmoSnapResult GizmoSnap::snap_position(const GizmoSnapRequest& request)
    {
        GizmoSnapResult result{};
        result.snappedPosition = request.candidatePosition;
        result.delta = request.candidatePosition - request.originalPosition;

        if (!request.settings || !request.solver || !request.settings->snapping_enabled()) {
            return result;
        }

        SnapContext context{};
        context.scene = request.scene;
        context.originalPosition = request.originalPosition;
        context.candidatePosition = request.candidatePosition;
        context.referenceOrigin = request.referenceOrigin;
        context.viewDirection = request.viewDirection;
        context.activeNode = request.activeNode;
        context.space = request.space;
        context.maxDistanceOverride = request.maxDistanceOverride;

        const SnapResult snap = request.solver->solve(*request.settings, context);
        if (!snap.is_valid()) {
            return result;
        }

        result.snapped = true;
        result.snappedPosition = snap.snappedPosition;
        result.delta = snap.snappedPosition - request.originalPosition;
        result.source = snap;
        return result;
    }

    GizmoSnapResult GizmoSnap::snap_translation(
        const GizmoSnapRequest& request,
        const GizmoConstraintResult& constraint)
    {
        GizmoSnapRequest translatedRequest = request;
        translatedRequest.candidatePosition = request.originalPosition + constraint.translation;
        return snap_position(translatedRequest);
    }

    float GizmoSnap::snap_angle(float radians, const SnapSettings& settings)
    {
        if (!settings.snapping_enabled() || !settings.is_enabled(SnapMode::Angle)) {
            return radians;
        }

        return snap_scalar(radians, settings.angle_increment());
    }

} // namespace locus::editor