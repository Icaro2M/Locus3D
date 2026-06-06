/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/camera/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::graphics
{
    void Camera::set_position(const glm::vec3& position)
    {
        position_ = position;
    }

    void Camera::set_rotation(const glm::quat& rotation)
    {
        rotation_ = rotation;
    }

    void Camera::look_at(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& upVector)
    {
        position_ = eye;

        const glm::mat4 view = glm::lookAt(eye, target, upVector);
        const glm::mat4 inverseView = glm::inverse(view);

        // Store the world orientation, not the inverse orientation encoded by the view matrix.
        rotation_ = glm::quat_cast(inverseView);
    }

    Projection& Camera::projection()
    {
        return projection_;
    }

    const Projection& Camera::projection() const
    {
        return projection_;
    }

    const glm::vec3& Camera::position() const
    {
        return position_;
    }

    const glm::quat& Camera::rotation() const
    {
        return rotation_;
    }

    glm::vec3 Camera::forward() const
    {
        return rotation_ * glm::vec3{ 0.0f, 0.0f, -1.0f };
    }

    glm::vec3 Camera::right() const
    {
        return rotation_ * glm::vec3{ 1.0f, 0.0f, 0.0f };
    }

    glm::vec3 Camera::up() const
    {
        return rotation_ * glm::vec3{ 0.0f, 1.0f, 0.0f };
    }

    glm::mat4 Camera::view_matrix() const
    {
        // View space is the inverse of the camera world transform.
        const glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(rotation_));
        const glm::mat4 translationMatrix = glm::translate(glm::mat4{ 1.0f }, -position_);

        return rotationMatrix * translationMatrix;
    }

    glm::mat4 Camera::projection_matrix() const
    {
        return projection_.matrix();
    }

    glm::mat4 Camera::view_projection_matrix() const
    {
        return projection_matrix() * view_matrix();
    }
}
