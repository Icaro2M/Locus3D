/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::kernel::math {

/**
 * @brief Position, rotation, and scale transform for geometric objects.
 */
struct Transform {
    /**
     * @brief Translation component.
     */
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };

    /**
     * @brief Rotation component stored as a quaternion.
     */
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };

    /**
     * @brief Non-uniform scale component.
     */
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    /**
     * @brief Creates an identity transform.
     *
     * @return Transform with zero translation, identity rotation, and unit scale.
     */
    [[nodiscard]] static Transform identity()
    {
        return {};
    }

    /**
     * @brief Builds the transform matrix.
     *
     * @return Matrix applying translation, rotation, then scale.
     */
    [[nodiscard]] glm::mat4 matrix() const
    {
        const glm::mat4 translationMatrix = glm::translate(glm::mat4{ 1.0f }, translation);
        const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
        const glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.0f }, scale);
        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    /**
     * @brief Builds the inverse transform matrix.
     *
     * @return Matrix applying inverse scale, inverse rotation, and inverse translation.
     * @note Zero scale components are treated as one to avoid division by zero.
     */
    [[nodiscard]] glm::mat4 inverse_matrix() const
    {
        const glm::vec3 safeScale{
            scale.x == 0.0f ? 1.0f : scale.x,
            scale.y == 0.0f ? 1.0f : scale.y,
            scale.z == 0.0f ? 1.0f : scale.z
        };

        const glm::mat4 inverseScale = glm::scale(
            glm::mat4{ 1.0f },
            glm::vec3{ 1.0f / safeScale.x, 1.0f / safeScale.y, 1.0f / safeScale.z }
        );
        const glm::mat4 inverseRotation = glm::mat4_cast(glm::inverse(rotation));
        const glm::mat4 inverseTranslation = glm::translate(glm::mat4{ 1.0f }, -translation);

        return inverseScale * inverseRotation * inverseTranslation;
    }

    /**
     * @brief Transforms a point by this transform.
     *
     * @param point Point with homogeneous w equal to one.
     * @return Transformed point.
     */
    [[nodiscard]] glm::vec3 transform_point(const glm::vec3& point) const
    {
        return glm::vec3{ matrix() * glm::vec4{ point, 1.0f } };
    }

    /**
     * @brief Transforms a direction vector by this transform.
     *
     * @param vector Vector with homogeneous w equal to zero.
     * @return Transformed vector.
     */
    [[nodiscard]] glm::vec3 transform_vector(const glm::vec3& vector) const
    {
        return glm::vec3{ matrix() * glm::vec4{ vector, 0.0f } };
    }
};

}
