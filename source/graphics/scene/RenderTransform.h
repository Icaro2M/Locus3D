/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::graphics
{
    /**
     * @brief Local transform used to place an object in render space.
     */
    struct RenderTransform
    {
        /**
         * @brief Translation in world units.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Orientation as a unit quaternion.
         */
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief Non-uniform scale along local axes.
         */
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

        /**
         * @brief Builds the local-to-world transform matrix.
         *
         * @return Transform matrix using translation, rotation, and scale.
         */
        [[nodiscard]] glm::mat4 matrix() const;
    };
}
