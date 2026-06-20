/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Types.h"
#include "kernel/math/GeometryMath.h"

#include <glm/glm.hpp>

#include <cmath>

namespace locus::kernel::math {

/**
 * @brief Two-dimensional floating-point vector used by kernel geometry code.
 */
using Vec2 = glm::vec2;

/**
 * @brief Three-dimensional floating-point vector used by kernel geometry code.
 */
using Vec3 = glm::vec3;

/**
 * @brief Four-dimensional floating-point vector used by kernel geometry code.
 */
using Vec4 = glm::vec4;

/**
 * @brief Two-dimensional signed integer vector.
 */
using IVec2 = glm::ivec2;

/**
 * @brief Three-dimensional signed integer vector.
 */
using IVec3 = glm::ivec3;

/**
 * @brief Four-dimensional signed integer vector.
 */
using IVec4 = glm::ivec4;

/**
 * @brief Converts an axis enum to a vector component index.
 *
 * @param axis Axis to convert.
 * @return Component index in the range [0, 2].
 */
[[nodiscard]] inline int axis_index(Axis axis)
{
	switch (axis) {
	case Axis::X:
		return 0;
	case Axis::Y:
		return 1;
	case Axis::Z:
		return 2;
	default:
		return 0;
	}
}

/**
 * @brief Builds a unit vector pointing along an axis and orientation.
 *
 * @param axis Axis direction.
 * @param orientation Positive or negative orientation.
 * @return Unit vector aligned with the requested axis.
 */
[[nodiscard]] inline Vec3 axis_vector(Axis axis, Orientation orientation = Orientation::Positive)
{
	const float sign = orientation == Orientation::Positive ? 1.0f : -1.0f;

	switch (axis) {
	case Axis::X:
		return Vec3{ sign, 0.0f, 0.0f };
	case Axis::Y:
		return Vec3{ 0.0f, sign, 0.0f };
	case Axis::Z:
		return Vec3{ 0.0f, 0.0f, sign };
	default:
		return Vec3{ sign, 0.0f, 0.0f };
	}
}

/**
 * @brief Reads a vector component by axis.
 *
 * @param vector Vector to inspect.
 * @param axis Axis component to read.
 * @return Component value.
 */
[[nodiscard]] inline float component(const Vec3& vector, Axis axis)
{
	return vector[axis_index(axis)];
}

/**
 * @brief Writes a vector component by axis.
 *
 * @param vector Vector to modify.
 * @param axis Axis component to write.
 * @param value New component value.
 */
inline void set_component(Vec3& vector, Axis axis, float value)
{
	vector[axis_index(axis)] = value;
}

/**
 * @brief Checks whether every vector component is finite.
 *
 * @param vector Vector to test.
 * @return True when all components are finite numbers.
 */
[[nodiscard]] inline bool is_finite(const Vec2& vector)
{
	return std::isfinite(vector.x) && std::isfinite(vector.y);
}

/**
 * @brief Checks whether every vector component is finite.
 *
 * @param vector Vector to test.
 * @return True when all components are finite numbers.
 */
[[nodiscard]] inline bool is_finite(const Vec3& vector)
{
	return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

/**
 * @brief Checks whether every vector component is finite.
 *
 * @param vector Vector to test.
 * @return True when all components are finite numbers.
 */
[[nodiscard]] inline bool is_finite(const Vec4& vector)
{
	return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z) && std::isfinite(vector.w);
}

/**
 * @brief Compares two vectors using component-wise absolute tolerance.
 *
 * @param a First vector.
 * @param b Second vector.
 * @param epsilon Maximum accepted absolute difference per component.
 * @return True when all components are within epsilon.
 */
[[nodiscard]] inline bool nearly_equal(const Vec2& a, const Vec2& b, float epsilon = Epsilon)
{
	return nearly_equal(a.x, b.x, epsilon) && nearly_equal(a.y, b.y, epsilon);
}

/**
 * @brief Compares two vectors using component-wise absolute tolerance.
 *
 * @param a First vector.
 * @param b Second vector.
 * @param epsilon Maximum accepted absolute difference per component.
 * @return True when all components are within epsilon.
 */
[[nodiscard]] inline bool nearly_equal(const Vec3& a, const Vec3& b, float epsilon = Epsilon)
{
	return nearly_equal(a.x, b.x, epsilon)
		&& nearly_equal(a.y, b.y, epsilon)
		&& nearly_equal(a.z, b.z, epsilon);
}

/**
 * @brief Compares two vectors using component-wise absolute tolerance.
 *
 * @param a First vector.
 * @param b Second vector.
 * @param epsilon Maximum accepted absolute difference per component.
 * @return True when all components are within epsilon.
 */
[[nodiscard]] inline bool nearly_equal(const Vec4& a, const Vec4& b, float epsilon = Epsilon)
{
	return nearly_equal(a.x, b.x, epsilon)
		&& nearly_equal(a.y, b.y, epsilon)
		&& nearly_equal(a.z, b.z, epsilon)
		&& nearly_equal(a.w, b.w, epsilon);
}

/**
 * @brief Computes squared vector length.
 *
 * @param vector Vector to measure.
 * @return Squared length.
 */
[[nodiscard]] inline float length_squared(const Vec2& vector)
{
	return glm::dot(vector, vector);
}

/**
 * @brief Computes squared vector length.
 *
 * @param vector Vector to measure.
 * @return Squared length.
 */
[[nodiscard]] inline float length_squared(const Vec3& vector)
{
	return glm::dot(vector, vector);
}

/**
 * @brief Computes squared distance between two points.
 *
 * @param a First point.
 * @param b Second point.
 * @return Squared distance.
 */
[[nodiscard]] inline float distance_squared(const Vec2& a, const Vec2& b)
{
	return length_squared(a - b);
}

/**
 * @brief Computes squared distance between two points.
 *
 * @param a First point.
 * @param b Second point.
 * @return Squared distance.
 */
[[nodiscard]] inline float distance_squared(const Vec3& a, const Vec3& b)
{
	return length_squared(a - b);
}

/**
 * @brief Projects a vector onto a direction.
 *
 * @param vector Vector to project.
 * @param direction Projection direction.
 * @param epsilon Minimum accepted squared direction length.
 * @return Projected vector, or zero when direction is too small.
 */
[[nodiscard]] inline Vec3 project(const Vec3& vector, const Vec3& direction, float epsilon = Epsilon)
{
	const float denominator = glm::dot(direction, direction);
	if (denominator <= epsilon * epsilon) {
		return Vec3{ 0.0f, 0.0f, 0.0f };
	}

	return direction * (glm::dot(vector, direction) / denominator);
}

/**
 * @brief Removes the component of a vector along a direction.
 *
 * @param vector Vector to reject.
 * @param direction Direction to remove.
 * @param epsilon Minimum accepted squared direction length.
 * @return Vector component perpendicular to direction.
 */
[[nodiscard]] inline Vec3 reject(const Vec3& vector, const Vec3& direction, float epsilon = Epsilon)
{
	return vector - project(vector, direction, epsilon);
}

/**
 * @brief Returns the dominant axis of a vector by absolute magnitude.
 *
 * @param vector Vector to inspect.
 * @return Axis with the largest absolute component.
 */
[[nodiscard]] inline Axis dominant_axis(const Vec3& vector)
{
	const Vec3 absolute = glm::abs(vector);

	if (absolute.x >= absolute.y && absolute.x >= absolute.z) {
		return Axis::X;
	}

	if (absolute.y >= absolute.z) {
		return Axis::Y;
	}

	return Axis::Z;
}

/**
 * @brief Returns a normalized vector with a fallback for near-zero length.
 *
 * @param vector Vector to normalize.
 * @param fallback Value returned when the vector length is too small.
 * @param epsilon Minimum accepted vector length.
 * @return Unit vector or fallback.
 */
[[nodiscard]] inline Vec2 safe_normalize(
	const Vec2& vector,
	const Vec2& fallback = Vec2{ 0.0f, 0.0f },
	float epsilon = Epsilon)
{
	const float length = glm::length(vector);
	if (length <= epsilon) {
		return fallback;
	}

	return vector / length;
}

/**
 * @brief Returns a vector with each component clamped to a range.
 *
 * @param vector Vector to clamp.
 * @param minValue Minimum accepted component value.
 * @param maxValue Maximum accepted component value.
 * @return Component-wise clamped vector.
 */
[[nodiscard]] inline Vec3 clamp(const Vec3& vector, float minValue, float maxValue)
{
	return Vec3{
		std::clamp(vector.x, minValue, maxValue),
		std::clamp(vector.y, minValue, maxValue),
		std::clamp(vector.z, minValue, maxValue)
	};
}

/**
 * @brief Returns a vector with each component clamped to the inclusive range [0, 1].
 *
 * @param vector Vector to clamp.
 * @return Component-wise clamped vector.
 */
[[nodiscard]] inline Vec3 clamp01(const Vec3& vector)
{
	return clamp(vector, 0.0f, 1.0f);
}

}