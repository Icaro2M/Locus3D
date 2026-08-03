/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/camera/Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace locus::graphics
{
    /**
     * @brief Orbit-style camera controller for viewport navigation.
     */
    class OrbitCameraRig
    {
    public:
        /**
         * @brief Creates a rig centered on the world origin.
         */
        OrbitCameraRig();

        /**
         * @brief Destroys the rig state.
         */
        ~OrbitCameraRig() = default;

        /**
         * @brief Sets the orbit target point.
         *
         * @param target World-space point to orbit around.
         */
        void set_target(const glm::vec3& target);

        /**
         * @brief Sets the camera distance from the target.
         *
         * @param distance Requested distance in world units.
         */
        void set_distance(float distance);

        /**
         * @brief Sets absolute orbit angles.
         *
         * @param yawRadians Horizontal orbit angle in radians.
         * @param pitchRadians Vertical orbit angle in radians.
         */
        void set_angles(float yawRadians, float pitchRadians);

        /**
         * @brief Sets the exact camera orientation used by the rig.
         *
         * @param orientation World-space camera orientation.
         */
        void set_orientation(const glm::quat& orientation);

        /**
         * @brief Points the camera along a direction with a stable up vector.
         *
         * @param forward Desired world-space view direction.
         * @param up Preferred world-space up direction.
         */
        void look(const glm::vec3& forward, const glm::vec3& up);

        /**
         * @brief Rotates the rig around its target.
         *
         * @param deltaYawRadians Horizontal angle delta in radians.
         * @param deltaPitchRadians Vertical angle delta in radians.
         */
        void orbit(float deltaYawRadians, float deltaPitchRadians);

        /**
         * @brief Moves the target along the camera plane.
         *
         * @param deltaRight Offset along camera right.
         * @param deltaUp Offset along camera up.
         */
        void pan(float deltaRight, float deltaUp);

        /**
         * @brief Changes the camera distance from the target.
         *
         * @param deltaDistance Distance delta in world units.
         */
        void zoom(float deltaDistance);

        /**
         * @brief Applies the rig position and orientation to a camera.
         *
         * @param camera Camera to update.
         */
        void apply(Camera& camera) const;

        /**
         * @brief Returns the current orbit target.
         *
         * @return Target point in world space.
         */
        [[nodiscard]] const glm::vec3& target() const;

        /**
         * @brief Returns the current camera distance.
         *
         * @return Distance in world units.
         */
        [[nodiscard]] float distance() const;

        /**
         * @brief Returns the current yaw angle.
         *
         * @return Yaw in radians.
         */
        [[nodiscard]] float yaw_radians() const;

        /**
         * @brief Returns the current pitch angle.
         *
         * @return Pitch in radians.
         */
        [[nodiscard]] float pitch_radians() const;

        /**
         * @brief Returns the current camera orientation.
         *
         * @return World-space camera orientation.
         */
        [[nodiscard]] const glm::quat& orientation() const;

    private:
        void update_orientation_from_angles();
        void update_angles_from_orientation();

        glm::vec3 target_{ 0.0f, 0.0f, 0.0f };
        glm::quat orientation_{ 1.0f, 0.0f, 0.0f, 0.0f };
        float distance_ = 5.0f;
        float yawRadians_ = 0.0f;
        float pitchRadians_ = 0.35f;
        float minDistance_ = 0.05f;
        float maxDistance_ = 1000.0f;
    };
}
