/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/math/GeometryMath.h"
#include "kernel/math/Vec.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>

namespace locus::kernel::math {

/**
 * @brief Three-by-three floating-point matrix used by kernel geometry code.
 */
using Mat3 = glm::mat3;

/**
 * @brief Four-by-four floating-point matrix used by kernel geometry code.
 */
using Mat4 = glm::mat4;

/**
 * @brief Creates an identity 3x3 matrix.
 *
 * @return Identity matrix.
 */
[[nodiscard]] inline Mat3 identity_mat3()
{
	return Mat3{ 1.0f };
}

/**
 * @brief Creates an identity 4x4 matrix.
 *
 * @return Identity matrix.
 */
[[nodiscard]] inline Mat4 identity_mat4()
{
	return Mat4{ 1.0f };
}

/**
 * @brief Checks whether every matrix component is finite.
 *
 * @param matrix Matrix to test.
 * @return True when all components are finite numbers.
 */
[[nodiscard]] inline bool is_finite(const Mat3& matrix)
{
	for (int column = 0; column < 3; ++column) {
		for (int row = 0; row < 3; ++row) {
			if (!std::isfinite(matrix[column][row])) {
				return false;
			}
		}
	}

	return true;
}

/**
 * @brief Checks whether every matrix component is finite.
 *
 * @param matrix Matrix to test.
 * @return True when all components are finite numbers.
 */
[[nodiscard]] inline bool is_finite(const Mat4& matrix)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[column][row])) {
				return false;
			}
		}
	}

	return true;
}

/**
 * @brief Compares two 3x3 matrices using component-wise absolute tolerance.
 *
 * @param a First matrix.
 * @param b Second matrix.
 * @param epsilon Maximum accepted absolute difference per component.
 * @return True when all components are within epsilon.
 */
[[nodiscard]] inline bool nearly_equal(const Mat3& a, const Mat3& b, float epsilon = Epsilon)
{
	for (int column = 0; column < 3; ++column) {
		for (int row = 0; row < 3; ++row) {
			if (!nearly_equal(a[column][row], b[column][row], epsilon)) {
				return false;
			}
		}
	}

	return true;
}

/**
 * @brief Compares two 4x4 matrices using component-wise absolute tolerance.
 *
 * @param a First matrix.
 * @param b Second matrix.
 * @param epsilon Maximum accepted absolute difference per component.
 * @return True when all components are within epsilon.
 */
[[nodiscard]] inline bool nearly_equal(const Mat4& a, const Mat4& b, float epsilon = Epsilon)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!nearly_equal(a[column][row], b[column][row], epsilon)) {
				return false;
			}
		}
	}

	return true;
}

/**
 * @brief Builds a translation matrix.
 *
 * @param translation Translation vector.
 * @return Translation matrix.
 */
[[nodiscard]] inline Mat4 translation_matrix(const Vec3& translation)
{
	return glm::translate(Mat4{ 1.0f }, translation);
}

/**
 * @brief Builds a non-uniform scale matrix.
 *
 * @param scale Scale vector.
 * @return Scale matrix.
 */
[[nodiscard]] inline Mat4 scale_matrix(const Vec3& scale)
{
	return glm::scale(Mat4{ 1.0f }, scale);
}

/**
 * @brief Builds a uniform scale matrix.
 *
 * @param scale Scale factor.
 * @return Scale matrix.
 */
[[nodiscard]] inline Mat4 scale_matrix(float scale)
{
	return glm::scale(Mat4{ 1.0f }, Vec3{ scale, scale, scale });
}

/**
 * @brief Builds a rotation matrix from a quaternion.
 *
 * @param rotation Rotation quaternion.
 * @return Rotation matrix.
 */
[[nodiscard]] inline Mat4 rotation_matrix(const glm::quat& rotation)
{
	return glm::mat4_cast(rotation);
}

/**
 * @brief Builds a transform matrix from translation, rotation, and scale components.
 *
 * @param translation Translation component.
 * @param rotation Rotation component.
 * @param scale Scale component.
 * @return Matrix applying translation, rotation, then scale.
 */
[[nodiscard]] inline Mat4 compose_trs(
	const Vec3& translation,
	const glm::quat& rotation,
	const Vec3& scale)
{
	return translation_matrix(translation) * rotation_matrix(rotation) * scale_matrix(scale);
}

/**
 * @brief Transforms a position by a matrix.
 *
 * @param matrix Matrix to apply.
 * @param point Point with homogeneous w equal to one.
 * @return Transformed point.
 */
[[nodiscard]] inline Vec3 transform_point(const Mat4& matrix, const Vec3& point)
{
	return Vec3{ matrix * Vec4{ point, 1.0f } };
}

/**
 * @brief Transforms a direction vector by a matrix.
 *
 * @param matrix Matrix to apply.
 * @param vector Vector with homogeneous w equal to zero.
 * @return Transformed vector.
 */
[[nodiscard]] inline Vec3 transform_vector(const Mat4& matrix, const Vec3& vector)
{
	return Vec3{ matrix * Vec4{ vector, 0.0f } };
}

/**
 * @brief Transforms a normal vector by the inverse transpose of a matrix.
 *
 * @param matrix Matrix to apply.
 * @param normal Normal vector.
 * @param fallback Value returned when the transformed normal is too small.
 * @return Transformed unit normal.
 */
[[nodiscard]] inline Vec3 transform_normal(
	const Mat4& matrix,
	const Vec3& normal,
	const Vec3& fallback = Vec3{ 0.0f, 1.0f, 0.0f })
{
	const Mat3 normalMatrix = glm::transpose(glm::inverse(Mat3{ matrix }));
	return safe_normalize(normalMatrix * normal, fallback);
}

/**
 * @brief Computes the normal matrix for a transform matrix.
 *
 * @param matrix Transform matrix.
 * @return Inverse transpose 3x3 matrix.
 */
[[nodiscard]] inline Mat3 normal_matrix(const Mat4& matrix)
{
	return glm::transpose(glm::inverse(Mat3{ matrix }));
}

/**
 * @brief Inverts a matrix, returning identity when the determinant is too small.
 *
 * @param matrix Matrix to invert.
 * @param epsilon Minimum accepted determinant magnitude.
 * @return Inverse matrix or identity matrix.
 */
[[nodiscard]] inline Mat4 inverse_or_identity(const Mat4& matrix, float epsilon = Epsilon)
{
	const float determinant = glm::determinant(matrix);
	if (std::abs(determinant) <= epsilon) {
		return identity_mat4();
	}

	return glm::inverse(matrix);
}

/**
 * @brief Builds an orthographic projection matrix.
 *
 * @param left Left plane.
 * @param right Right plane.
 * @param bottom Bottom plane.
 * @param top Top plane.
 * @param nearPlane Near plane.
 * @param farPlane Far plane.
 * @return Orthographic projection matrix.
 */
[[nodiscard]] inline Mat4 orthographic_matrix(
	float left,
	float right,
	float bottom,
	float top,
	float nearPlane,
	float farPlane)
{
	return glm::ortho(left, right, bottom, top, nearPlane, farPlane);
}

/**
 * @brief Builds a perspective projection matrix.
 *
 * @param verticalFovRadians Vertical field of view in radians.
 * @param aspect Aspect ratio.
 * @param nearPlane Near plane.
 * @param farPlane Far plane.
 * @return Perspective projection matrix.
 */
[[nodiscard]] inline Mat4 perspective_matrix(
	float verticalFovRadians,
	float aspect,
	float nearPlane,
	float farPlane)
{
	return glm::perspective(verticalFovRadians, aspect, nearPlane, farPlane);
}

}