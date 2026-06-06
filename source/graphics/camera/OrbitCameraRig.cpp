/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/camera/OrbitCameraRig.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace locus::graphics
{
    void OrbitCameraRig::set_target(const glm::vec3& target)
    {
        target_ = target;
    }

    void OrbitCameraRig::set_distance(float distance)
    {
        // Clamp to prevent clipping through the target or losing the scene in extreme zoom.
        distance_ = std::clamp(distance, minDistance_, maxDistance_);
    }

    void OrbitCameraRig::set_angles(float yawRadians, float pitchRadians)
    {
        yawRadians_ = yawRadians;
        // Keep pitch away from the poles to avoid unstable orbit controls.
        pitchRadians_ = std::clamp(pitchRadians, -1.5f, 1.5f);
    }

    void OrbitCameraRig::orbit(float deltaYawRadians, float deltaPitchRadians)
    {
        yawRadians_ += deltaYawRadians;
        pitchRadians_ = std::clamp(pitchRadians_ + deltaPitchRadians, -1.5f, 1.5f);
    }

    void OrbitCameraRig::pan(float deltaRight, float deltaUp)
    {
        const float cosPitch = std::cos(pitchRadians_);
        const float sinPitch = std::sin(pitchRadians_);
        const float cosYaw = std::cos(yawRadians_);
        const float sinYaw = std::sin(yawRadians_);

        const glm::vec3 position{
            target_.x + distance_ * cosPitch * sinYaw,
            target_.y + distance_ * sinPitch,
            target_.z + distance_ * cosPitch * cosYaw
        };

        const glm::mat4 view = glm::lookAt(position, target_, glm::vec3{ 0.0f, 1.0f, 0.0f });
        const glm::mat4 inverseView = glm::inverse(view);

        // Pan in camera space so dragging feels consistent at any orbit angle.
        const glm::vec3 right = glm::normalize(glm::vec3{ inverseView[0] });
        const glm::vec3 up = glm::normalize(glm::vec3{ inverseView[1] });

        target_ += right * deltaRight;
        target_ += up * deltaUp;
    }

    void OrbitCameraRig::zoom(float deltaDistance)
    {
        set_distance(distance_ + deltaDistance);
    }

    void OrbitCameraRig::apply(Camera& camera) const
    {
        const float cosPitch = std::cos(pitchRadians_);
        const float sinPitch = std::sin(pitchRadians_);
        const float cosYaw = std::cos(yawRadians_);
        const float sinYaw = std::sin(yawRadians_);

        const glm::vec3 position{
            target_.x + distance_ * cosPitch * sinYaw,
            target_.y + distance_ * sinPitch,
            target_.z + distance_ * cosPitch * cosYaw
        };

        camera.look_at(position, target_, glm::vec3{ 0.0f, 1.0f, 0.0f });
    }

    const glm::vec3& OrbitCameraRig::target() const
    {
        return target_;
    }

    float OrbitCameraRig::distance() const
    {
        return distance_;
    }

    float OrbitCameraRig::yaw_radians() const
    {
        return yawRadians_;
    }

    float OrbitCameraRig::pitch_radians() const
    {
        return pitchRadians_;
    }
}
