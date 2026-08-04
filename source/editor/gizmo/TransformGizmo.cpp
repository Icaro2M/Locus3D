/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/gizmo/TransformGizmo.h"

#include <algorithm>
#include <cmath>

namespace locus::editor {
    namespace {

        constexpr float epsilon = 0.000001f;

        struct CandidateHit {
            bool valid = false;
            GizmoMode mode = GizmoMode::None;
            GizmoAxis axis = GizmoAxis::None;
            glm::vec3 point{ 0.0f, 0.0f, 0.0f };
            float distance = 0.0f;
            float depth = 0.0f;
        };

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

        [[nodiscard]] float hit_depth(const GizmoRay& ray, const glm::vec3& point)
        {
            const GizmoRay normalized = normalized_ray(ray);
            return glm::dot(point - normalized.origin, normalized.direction);
        }

        [[nodiscard]] bool is_better_hit(const CandidateHit& candidate, const CandidateHit& best)
        {
            if (!candidate.valid) {
                return false;
            }

            if (!best.valid) {
                return true;
            }

            if (std::abs(candidate.distance - best.distance) > 0.0001f) {
                return candidate.distance < best.distance;
            }

            return candidate.depth < best.depth;
        }

        void accept_hit(CandidateHit& best, const CandidateHit& candidate)
        {
            if (is_better_hit(candidate, best)) {
                best = candidate;
            }
        }

        [[nodiscard]] GizmoHit to_gizmo_hit(const CandidateHit& candidate)
        {
            if (!candidate.valid) {
                return GizmoHit::none();
            }

            return GizmoHit::make(
                candidate.mode,
                candidate.axis,
                candidate.point,
                candidate.distance,
                candidate.depth);
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
            if (t < 0.0f) {
                return false;
            }

            outPoint = normalized.origin + normalized.direction * t;
            return true;
        }

        [[nodiscard]] CandidateHit hit_center(
            const TransformGizmoHitTestInput& input,
            GizmoMode mode,
            float radius)
        {
            const GizmoRay ray = normalized_ray(input.ray);
            const float t = std::max(0.0f, glm::dot(input.pivot - ray.origin, ray.direction));
            const glm::vec3 closest = ray.origin + ray.direction * t;
            const float distance = glm::length(closest - input.pivot);

            if (distance > radius) {
                return {};
            }

            CandidateHit hit{};
            hit.valid = true;
            hit.mode = mode;
            hit.axis = GizmoAxis::XYZ;
            hit.point = input.pivot;
            hit.distance = distance;
            hit.depth = hit_depth(ray, input.pivot);
            return hit;
        }

        [[nodiscard]] CandidateHit hit_sphere(
            const TransformGizmoHitTestInput& input,
            GizmoMode mode,
            GizmoAxis axis,
            const glm::vec3& center,
            float radius)
        {
            const GizmoRay ray = normalized_ray(input.ray);
            const float t = std::max(0.0f, glm::dot(center - ray.origin, ray.direction));
            const glm::vec3 closest = ray.origin + ray.direction * t;
            const float distance = glm::length(closest - center);

            if (distance > radius) {
                return {};
            }

            CandidateHit hit{};
            hit.valid = true;
            hit.mode = mode;
            hit.axis = axis;
            hit.point = center;
            hit.distance = distance;
            hit.depth = hit_depth(ray, center);
            return hit;
        }

        [[nodiscard]] CandidateHit hit_axis_segment(
            const TransformGizmoHitTestInput& input,
            GizmoMode mode,
            GizmoAxis axis,
            float length,
            float thickness)
        {
            const GizmoRay ray = normalized_ray(input.ray);
            const glm::vec3 axisDirection = GizmoConstraint::axis_vector(axis, input.orientation);
            const glm::vec3 a = input.pivot;
            const glm::vec3 b = input.pivot + axisDirection * length;
            const glm::vec3 segment = b - a;

            const float segmentLengthSq = glm::dot(segment, segment);
            if (segmentLengthSq <= epsilon) {
                return {};
            }

            const glm::vec3 w = ray.origin - a;
            const float aCoeff = glm::dot(ray.direction, ray.direction);
            const float bCoeff = glm::dot(ray.direction, segment);
            const float cCoeff = segmentLengthSq;
            const float dCoeff = glm::dot(ray.direction, w);
            const float eCoeff = glm::dot(segment, w);
            const float denominator = (aCoeff * cCoeff) - (bCoeff * bCoeff);

            float rayT = 0.0f;
            float segmentT = 0.0f;

            if (std::abs(denominator) <= epsilon) {
                segmentT = std::clamp(eCoeff / cCoeff, 0.0f, 1.0f);
                rayT = std::max(0.0f, -dCoeff / aCoeff);
            }
            else {
                rayT = ((bCoeff * eCoeff) - (cCoeff * dCoeff)) / denominator;
                segmentT = ((aCoeff * eCoeff) - (bCoeff * dCoeff)) / denominator;

                rayT = std::max(0.0f, rayT);
                segmentT = std::clamp(segmentT, 0.0f, 1.0f);
            }

            const glm::vec3 pointOnRay = ray.origin + ray.direction * rayT;
            const glm::vec3 pointOnSegment = a + segment * segmentT;
            const float distance = glm::length(pointOnRay - pointOnSegment);

            if (distance > thickness) {
                return {};
            }

            CandidateHit hit{};
            hit.valid = true;
            hit.mode = mode;
            hit.axis = axis;
            hit.point = pointOnSegment;
            hit.distance = distance;
            hit.depth = hit_depth(ray, pointOnSegment);
            return hit;
        }

        [[nodiscard]] CandidateHit hit_plane_square(
            const TransformGizmoHitTestInput& input,
            GizmoMode mode,
            GizmoAxis axis,
            float offset,
            float size,
            float padding)
        {
            const glm::vec3 x = GizmoConstraint::axis_vector(GizmoAxis::X, input.orientation);
            const glm::vec3 y = GizmoConstraint::axis_vector(GizmoAxis::Y, input.orientation);
            const glm::vec3 z = GizmoConstraint::axis_vector(GizmoAxis::Z, input.orientation);

            glm::vec3 u{ 1.0f, 0.0f, 0.0f };
            glm::vec3 v{ 0.0f, 1.0f, 0.0f };
            glm::vec3 normal{ 0.0f, 0.0f, 1.0f };

            switch (axis) {
            case GizmoAxis::XY:
                u = x;
                v = y;
                normal = z;
                break;
            case GizmoAxis::XZ:
                u = x;
                v = z;
                normal = y;
                break;
            case GizmoAxis::YZ:
                u = y;
                v = z;
                normal = x;
                break;
            default:
                return {};
            }

            glm::vec3 point{};
            if (!intersect_ray_plane(input.ray, input.pivot, normal, point)) {
                return {};
            }

            const glm::vec3 local = point - input.pivot;
            const float du = glm::dot(local, u);
            const float dv = glm::dot(local, v);

            if (du < offset - padding ||
                dv < offset - padding ||
                du > offset + size + padding ||
                dv > offset + size + padding) {
                return {};
            }

            const float centerU = offset + size * 0.5f;
            const float centerV = offset + size * 0.5f;
            const glm::vec3 center = input.pivot + u * centerU + v * centerV;

            CandidateHit hit{};
            hit.valid = true;
            hit.mode = mode;
            hit.axis = axis;
            hit.point = point;
            hit.distance = glm::length(point - center);
            hit.depth = hit_depth(input.ray, point);
            return hit;
        }

        [[nodiscard]] CandidateHit hit_rotation_ring(
            const TransformGizmoHitTestInput& input,
            GizmoAxis axis,
            float radius,
            float thickness)
        {
            glm::vec3 normal{ 0.0f, 0.0f, 1.0f };

            if (axis == GizmoAxis::View) {
                normal = safe_normalize(-input.viewDirection, glm::vec3{ 0.0f, 0.0f, 1.0f });
            }
            else if (is_gizmo_single_axis(axis)) {
                normal = GizmoConstraint::axis_vector(axis, input.orientation);
            }
            else {
                return {};
            }

            glm::vec3 point{};
            if (!intersect_ray_plane(input.ray, input.pivot, normal, point)) {
                return {};
            }

            const float radialDistance = glm::length(point - input.pivot);
            const float distance = std::abs(radialDistance - radius);

            if (distance > thickness) {
                return {};
            }

            CandidateHit hit{};
            hit.valid = true;
            hit.mode = GizmoMode::Rotate;
            hit.axis = axis;
            hit.point = point;
            hit.distance = distance;
            hit.depth = hit_depth(input.ray, point);
            return hit;
        }

        [[nodiscard]] CandidateHit hit_translate_axis(
            const TransformGizmoHitTestInput& input,
            GizmoAxis axis,
            float length,
            float shaftLength,
            float shaftThickness,
            float tipRadius)
        {
            CandidateHit best{};
            accept_hit(
                best,
                hit_axis_segment(
                    input,
                    GizmoMode::Translate,
                    axis,
                    shaftLength,
                    shaftThickness));

            const glm::vec3 axisDirection =
                GizmoConstraint::axis_vector(axis, input.orientation);
            accept_hit(
                best,
                hit_sphere(
                    input,
                    GizmoMode::Translate,
                    axis,
                    input.pivot + axisDirection * length,
                    tipRadius));

            return best;
        }

        [[nodiscard]] CandidateHit hit_scale_axis(
            const TransformGizmoHitTestInput& input,
            GizmoAxis axis,
            float length,
            float shaftLength,
            float shaftThickness,
            float cubeRadius)
        {
            CandidateHit best{};
            accept_hit(
                best,
                hit_axis_segment(
                    input,
                    GizmoMode::Scale,
                    axis,
                    shaftLength,
                    shaftThickness));

            const glm::vec3 axisDirection =
                GizmoConstraint::axis_vector(axis, input.orientation);
            accept_hit(
                best,
                hit_sphere(
                    input,
                    GizmoMode::Scale,
                    axis,
                    input.pivot + axisDirection * length,
                    cubeRadius));

            return best;
        }

    } // namespace

    TransformGizmo::TransformGizmo(const TransformGizmoConfig& config)
        : config_(config)
    {
    }

    const TransformGizmoConfig& TransformGizmo::config() const
    {
        return config_;
    }

    void TransformGizmo::set_config(const TransformGizmoConfig& config)
    {
        config_ = config;
    }

    GizmoHit TransformGizmo::hit_test(const TransformGizmoHitTestInput& input) const
    {
        switch (input.mode) {
        case GizmoMode::Translate:
            return hit_test_translate(input);
        case GizmoMode::Rotate:
            return hit_test_rotate(input);
        case GizmoMode::Scale:
            return hit_test_scale(input);
        case GizmoMode::Universal:
            return hit_test_universal(input);
        case GizmoMode::None:
        default:
            return GizmoHit::none();
        }
    }

    GizmoHit TransformGizmo::hit_test_translate(
        const TransformGizmoHitTestInput& input) const
    {
        const float scale = std::max(input.visualScale, 0.0001f);
        const float length = config_.axisLength * scale;
        const float coneLength =
            std::min(config_.arrowLength, config_.axisLength * 0.45f) * scale;
        const float shaftLength =
            std::max(length - coneLength, config_.shaftRadius * scale * 2.0f);
        const float thickness =
            std::max(config_.axisThickness, config_.shaftRadius + config_.pickingPadding) * scale;
        const float tipRadius =
            (config_.arrowRadius + config_.pickingPadding) * scale;
        const float planeOffset = config_.planeOffset * scale;
        const float planeSize = config_.planeSize * scale;
        const float planePadding = config_.planePickingPadding * scale;
        const float centerRadius = config_.centerRadius * scale;

        const CandidateHit center = hit_center(input, GizmoMode::Translate, centerRadius);
        if (center.valid) {
            return to_gizmo_hit(center);
        }

        CandidateHit best{};

        for (GizmoAxis axis : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z }) {
            accept_hit(
                best,
                hit_translate_axis(
                    input,
                    axis,
                    length,
                    shaftLength,
                    thickness,
                    tipRadius));
        }

        for (GizmoAxis axis : { GizmoAxis::XY, GizmoAxis::XZ, GizmoAxis::YZ }) {
            accept_hit(best, hit_plane_square(input, GizmoMode::Translate, axis, planeOffset, planeSize, planePadding));
        }

        return to_gizmo_hit(best);
    }

    GizmoHit TransformGizmo::hit_test_rotate(
        const TransformGizmoHitTestInput& input) const
    {
        const float scale = std::max(input.visualScale, 0.0001f);
        const float radius = config_.rotationRadius * scale;
        const float thickness =
            std::max(config_.rotationThickness, config_.rotationTubeRadius + config_.pickingPadding) * scale;
        const float viewRadius = radius * config_.viewRingScale;

        CandidateHit best{};

        for (GizmoAxis axis : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z }) {
            accept_hit(best, hit_rotation_ring(input, axis, radius, thickness));
        }

        accept_hit(best, hit_rotation_ring(input, GizmoAxis::View, viewRadius, thickness));
        return to_gizmo_hit(best);
    }

    GizmoHit TransformGizmo::hit_test_scale(
        const TransformGizmoHitTestInput& input) const
    {
        const float scale = std::max(input.visualScale, 0.0001f);
        const float length = config_.axisLength * scale;
        const float halfCube = config_.scaleCubeSize * 0.5f * scale;
        const float shaftLength =
            std::max(length - halfCube, config_.shaftRadius * scale * 2.0f);
        const float thickness =
            std::max(config_.axisThickness, config_.shaftRadius + config_.pickingPadding) * scale;
        const float cubeRadius =
            (config_.scaleCubeSize * 0.92f + config_.pickingPadding) * scale;
        const float planeOffset = config_.planeOffset * scale;
        const float planeSize = config_.planeSize * scale;
        const float planePadding = config_.planePickingPadding * scale;
        const float centerRadius = config_.centerRadius * scale;

        const CandidateHit center = hit_center(input, GizmoMode::Scale, centerRadius);
        if (center.valid) {
            return to_gizmo_hit(center);
        }

        CandidateHit best{};

        for (GizmoAxis axis : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z }) {
            accept_hit(
                best,
                hit_scale_axis(
                    input,
                    axis,
                    length,
                    shaftLength,
                    thickness,
                    cubeRadius));
        }

        for (GizmoAxis axis : { GizmoAxis::XY, GizmoAxis::XZ, GizmoAxis::YZ }) {
            accept_hit(best, hit_plane_square(input, GizmoMode::Scale, axis, planeOffset, planeSize, planePadding));
        }

        return to_gizmo_hit(best);
    }

    GizmoHit TransformGizmo::hit_test_universal(
        const TransformGizmoHitTestInput& input) const
    {
        TransformGizmoHitTestInput translateInput = input;
        translateInput.mode = GizmoMode::Translate;

        TransformGizmoHitTestInput rotateInput = input;
        rotateInput.mode = GizmoMode::Rotate;

        TransformGizmoHitTestInput scaleInput = input;
        scaleInput.mode = GizmoMode::Scale;

        CandidateHit best{};

        const GizmoHit scaleHit = hit_test_scale(scaleInput);
        if (scaleHit.is_valid()) {
            CandidateHit candidate{};
            candidate.valid = true;
            candidate.mode = scaleHit.mode;
            candidate.axis = scaleHit.axis;
            candidate.point = scaleHit.worldPosition;
            candidate.distance = scaleHit.distance;
            candidate.depth = scaleHit.depth;
            accept_hit(best, candidate);
        }

        const GizmoHit rotateHit = hit_test_rotate(rotateInput);
        if (rotateHit.is_valid()) {
            CandidateHit candidate{};
            candidate.valid = true;
            candidate.mode = rotateHit.mode;
            candidate.axis = rotateHit.axis;
            candidate.point = rotateHit.worldPosition;
            candidate.distance = rotateHit.distance;
            candidate.depth = rotateHit.depth;
            accept_hit(best, candidate);
        }

        const GizmoHit translateHit = hit_test_translate(translateInput);
        if (translateHit.is_valid()) {
            CandidateHit candidate{};
            candidate.valid = true;
            candidate.mode = translateHit.mode;
            candidate.axis = translateHit.axis;
            candidate.point = translateHit.worldPosition;
            candidate.distance = translateHit.distance;
            candidate.depth = translateHit.depth;
            accept_hit(best, candidate);
        }

        return to_gizmo_hit(best);
    }

} // namespace locus::editor
