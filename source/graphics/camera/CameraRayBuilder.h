/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/camera/Camera.h"
#include "graphics/common/GraphicsTypes.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief World-space ray emitted from a camera through a viewport pixel.
     */
    struct CameraRay
    {
        /**
         * @brief Ray origin in world space.
         */
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Normalized ray direction in world space.
         */
        glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
    };

    /**
     * @brief Utility for converting viewport pixels into camera rays.
     */
    class CameraRayBuilder
    {
    public:
        CameraRayBuilder() = delete;

        /**
         * @brief Builds a world-space picking ray from a viewport pixel.
         *
         * @param camera Camera used for view and projection transforms.
         * @param viewport Viewport rectangle in framebuffer coordinates.
         * @param pixelX Pixel X coordinate.
         * @param pixelY Pixel Y coordinate.
         * @return Ray passing through the requested pixel.
         */
        [[nodiscard]] static CameraRay from_viewport_pixel(
            const Camera& camera,
            const ViewportRect& viewport,
            float pixelX,
            float pixelY
        );

    private:
        [[nodiscard]] static glm::vec2 pixel_to_ndc(
            const ViewportRect& viewport,
            float pixelX,
            float pixelY
        );
    };
}
