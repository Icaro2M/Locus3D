/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <glm/glm.hpp>

namespace locus::kernel::math {

/**
 * @brief Infinite half-line represented by an origin and direction.
 */
struct Ray {
    /**
     * @brief Ray start position in object or world space.
     */
    glm::vec3 origin{ 0.0f, 0.0f, 0.0f };

    /**
     * @brief Ray direction vector.
     *
     * @note The direction is not required to be normalized by the type itself.
     */
    glm::vec3 direction{ 0.0f, 0.0f, -1.0f };

    /**
     * @brief Computes a position along the ray.
     *
     * @param distance Distance multiplier along the direction vector.
     * @return Point at origin plus direction multiplied by distance.
     */
    [[nodiscard]] glm::vec3 at(float distance) const
    {
        return origin + direction * distance;
    }

    /**
     * @brief Returns a copy of this ray with a normalized direction.
     *
     * @return Ray with unit direction, or the original ray when direction has zero length.
     */
    [[nodiscard]] Ray normalized() const
    {
        const float length = glm::length(direction);
        if (length <= 0.0f) {
            return *this;
        }

        return Ray{ origin, direction / length };
    }
};

}
