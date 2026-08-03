/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Projection mode used to convert view space into clip space.
     */
    enum class ProjectionType
    {
        /**
         * @brief Perspective projection with foreshortening.
         */
        Perspective,

        /**
         * @brief Orthographic projection with constant object scale.
         */
        Orthographic
    };

    /**
     * @brief Stores projection parameters and builds projection matrices.
     */
    class Projection
    {
    public:
        /**
         * @brief Creates a default perspective projection.
         */
        Projection() = default;

        /**
         * @brief Destroys the projection state.
         */
        ~Projection() = default;

        /**
         * @brief Configures a perspective projection.
         *
         * @param verticalFovRadians Vertical field of view in radians.
         * @param aspectRatio Width divided by height.
         * @param nearPlane Near clipping plane distance.
         * @param farPlane Far clipping plane distance.
         */
        void set_perspective(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane);

        /**
         * @brief Configures an orthographic projection.
         *
         * @param height Vertical size of the orthographic view volume.
         * @param aspectRatio Width divided by height.
         * @param nearPlane Near clipping plane distance.
         * @param farPlane Far clipping plane distance.
         */
        void set_orthographic(float height, float aspectRatio, float nearPlane, float farPlane);

        /**
         * @brief Updates only the projection aspect ratio.
         *
         * @param aspectRatio Width divided by height.
         */
        void set_aspect_ratio(float aspectRatio);

        /**
         * @brief Returns the active projection mode.
         *
         * @return Projection type.
         */
        [[nodiscard]] ProjectionType type() const;

        /**
         * @brief Returns the current aspect ratio.
         *
         * @return Width divided by height.
         */
        [[nodiscard]] float aspect_ratio() const;

        /**
         * @brief Returns the near clipping plane distance.
         *
         * @return Near plane distance.
         */
        [[nodiscard]] float near_plane() const;

        /**
         * @brief Returns the far clipping plane distance.
         *
         * @return Far plane distance.
         */
        [[nodiscard]] float far_plane() const;

        /**
         * @brief Returns the perspective vertical field of view.
         *
         * @return Field of view in radians.
         */
        [[nodiscard]] float vertical_fov_radians() const;

        /**
         * @brief Returns the orthographic vertical size.
         *
         * @return Orthographic height in world units.
         */
        [[nodiscard]] float orthographic_height() const;

        /**
         * @brief Returns the minimum accepted orthographic height.
         *
         * @return Minimum positive height in world units.
         */
        [[nodiscard]] static float min_orthographic_height() noexcept;

        /**
         * @brief Returns the maximum accepted orthographic height.
         *
         * @return Maximum height in world units.
         */
        [[nodiscard]] static float max_orthographic_height() noexcept;

        /**
         * @brief Builds the projection matrix for the active mode.
         *
         * @return View-to-clip projection matrix.
         */
        [[nodiscard]] glm::mat4 matrix() const;

    private:
        ProjectionType type_ = ProjectionType::Perspective;

        float verticalFovRadians_ = 0.78539816339f;
        float orthographicHeight_ = 10.0f;

        float aspectRatio_ = 16.0f / 9.0f;
        float nearPlane_ = 0.01f;
        float farPlane_ = 1000.0f;
    };
}
