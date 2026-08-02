/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <glm/glm.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

namespace locus::graphics
{
    /**
     * @brief One world-space point drawn as a constant-size screen marker.
     */
    struct PointMarker
    {
        /**
         * @brief Marker center in world space.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Linear RGBA fill color.
         */
        ColorRGBA fillColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        /**
         * @brief Linear RGBA border color.
         */
        ColorRGBA borderColor{ 0.0f, 0.0f, 0.0f, 1.0f };

        /**
         * @brief Circle radius in framebuffer pixels.
         */
        float radiusPixels = 4.0f;

        /**
         * @brief Border width in framebuffer pixels.
         */
        float borderWidthPixels = 1.0f;
    };

    /**
     * @brief CPU-side batch of point markers.
     */
    struct PointMarkerBatch
    {
        /**
         * @brief Marker list.
         */
        std::vector<PointMarker> markers;

        /**
         * @brief Checks whether the batch has any markers.
         *
         * @return True when at least one marker exists.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return markers.empty();
        }

        /**
         * @brief Returns the number of markers in the batch.
         *
         * @return Marker count.
         */
        [[nodiscard]] std::size_t size() const noexcept
        {
            return markers.size();
        }
    };

    /**
     * @brief Checks whether a point marker is finite and drawable.
     *
     * @param marker Marker to validate.
     * @return True when the marker can be submitted safely.
     */
    [[nodiscard]] inline bool is_drawable(const PointMarker& marker) noexcept
    {
        const bool finitePosition =
            std::isfinite(marker.position.x) &&
            std::isfinite(marker.position.y) &&
            std::isfinite(marker.position.z);

        return finitePosition &&
            std::isfinite(marker.radiusPixels) &&
            marker.radiusPixels > 0.0f &&
            std::isfinite(marker.borderWidthPixels) &&
            marker.borderWidthPixels >= 0.0f;
    }
}
