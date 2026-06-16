#pragma once

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

namespace locus::kernel::math {

constexpr float Pi = 3.14159265358979323846f;
constexpr float TwoPi = Pi * 2.0f;
constexpr float HalfPi = Pi * 0.5f;
constexpr float Epsilon = 1.0e-5f;

[[nodiscard]] inline bool nearly_equal(float a, float b, float epsilon = Epsilon)
{
    return std::abs(a - b) <= epsilon;
}

[[nodiscard]] inline bool nearly_zero(float value, float epsilon = Epsilon)
{
    return std::abs(value) <= epsilon;
}

[[nodiscard]] inline float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] inline float radians(float degrees)
{
    return degrees * Pi / 180.0f;
}

[[nodiscard]] inline float degrees(float radians)
{
    return radians * 180.0f / Pi;
}

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

[[nodiscard]] inline float triangle_area(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    return glm::length(glm::cross(b - a, c - a)) * 0.5f;
}

[[nodiscard]] inline glm::vec3 triangle_normal(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& fallback = glm::vec3{ 0.0f, 1.0f, 0.0f })
{
    return safe_normalize(glm::cross(b - a, c - a), fallback);
}

[[nodiscard]] inline float signed_area_2d(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
    return ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
}

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
