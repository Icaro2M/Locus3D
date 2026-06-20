/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/math/GeometryMath.h"
#include "kernel/math/Vec.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

namespace locus::kernel::math {

/**
 * @brief Quaternion used to represent 3D rotations in kernel geometry code.
 */
using Quaternion = glm::quat;

/**
 * @brief Creates an identity quaternion.
 *
 * @return Identity rotation quaternion.
 */
[[nodiscard]] inline Quaternion identity_quaternion()
{
	return Quaternion{ 1.0f, 0.0f, 0.0f, 0.0f };
}

/**
 * @brief Checks whether every quaternion component is finite.
 *
 * @param quaternion Quaternion to test.
 * @return True when all components are finite numbers.
 */
[[nodiscard]] inline bool is_finite(const Quaternion& quaternion)
{
	return std::isfinite(quaternion.w)
		&& std::isfinite(quaternion.x)
		&& std::isfinite(quaternion.y)
		&& std::isfinite(quaternion.z);
}

/**
 * @brief Compares two quaternions using component-wise absolute tolerance.
 *
 * @param a First quaternion.
 * @param b Second quaternion.
 * @param epsilon Maximum accepted absolute difference per component.
 * @return True when all components are within epsilon.
 */
[[nodiscard]] inline bool nearly_equal(const Quaternion& a, const Quaternion& b, float epsilon = Epsilon)
{
	return nearly_equal(a.w, b.w, epsilon)
		&& nearly_equal(a.x, b.x, epsilon)
		&& nearly_equal(a.y, b.y, epsilon)
		&& nearly_equal(a.z, b.z, epsilon);
}

/**
 * @brief Normalizes a quaternion with identity fallback.
 *
 * @param quaternion Quaternion to normalize.
 * @param fallback Value returned when quaternion length is too small.
 * @param epsilon Minimum accepted quaternion length.
 * @return Unit quaternion or fallback.
 */
[[nodiscard]] inline Quaternion safe_normalize(
	const Quaternion& quaternion,
	const Quaternion& fallback = identity_quaternion(),
	float epsilon = Epsilon)
{
	const float length = glm::length(quaternion);
	if (length <= epsilon) {
		return fallback;
	}

	return quaternion / length;
}

/**
 * @brief Builds a quaternion from an axis and angle.
 *
 * @param axis Rotation axis.
 * @param angleRadians Rotation angle in radians.
 * @param epsilon Minimum accepted axis length.
 * @return Rotation quaternion, or identity when the axis is too small.
 */
[[nodiscard]] inline Quaternion quaternion_from_axis_angle(
	const Vec3& axis,
	float angleRadians,
	float epsilon = Epsilon)
{
	const Vec3 normalizedAxis = safe_normalize(axis, Vec3{ 0.0f, 0.0f, 0.0f }, epsilon);
	if (length_squared(normalizedAxis) <= epsilon * epsilon) {
		return identity_quaternion();
	}

	return glm::angleAxis(angleRadians, normalizedAxis);
}

/**
 * @brief Builds a quaternion from Euler angles in XYZ order.
 *
 * @param eulerRadians Euler angles in radians.
 * @return Rotation quaternion.
 */
[[nodiscard]] inline Quaternion quaternion_from_euler_xyz(const Vec3& eulerRadians)
{
	const Quaternion xRotation = glm::angleAxis(eulerRadians.x, Vec3{ 1.0f, 0.0f, 0.0f });
	const Quaternion yRotation = glm::angleAxis(eulerRadians.y, Vec3{ 0.0f, 1.0f, 0.0f });
	const Quaternion zRotation = glm::angleAxis(eulerRadians.z, Vec3{ 0.0f, 0.0f, 1.0f });

	return safe_normalize(zRotation * yRotation * xRotation);
}

/**
 * @brief Rotates a vector by a quaternion.
 *
 * @param quaternion Rotation quaternion.
 * @param vector Vector to rotate.
 * @return Rotated vector.
 */
[[nodiscard]] inline Vec3 rotate_vector(const Quaternion& quaternion, const Vec3& vector)
{
	return safe_normalize(quaternion) * vector;
}

/**
 * @brief Computes the inverse rotation of a quaternion.
 *
 * @param quaternion Quaternion to invert.
 * @return Inverse unit quaternion.
 */
[[nodiscard]] inline Quaternion inverse_rotation(const Quaternion& quaternion)
{
	return glm::inverse(safe_normalize(quaternion));
}

/**
 * @brief Builds a rotation from one direction to another.
 *
 * @param from Source direction.
 * @param to Target direction.
 * @param fallbackAxis Axis used when the directions are opposite.
 * @param epsilon Minimum accepted direction length and parallel tolerance.
 * @return Quaternion rotating from source direction to target direction.
 */
[[nodiscard]] inline Quaternion quaternion_between_vectors(
	const Vec3& from,
	const Vec3& to,
	const Vec3& fallbackAxis = Vec3{ 0.0f, 1.0f, 0.0f },
	float epsilon = Epsilon)
{
	const Vec3 fromNormalized = safe_normalize(from, Vec3{ 0.0f, 0.0f, 0.0f }, epsilon);
	const Vec3 toNormalized = safe_normalize(to, Vec3{ 0.0f, 0.0f, 0.0f }, epsilon);

	if (length_squared(fromNormalized) <= epsilon * epsilon || length_squared(toNormalized) <= epsilon * epsilon) {
		return identity_quaternion();
	}

	const float dotValue = std::clamp(glm::dot(fromNormalized, toNormalized), -1.0f, 1.0f);

	if (dotValue >= 1.0f - epsilon) {
		return identity_quaternion();
	}

	if (dotValue <= -1.0f + epsilon) {
		Vec3 axis = safe_normalize(fallbackAxis, Vec3{ 0.0f, 0.0f, 0.0f }, epsilon);

		if (length_squared(axis) <= epsilon * epsilon || std::abs(glm::dot(axis, fromNormalized)) >= 1.0f - epsilon) {
			axis = glm::abs(fromNormalized.x) < 0.9f
				? safe_normalize(glm::cross(fromNormalized, Vec3{ 1.0f, 0.0f, 0.0f }))
				: safe_normalize(glm::cross(fromNormalized, Vec3{ 0.0f, 1.0f, 0.0f }));
		}
		else {
			axis = safe_normalize(reject(axis, fromNormalized));
		}

		return quaternion_from_axis_angle(axis, Pi);
	}

	const Vec3 axis = glm::cross(fromNormalized, toNormalized);
	const float s = std::sqrt((1.0f + dotValue) * 2.0f);
	const float inverseS = 1.0f / s;

	return safe_normalize(Quaternion{
		s * 0.5f,
		axis.x * inverseS,
		axis.y * inverseS,
		axis.z * inverseS
	});
}

/**
 * @brief Interpolates between two rotations.
 *
 * @param a First rotation.
 * @param b Second rotation.
 * @param t Interpolation factor.
 * @return Spherical linear interpolation result.
 */
[[nodiscard]] inline Quaternion slerp(const Quaternion& a, const Quaternion& b, float t)
{
	return safe_normalize(glm::slerp(safe_normalize(a), safe_normalize(b), clamp01(t)));
}

}