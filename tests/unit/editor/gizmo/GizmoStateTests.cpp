/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GizmoTestSuite.h"

#include "editor/gizmo/GizmoAxis.h"
#include "editor/gizmo/GizmoHit.h"
#include "editor/gizmo/GizmoState.h"

namespace locus::tests {

TestResult run_gizmo_state_tests()
{
    using namespace editor;

    if (!is_gizmo_single_axis(GizmoAxis::X) ||
        !is_gizmo_single_axis(GizmoAxis::Y) ||
        !is_gizmo_single_axis(GizmoAxis::Z) ||
        is_gizmo_single_axis(GizmoAxis::XY) ||
        is_gizmo_single_axis(GizmoAxis::XYZ)) {
        return TestResult::fail("single-axis helper should classify only X, Y, and Z");
    }

    if (!is_gizmo_plane_axis(GizmoAxis::XY) ||
        !is_gizmo_plane_axis(GizmoAxis::XZ) ||
        !is_gizmo_plane_axis(GizmoAxis::YZ) ||
        is_gizmo_plane_axis(GizmoAxis::View) ||
        is_gizmo_plane_axis(GizmoAxis::None)) {
        return TestResult::fail("plane-axis helper should classify only transform planes");
    }

    if (!is_gizmo_free_axis(GizmoAxis::XYZ) ||
        is_gizmo_free_axis(GizmoAxis::View) ||
        is_gizmo_free_axis(GizmoAxis::X)) {
        return TestResult::fail("free-axis helper should classify only XYZ");
    }

    const GizmoHit none = GizmoHit::none();
    const GizmoHit hit = GizmoHit::make(
        GizmoMode::Translate,
        GizmoAxis::Y,
        glm::vec3{ 1.0f, 2.0f, 3.0f },
        0.25f,
        4.0f);
    if (none.is_valid() ||
        !hit.is_valid() ||
        hit.mode != GizmoMode::Translate ||
        hit.axis != GizmoAxis::Y ||
        hit.worldPosition != glm::vec3{ 1.0f, 2.0f, 3.0f } ||
        hit.distance != 0.25f ||
        hit.depth != 4.0f) {
        return TestResult::fail("GizmoHit factories should populate valid and invalid hits");
    }

    GizmoState state;
    if (!state.can_interact() ||
        !state.enabled ||
        !state.visible ||
        state.dragging ||
        state.mode != GizmoMode::Translate) {
        return TestResult::fail("GizmoState defaults should be interactive and idle");
    }

    state.hovered = hit;
    state.active = hit;
    state.dragging = true;
    state.clear_hover();
    if (state.hovered.is_valid()) {
        return TestResult::fail("clear_hover should reset only hovered hit state");
    }

    state.clear_active();
    if (state.active.is_valid() || state.dragging) {
        return TestResult::fail("clear_active should reset active hit and dragging flag");
    }

    state.enabled = false;
    if (state.can_interact()) {
        return TestResult::fail("disabled GizmoState should not be interactive");
    }

    state.enabled = true;
    state.visible = false;
    if (state.can_interact()) {
        return TestResult::fail("hidden GizmoState should not be interactive");
    }

    return TestResult::pass();
}

} // namespace locus::tests
