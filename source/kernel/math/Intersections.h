/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/math/Bounds.h"
#include "kernel/math/GeometryMath.h"
#include "kernel/math/Ray.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/glm.hpp>

namespace locus::kernel::math {

/**
 * @brief Result of a ray intersection query.
 */
struct RayHit {
    /**
     * @brief True when an intersection was found.
     */
    bool hit = false;

    /**
     * @brief Distance along the ray direction to the hit point.
     */
    float distance = 0.0f;

    /**
     * @brief Hit position.
     */
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };

    /**
     * @brief Surface normal at the hit position.
     */
    glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
};

/**
 * @brief Intersects a ray against an infinite plane.
 *
 * @param ray Ray to test.
 * @param planePoint Any point on the plane.
 * @param planeNormal Plane normal.
 * @param epsilon Parallelism tolerance.
 * @return Hit information, or a miss result when the ray does not hit the plane forward.
 */
[[nodiscard]] inline RayHit intersect_ray_plane(
    const Ray& ray,
    const glm::vec3& planePoint,
    const glm::vec3& planeNormal,
    float epsilon = Epsilon)
{
    const float denominator = glm::dot(ray.direction, planeNormal);
    if (std::abs(denominator) <= epsilon) {
        return {};
    }

    const float distance = glm::dot(planePoint - ray.origin, planeNormal) / denominator;
    if (distance < 0.0f) {
        return {};
    }

    return RayHit{ true, distance, ray.at(distance), safe_normalize(planeNormal, glm::vec3{ 0.0f, 1.0f, 0.0f }) };
}

/**
 * @brief Intersects a ray against a triangle.
 *
 * Uses the Moller-Trumbore ray-triangle intersection test.
 *
 * @param ray Ray to test.
 * @param a First triangle vertex.
 * @param b Second triangle vertex.
 * @param c Third triangle vertex.
 * @param epsilon Degeneracy and parallelism tolerance.
 * @return Hit information, or a miss result when no forward hit exists.
 */
[[nodiscard]] inline RayHit intersect_ray_triangle(
    const Ray& ray,
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    float epsilon = Epsilon)
{
    const glm::vec3 edgeA = b - a;
    const glm::vec3 edgeB = c - a;
    const glm::vec3 p = glm::cross(ray.direction, edgeB);
    const float determinant = glm::dot(edgeA, p);

    if (std::abs(determinant) <= epsilon) {
        return {};
    }

    const float inverseDeterminant = 1.0f / determinant;
    const glm::vec3 t = ray.origin - a;
    const float u = glm::dot(t, p) * inverseDeterminant;

    if (u < 0.0f || u > 1.0f) {
        return {};
    }

    const glm::vec3 q = glm::cross(t, edgeA);
    const float v = glm::dot(ray.direction, q) * inverseDeterminant;

    if (v < 0.0f || u + v > 1.0f) {
        return {};
    }

    const float distance = glm::dot(edgeB, q) * inverseDeterminant;
    if (distance < 0.0f) {
        return {};
    }

    return RayHit{ true, distance, ray.at(distance), triangle_normal(a, b, c) };
}

/**
 * @brief Intersects a ray against an axis-aligned bounding box.
 *
 * @param ray Ray to test.
 * @param bounds Bounds to test.
 * @param epsilon Tolerance used for near-parallel slab checks.
 * @return Hit information, or a miss result when the bounds is invalid or not hit.
 */
[[nodiscard]] inline RayHit intersect_ray_bounds(
    const Ray& ray,
    const Bounds& bounds,
    float epsilon = Epsilon)
{
    if (!bounds.is_valid()) {
        return {};
    }

    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; ++axis) {
        const float origin = ray.origin[axis];
        const float direction = ray.direction[axis];
        const float minValue = bounds.min[axis];
        const float maxValue = bounds.max[axis];

        if (std::abs(direction) <= epsilon) {
            if (origin < minValue || origin > maxValue) {
                return {};
            }
            continue;
        }

        float t1 = (minValue - origin) / direction;
        float t2 = (maxValue - origin) / direction;
        if (t1 > t2) {
            std::swap(t1, t2);
        }

        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);

        if (tMin > tMax) {
            return {};
        }
    }

    const glm::vec3 position = ray.at(tMin);
    glm::vec3 normal{ 0.0f, 0.0f, 0.0f };

    if (nearly_equal(position.x, bounds.min.x, epsilon)) {
        normal = glm::vec3{ -1.0f, 0.0f, 0.0f };
    } else if (nearly_equal(position.x, bounds.max.x, epsilon)) {
        normal = glm::vec3{ 1.0f, 0.0f, 0.0f };
    } else if (nearly_equal(position.y, bounds.min.y, epsilon)) {
        normal = glm::vec3{ 0.0f, -1.0f, 0.0f };
    } else if (nearly_equal(position.y, bounds.max.y, epsilon)) {
        normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
    } else if (nearly_equal(position.z, bounds.min.z, epsilon)) {
        normal = glm::vec3{ 0.0f, 0.0f, -1.0f };
    } else if (nearly_equal(position.z, bounds.max.z, epsilon)) {
        normal = glm::vec3{ 0.0f, 0.0f, 1.0f };
    }

    return RayHit{ true, tMin, position, normal };
}

/**
 * @brief Checks whether two axis-aligned bounds overlap.
 *
 * @param a First bounds.
 * @param b Second bounds.
 * @return True when both bounds are valid and overlap on all axes.
 */
[[nodiscard]] inline bool intersects(const Bounds& a, const Bounds& b)
{
    if (!a.is_valid() || !b.is_valid()) {
        return false;
    }

    return a.min.x <= b.max.x && a.max.x >= b.min.x
        && a.min.y <= b.max.y && a.max.y >= b.min.y
        && a.min.z <= b.max.z && a.max.z >= b.min.z;
}

}
