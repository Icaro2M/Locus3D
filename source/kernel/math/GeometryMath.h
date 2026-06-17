/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

namespace locus::kernel::math {

/**
 * @brief Mathematical constant pi.
 */
constexpr float Pi = 3.14159265358979323846f;

/**
 * @brief Mathematical constant two pi.
 */
constexpr float TwoPi = Pi * 2.0f;

/**
 * @brief Mathematical constant half pi.
 */
constexpr float HalfPi = Pi * 0.5f;

/**
 * @brief Default tolerance used by geometry math helpers.
 */
constexpr float Epsilon = 1.0e-5f;

/**
 * @brief Compares two floating-point values using an absolute tolerance.
 *
 * @param a First value.
 * @param b Second value.
 * @param epsilon Maximum accepted absolute difference.
 * @return True when the values are within epsilon.
 */
[[nodiscard]] inline bool nearly_equal(float a, float b, float epsilon = Epsilon)
{
    return std::abs(a - b) <= epsilon;
}

/**
 * @brief Checks whether a floating-point value is close to zero.
 *
 * @param value Value to test.
 * @param epsilon Maximum accepted absolute distance from zero.
 * @return True when the value is within epsilon of zero.
 */
[[nodiscard]] inline bool nearly_zero(float value, float epsilon = Epsilon)
{
    return std::abs(value) <= epsilon;
}

/**
 * @brief Clamps a value to the inclusive range [0, 1].
 *
 * @param value Value to clamp.
 * @return Clamped value.
 */
[[nodiscard]] inline float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

/**
 * @brief Converts degrees to radians.
 *
 * @param degrees Angle in degrees.
 * @return Angle in radians.
 */
[[nodiscard]] inline float radians(float degrees)
{
    return degrees * Pi / 180.0f;
}

/**
 * @brief Converts radians to degrees.
 *
 * @param radians Angle in radians.
 * @return Angle in degrees.
 */
[[nodiscard]] inline float degrees(float radians)
{
    return radians * 180.0f / Pi;
}

/**
 * @brief Normalizes a vector with a fallback for near-zero length.
 *
 * @param vector Vector to normalize.
 * @param fallback Value returned when the vector length is too small.
 * @param epsilon Minimum accepted vector length.
 * @return Unit vector or fallback.
 */
[[nodiscard]] inline glm::vec3 safe_normalize(
    const glm::vec3& vector,
    const glm::vec3& fallback = glm::vec3{ 0.0f, 0.0f, 0.0f },
    float epsilon = Epsilon)
{
    const float length = glm::length(vector);
    if (length <= epsilon) {
        return fallback;
    }

    return vector / length;
}

/**
 * @brief Computes the area of a 3D triangle.
 *
 * @param a First triangle vertex.
 * @param b Second triangle vertex.
 * @param c Third triangle vertex.
 * @return Triangle area.
 */
[[nodiscard]] inline float triangle_area(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    return glm::length(glm::cross(b - a, c - a)) * 0.5f;
}

/**
 * @brief Computes a normalized triangle normal.
 *
 * @param a First triangle vertex.
 * @param b Second triangle vertex.
 * @param c Third triangle vertex.
 * @param fallback Value returned for degenerate triangles.
 * @return Unit normal following the triangle winding.
 */
[[nodiscard]] inline glm::vec3 triangle_normal(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& fallback = glm::vec3{ 0.0f, 1.0f, 0.0f })
{
    return safe_normalize(glm::cross(b - a, c - a), fallback);
}

/**
 * @brief Computes the signed 2D area determinant for three points.
 *
 * @param a First point.
 * @param b Second point.
 * @param c Third point.
 * @return Positive, negative, or zero value depending on point orientation.
 */
[[nodiscard]] inline float signed_area_2d(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
    return ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
}

/**
 * @brief Checks whether a point lies on a 2D segment.
 *
 * @param point Point to test.
 * @param a Segment start.
 * @param b Segment end.
 * @param epsilon Tolerance used for collinearity and bounds checks.
 * @return True when the point lies on the segment within epsilon.
 */
[[nodiscard]] inline bool is_point_on_segment_2d(
    const glm::vec2& point,
    const glm::vec2& a,
    const glm::vec2& b,
    float epsilon = Epsilon)
{
    const float area = signed_area_2d(a, b, point);
    if (!nearly_zero(area, epsilon)) {
        return false;
    }

    return point.x >= std::min(a.x, b.x) - epsilon
        && point.x <= std::max(a.x, b.x) + epsilon
        && point.y >= std::min(a.y, b.y) - epsilon
        && point.y <= std::max(a.y, b.y) + epsilon;
}

}
