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
    namespace {
        constexpr float MinPitchRadians = -1.5f;
        constexpr float MaxPitchRadians = 1.5f;
        constexpr float DirectionEpsilon = 0.000001f;

        [[nodiscard]] glm::vec3 safe_normalize(
            const glm::vec3& value,
            const glm::vec3& fallback)
        {
            const float length = glm::length(value);
            if (length <= DirectionEpsilon) {
                return fallback;
            }

            return value / length;
        }

        [[nodiscard]] glm::quat orientation_from_forward_up(
            const glm::vec3& forward,
            const glm::vec3& up)
        {
            const glm::vec3 safeForward =
                safe_normalize(forward, { 0.0f, 0.0f, -1.0f });
            glm::vec3 safeUp =
                safe_normalize(up, { 0.0f, 1.0f, 0.0f });

            if (std::abs(glm::dot(safeForward, safeUp)) > 0.999f) {
                safeUp = std::abs(safeForward.y) < 0.999f
                    ? glm::vec3{ 0.0f, 1.0f, 0.0f }
                    : glm::vec3{ 0.0f, 0.0f, 1.0f };
            }

            return glm::normalize(glm::quatLookAt(safeForward, safeUp));
        }

        [[nodiscard]] glm::quat orientation_from_angles(
            float yawRadians,
            float pitchRadians)
        {
            const float cosPitch = std::cos(pitchRadians);
            const float sinPitch = std::sin(pitchRadians);
            const float cosYaw = std::cos(yawRadians);
            const float sinYaw = std::sin(yawRadians);

            const glm::vec3 offset{
                cosPitch * sinYaw,
                sinPitch,
                cosPitch * cosYaw
            };

            return orientation_from_forward_up(
                -safe_normalize(offset, { 0.0f, 0.0f, 1.0f }),
                { 0.0f, 1.0f, 0.0f });
        }
    }

    OrbitCameraRig::OrbitCameraRig()
    {
        update_orientation_from_angles();
    }

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
        pitchRadians_ = std::clamp(pitchRadians, MinPitchRadians, MaxPitchRadians);
        update_orientation_from_angles();
    }

    void OrbitCameraRig::set_orientation(const glm::quat& orientation)
    {
        orientation_ = glm::normalize(orientation);
        update_angles_from_orientation();
    }

    void OrbitCameraRig::look(const glm::vec3& forward, const glm::vec3& up)
    {
        set_orientation(orientation_from_forward_up(forward, up));
    }

    void OrbitCameraRig::orbit(float deltaYawRadians, float deltaPitchRadians)
    {
        set_angles(
            yawRadians_ + deltaYawRadians,
            pitchRadians_ + deltaPitchRadians);
    }

    void OrbitCameraRig::pan(float deltaRight, float deltaUp)
    {
        // Pan in camera space so dragging feels consistent at any orbit angle.
        const glm::vec3 right = orientation_ * glm::vec3{ 1.0f, 0.0f, 0.0f };
        const glm::vec3 up = orientation_ * glm::vec3{ 0.0f, 1.0f, 0.0f };

        target_ += right * deltaRight;
        target_ += up * deltaUp;
    }

    void OrbitCameraRig::zoom(float deltaDistance)
    {
        set_distance(distance_ + deltaDistance);
    }

    void OrbitCameraRig::apply(Camera& camera) const
    {
        const glm::vec3 forward =
            orientation_ * glm::vec3{ 0.0f, 0.0f, -1.0f };
        const glm::vec3 position = target_ - forward * distance_;

        camera.set_position(position);
        camera.set_rotation(orientation_);
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

    const glm::quat& OrbitCameraRig::orientation() const
    {
        return orientation_;
    }

    void OrbitCameraRig::update_orientation_from_angles()
    {
        orientation_ = orientation_from_angles(
            yawRadians_,
            pitchRadians_);
    }

    void OrbitCameraRig::update_angles_from_orientation()
    {
        const glm::vec3 forward =
            orientation_ * glm::vec3{ 0.0f, 0.0f, -1.0f };
        const glm::vec3 offset =
            -safe_normalize(forward, { 0.0f, 0.0f, 1.0f });

        yawRadians_ = std::atan2(offset.x, offset.z);
        pitchRadians_ = std::asin(std::clamp(offset.y, -1.0f, 1.0f));
    }
}
