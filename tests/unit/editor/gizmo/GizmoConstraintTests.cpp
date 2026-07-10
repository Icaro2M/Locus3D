/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GizmoTestSuite.h"

#include "editor/gizmo/GizmoConstraint.h"

#include <cmath>

#include <glm/gtc/quaternion.hpp>

namespace {

constexpr float epsilon = 0.0001f;
constexpr float pi = 3.14159265358979323846f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

} // namespace

namespace locus::tests {

TestResult run_gizmo_constraint_tests()
{
    using namespace editor;

    if (!near_vec3(GizmoConstraint::axis_vector(GizmoAxis::X), glm::vec3{ 1.0f, 0.0f, 0.0f }) ||
        !near_vec3(GizmoConstraint::axis_vector(GizmoAxis::Y), glm::vec3{ 0.0f, 1.0f, 0.0f }) ||
        !near_vec3(GizmoConstraint::axis_vector(GizmoAxis::Z), glm::vec3{ 0.0f, 0.0f, 1.0f }) ||
        !near_vec3(GizmoConstraint::axis_vector(GizmoAxis::XY), glm::vec3{ 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("axis_vector should return unit vectors only for single-axis handles");
    }

    const glm::quat quarterTurnZ = glm::angleAxis(pi * 0.5f, glm::vec3{ 0.0f, 0.0f, 1.0f });
    if (!near_vec3(
            GizmoConstraint::axis_vector(GizmoAxis::X, quarterTurnZ),
            glm::vec3{ 0.0f, 1.0f, 0.0f }) ||
        !near_vec3(
            GizmoConstraint::plane_normal(GizmoAxis::XY, quarterTurnZ, glm::vec3{ 0.0f, 0.0f, -1.0f }),
            glm::vec3{ 0.0f, 0.0f, 1.0f })) {
        return TestResult::fail("oriented axis and plane helpers should apply gizmo orientation");
    }

    if (!near_vec3(
            GizmoConstraint::plane_normal(GizmoAxis::View, glm::vec3{ 0.0f, 0.0f, -2.0f }),
            glm::vec3{ 0.0f, 0.0f, 1.0f })) {
        return TestResult::fail("view plane normal should use normalized inverse view direction");
    }

    GizmoConstraintInput input;
    input.axis = GizmoAxis::X;
    input.startPoint = glm::vec3{ 0.0f, 0.0f, 0.0f };
    input.currentRay.origin = glm::vec3{ 2.0f, 0.0f, 1.0f };
    input.currentRay.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };

    const GizmoConstraintResult translation = GizmoConstraint::solve_translation(input);
    if (!translation.is_valid() ||
        !near_vec3(translation.translation, glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
        !near(translation.signedAmount, 2.0f) ||
        !near_vec3(translation.constrainedPoint, glm::vec3{ 2.0f, 0.0f, 0.0f })) {
        return TestResult::fail("axis translation should project the drag onto the active axis");
    }

    input.axis = GizmoAxis::XY;
    input.startPoint = glm::vec3{ 1.0f, 1.0f, 0.0f };
    input.currentRay.origin = glm::vec3{ 3.0f, 4.0f, 5.0f };
    input.currentRay.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    const GizmoConstraintResult planeTranslation = GizmoConstraint::solve_translation(input);
    if (!planeTranslation.is_valid() ||
        !near_vec3(planeTranslation.translation, glm::vec3{ 2.0f, 3.0f, 0.0f }) ||
        !near_vec3(planeTranslation.constrainedPoint, glm::vec3{ 3.0f, 4.0f, 0.0f })) {
        return TestResult::fail("plane translation should constrain movement to the selected plane");
    }

    input.axis = GizmoAxis::Y;
    input.startPoint = glm::vec3{ 0.0f, 0.0f, 0.0f };
    input.currentRay.origin = glm::vec3{ 0.0f, 2.0f, 1.0f };
    input.currentRay.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    input.scaleSensitivity = 0.5f;
    const GizmoConstraintResult scale = GizmoConstraint::solve_scale(input);
    if (!scale.is_valid() ||
        !near_vec3(scale.scale, glm::vec3{ 1.0f, 2.0f, 1.0f }) ||
        !near(scale.signedAmount, 2.0f)) {
        return TestResult::fail("axis scale should affect only the selected scale component");
    }

    input.axis = GizmoAxis::Z;
    input.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
    input.startPoint = glm::vec3{ 1.0f, 0.0f, 0.0f };
    input.currentRay.origin = glm::vec3{ 0.0f, 1.0f, 4.0f };
    input.currentRay.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    input.rotationSensitivity = 1.0f;
    const GizmoConstraintResult rotation = GizmoConstraint::solve_rotation(input);
    if (!rotation.is_valid() ||
        !near(rotation.angle, pi * 0.5f) ||
        !near_vec3(rotation.constrainedPoint, glm::vec3{ 0.0f, 1.0f, 0.0f })) {
        return TestResult::fail("axis rotation should solve signed angle on the axis plane");
    }

    input.axis = GizmoAxis::None;
    if (GizmoConstraint::solve_translation(input).is_valid() ||
        GizmoConstraint::solve_scale(input).is_valid() ||
        GizmoConstraint::solve_rotation(input).is_valid()) {
        return TestResult::fail("constraint solvers should reject empty handles");
    }

    return TestResult::pass();
}

} // namespace locus::tests
