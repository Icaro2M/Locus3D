/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/gizmo/GizmoConstraint.h"

#include <algorithm>
#include <cmath>

namespace locus::editor {
    namespace {

        constexpr float epsilon = 0.000001f;

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

        [[nodiscard]] GizmoRay normalized_ray(const GizmoRay& ray)
        {
            GizmoRay result = ray;
            result.direction = safe_normalize(ray.direction, glm::vec3{ 0.0f, 0.0f, -1.0f });
            return result;
        }

        [[nodiscard]] bool intersect_ray_plane(
            const GizmoRay& ray,
            const glm::vec3& planePoint,
            const glm::vec3& planeNormal,
            glm::vec3& outPoint)
        {
            const GizmoRay normalized = normalized_ray(ray);
            const glm::vec3 normal = safe_normalize(planeNormal, glm::vec3{ 0.0f, 1.0f, 0.0f });
            const float denominator = glm::dot(normal, normalized.direction);

            if (std::abs(denominator) <= epsilon) {
                return false;
            }

            const float t = glm::dot(planePoint - normalized.origin, normal) / denominator;
            outPoint = normalized.origin + normalized.direction * t;
            return true;
        }

        [[nodiscard]] bool closest_point_on_axis_from_ray(
            const GizmoRay& ray,
            const glm::vec3& axisOrigin,
            const glm::vec3& axisDirection,
            glm::vec3& outPoint)
        {
            const GizmoRay normalized = normalized_ray(ray);
            const glm::vec3 axis = safe_normalize(axisDirection, glm::vec3{ 1.0f, 0.0f, 0.0f });

            /*
             * Closest points between:
             *   ray line:  R(s) = ray.origin + s * ray.direction
             *   axis line: A(t) = axisOrigin + t * axis
             *
             * This uses w = ray.origin - axisOrigin.
             * The previous implementation used the inverse vector and therefore
             * inverted the signed amount on axis-constrained drags.
             */
            const glm::vec3 w = normalized.origin - axisOrigin;
            const float a = glm::dot(normalized.direction, normalized.direction);
            const float b = glm::dot(normalized.direction, axis);
            const float c = glm::dot(axis, axis);
            const float d = glm::dot(normalized.direction, w);
            const float e = glm::dot(axis, w);
            const float denominator = (a * c) - (b * b);

            if (std::abs(denominator) <= epsilon) {
                return false;
            }

            const float axisT = ((a * e) - (b * d)) / denominator;
            outPoint = axisOrigin + axis * axisT;
            return true;
        }

        [[nodiscard]] bool constrained_point(
            const GizmoConstraintInput& input,
            glm::vec3& outPoint)
        {
            if (is_gizmo_single_axis(input.axis)) {
                return closest_point_on_axis_from_ray(
                    input.currentRay,
                    input.startPoint,
                    GizmoConstraint::axis_vector(input.axis, input.orientation),
                    outPoint);
            }

            const glm::vec3 normal = GizmoConstraint::plane_normal(
                input.axis,
                input.orientation,
                input.viewDirection);

            return intersect_ray_plane(input.currentRay, input.startPoint, normal, outPoint);
        }

        [[nodiscard]] glm::vec3 project_delta_to_plane(
            const glm::vec3& delta,
            GizmoAxis axis,
            const glm::quat& orientation)
        {
            const glm::vec3 x = GizmoConstraint::axis_vector(GizmoAxis::X, orientation);
            const glm::vec3 y = GizmoConstraint::axis_vector(GizmoAxis::Y, orientation);
            const glm::vec3 z = GizmoConstraint::axis_vector(GizmoAxis::Z, orientation);

            switch (axis) {
            case GizmoAxis::XY:
                return x * glm::dot(delta, x) + y * glm::dot(delta, y);
            case GizmoAxis::XZ:
                return x * glm::dot(delta, x) + z * glm::dot(delta, z);
            case GizmoAxis::YZ:
                return y * glm::dot(delta, y) + z * glm::dot(delta, z);
            default:
                return delta;
            }
        }

        [[nodiscard]] float signed_angle_on_plane(
            const glm::vec3& from,
            const glm::vec3& to,
            const glm::vec3& normal)
        {
            const glm::vec3 n = safe_normalize(normal, glm::vec3{ 0.0f, 1.0f, 0.0f });
            const glm::vec3 a = safe_normalize(from - n * glm::dot(from, n), glm::vec3{ 1.0f, 0.0f, 0.0f });
            const glm::vec3 b = safe_normalize(to - n * glm::dot(to, n), a);

            const float sine = glm::dot(glm::cross(a, b), n);
            const float cosine = std::clamp(glm::dot(a, b), -1.0f, 1.0f);
            return std::atan2(sine, cosine);
        }

        [[nodiscard]] float safe_scale_factor(float signedAmount, float sensitivity)
        {
            const float factor = 1.0f + signedAmount * sensitivity;
            return std::max(factor, 0.0001f);
        }

    } // namespace

    glm::vec3 GizmoConstraint::axis_vector(GizmoAxis axis)
    {
        switch (axis) {
        case GizmoAxis::X:
            return glm::vec3{ 1.0f, 0.0f, 0.0f };
        case GizmoAxis::Y:
            return glm::vec3{ 0.0f, 1.0f, 0.0f };
        case GizmoAxis::Z:
            return glm::vec3{ 0.0f, 0.0f, 1.0f };
        default:
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }
    }

    glm::vec3 GizmoConstraint::axis_vector(
        GizmoAxis axis,
        const glm::quat& orientation)
    {
        const glm::vec3 base = axis_vector(axis);
        if (glm::length(base) <= epsilon) {
            return base;
        }

        return safe_normalize(orientation * base, base);
    }

    glm::vec3 GizmoConstraint::plane_normal(
        GizmoAxis axis,
        const glm::vec3& viewDirection)
    {
        switch (axis) {
        case GizmoAxis::XY:
            return glm::vec3{ 0.0f, 0.0f, 1.0f };
        case GizmoAxis::XZ:
            return glm::vec3{ 0.0f, 1.0f, 0.0f };
        case GizmoAxis::YZ:
            return glm::vec3{ 1.0f, 0.0f, 0.0f };
        case GizmoAxis::View:
        case GizmoAxis::XYZ:
        default:
            return safe_normalize(-viewDirection, glm::vec3{ 0.0f, 0.0f, 1.0f });
        }
    }

    glm::vec3 GizmoConstraint::plane_normal(
        GizmoAxis axis,
        const glm::quat& orientation,
        const glm::vec3& viewDirection)
    {
        switch (axis) {
        case GizmoAxis::XY:
            return axis_vector(GizmoAxis::Z, orientation);
        case GizmoAxis::XZ:
            return axis_vector(GizmoAxis::Y, orientation);
        case GizmoAxis::YZ:
            return axis_vector(GizmoAxis::X, orientation);
        case GizmoAxis::View:
        case GizmoAxis::XYZ:
        default:
            return safe_normalize(-viewDirection, glm::vec3{ 0.0f, 0.0f, 1.0f });
        }
    }

    GizmoConstraintResult GizmoConstraint::solve_translation(
        const GizmoConstraintInput& input)
    {
        if (input.axis == GizmoAxis::None) {
            return GizmoConstraintResult::none();
        }

        glm::vec3 currentPoint{};
        if (!constrained_point(input, currentPoint)) {
            return GizmoConstraintResult::none();
        }

        glm::vec3 delta = currentPoint - input.startPoint;
        float amount = glm::length(delta);

        if (is_gizmo_single_axis(input.axis)) {
            const glm::vec3 axis = axis_vector(input.axis, input.orientation);
            amount = glm::dot(delta, axis);
            delta = axis * amount;
        }
        else if (is_gizmo_plane_axis(input.axis)) {
            delta = project_delta_to_plane(delta, input.axis, input.orientation);
            amount = glm::length(delta);
        }

        GizmoConstraintResult result{};
        result.valid = true;
        result.translation = delta;
        result.signedAmount = amount;
        result.constrainedPoint = input.startPoint + delta;
        return result;
    }

    GizmoConstraintResult GizmoConstraint::solve_scale(
        const GizmoConstraintInput& input)
    {
        if (input.axis == GizmoAxis::None) {
            return GizmoConstraintResult::none();
        }

        glm::vec3 currentPoint{};
        if (!constrained_point(input, currentPoint)) {
            return GizmoConstraintResult::none();
        }

        const glm::vec3 delta = currentPoint - input.startPoint;
        float amount = glm::length(delta);

        if (is_gizmo_single_axis(input.axis)) {
            const glm::vec3 axis = axis_vector(input.axis, input.orientation);
            amount = glm::dot(delta, axis);
        }
        else {
            const glm::vec3 viewRight = safe_normalize(input.viewRight, glm::vec3{ 1.0f, 0.0f, 0.0f });
            const glm::vec3 viewUp = safe_normalize(input.viewUp, glm::vec3{ 0.0f, 1.0f, 0.0f });
            amount = glm::dot(delta, viewRight + viewUp) * 0.5f;
        }

        const float factor = safe_scale_factor(amount, input.scaleSensitivity);

        GizmoConstraintResult result{};
        result.valid = true;
        result.scale = glm::vec3{ 1.0f, 1.0f, 1.0f };
        result.signedAmount = amount;
        result.constrainedPoint = currentPoint;

        switch (input.axis) {
        case GizmoAxis::X:
            result.scale.x = factor;
            break;
        case GizmoAxis::Y:
            result.scale.y = factor;
            break;
        case GizmoAxis::Z:
            result.scale.z = factor;
            break;
        case GizmoAxis::XY:
            result.scale.x = factor;
            result.scale.y = factor;
            break;
        case GizmoAxis::XZ:
            result.scale.x = factor;
            result.scale.z = factor;
            break;
        case GizmoAxis::YZ:
            result.scale.y = factor;
            result.scale.z = factor;
            break;
        case GizmoAxis::XYZ:
        case GizmoAxis::View:
            result.scale = glm::vec3{ factor, factor, factor };
            break;
        default:
            return GizmoConstraintResult::none();
        }

        return result;
    }

    GizmoConstraintResult GizmoConstraint::solve_rotation(
        const GizmoConstraintInput& input)
    {
        if (input.axis == GizmoAxis::None) {
            return GizmoConstraintResult::none();
        }

        const glm::vec3 normal = is_gizmo_single_axis(input.axis)
            ? axis_vector(input.axis, input.orientation)
            : plane_normal(input.axis, input.orientation, input.viewDirection);

        glm::vec3 currentPoint{};
        if (!intersect_ray_plane(input.currentRay, input.pivot, normal, currentPoint)) {
            return GizmoConstraintResult::none();
        }

        const glm::vec3 from = input.startPoint - input.pivot;
        const glm::vec3 to = currentPoint - input.pivot;

        if (glm::length(from) <= epsilon || glm::length(to) <= epsilon) {
            return GizmoConstraintResult::none();
        }

        const float angle = signed_angle_on_plane(from, to, normal) * input.rotationSensitivity;

        GizmoConstraintResult result{};
        result.valid = true;
        result.rotation = glm::angleAxis(angle, safe_normalize(normal, glm::vec3{ 0.0f, 1.0f, 0.0f }));
        result.angle = angle;
        result.signedAmount = angle;
        result.constrainedPoint = currentPoint;
        return result;
    }

} // namespace locus::editor