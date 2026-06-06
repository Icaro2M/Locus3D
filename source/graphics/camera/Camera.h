/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/camera/Projection.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::graphics
{
    /**
     * @brief View camera with position, orientation, and projection state.
     */
    class Camera
    {
    public:
        /**
         * @brief Creates a camera at the default editor position.
         */
        Camera() = default;

        /**
         * @brief Destroys the camera state.
         */
        ~Camera() = default;

        /**
         * @brief Sets the camera world position.
         *
         * @param position New world-space position.
         */
        void set_position(const glm::vec3& position);

        /**
         * @brief Sets the camera world orientation.
         *
         * @param rotation New orientation quaternion.
         */
        void set_rotation(const glm::quat& rotation);

        /**
         * @brief Places and orients the camera toward a target.
         *
         * @param eye Camera position.
         * @param target Point to look at.
         * @param up Preferred up direction.
         */
        void look_at(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up);

        /**
         * @brief Returns mutable projection parameters.
         *
         * @return Projection state.
         */
        Projection& projection();

        /**
         * @brief Returns read-only projection parameters.
         *
         * @return Projection state.
         */
        [[nodiscard]] const Projection& projection() const;

        /**
         * @brief Returns the camera world position.
         *
         * @return Position vector.
         */
        [[nodiscard]] const glm::vec3& position() const;

        /**
         * @brief Returns the camera world orientation.
         *
         * @return Rotation quaternion.
         */
        [[nodiscard]] const glm::quat& rotation() const;

        /**
         * @brief Returns the camera forward direction.
         *
         * @return Normalized forward vector in world space.
         */
        [[nodiscard]] glm::vec3 forward() const;

        /**
         * @brief Returns the camera right direction.
         *
         * @return Normalized right vector in world space.
         */
        [[nodiscard]] glm::vec3 right() const;

        /**
         * @brief Returns the camera up direction.
         *
         * @return Normalized up vector in world space.
         */
        [[nodiscard]] glm::vec3 up() const;

        /**
         * @brief Builds the world-to-view matrix.
         *
         * @return View matrix.
         */
        [[nodiscard]] glm::mat4 view_matrix() const;

        /**
         * @brief Builds the view-to-clip projection matrix.
         *
         * @return Projection matrix.
         */
        [[nodiscard]] glm::mat4 projection_matrix() const;

        /**
         * @brief Builds the combined view-projection matrix.
         *
         * @return Projection multiplied by view.
         */
        [[nodiscard]] glm::mat4 view_projection_matrix() const;

    private:
        glm::vec3 position_{ 0.0f, 0.0f, 5.0f };
        glm::quat rotation_{ 1.0f, 0.0f, 0.0f, 0.0f };
        Projection projection_{};
    };
}
