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
     * @brief One world-space segment drawn with constant screen-space width.
     */
    struct ScreenSpaceLine
    {
        /**
         * @brief Segment start in world space.
         */
        glm::vec3 start{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Segment end in world space.
         */
        glm::vec3 end{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Linear RGBA color.
         */
        ColorRGBA color{ 1.0f, 1.0f, 1.0f, 1.0f };

        /**
         * @brief Line width in framebuffer pixels.
         */
        float widthPixels = 1.0f;
    };

    /**
     * @brief CPU-side batch of screen-space line segments.
     */
    struct ScreenSpaceLineBatch
    {
        /**
         * @brief Segment list.
         */
        std::vector<ScreenSpaceLine> lines;

        /**
         * @brief Checks whether the batch has any segments.
         *
         * @return True when at least one segment exists.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return lines.empty();
        }

        /**
         * @brief Returns the number of segments in the batch.
         *
         * @return Segment count.
         */
        [[nodiscard]] std::size_t size() const noexcept
        {
            return lines.size();
        }
    };

    /**
     * @brief Checks whether a line is finite, non-degenerate, and drawable.
     *
     * @param line Line to validate.
     * @return True when the line can be submitted safely.
     */
    [[nodiscard]] inline bool is_drawable(const ScreenSpaceLine& line) noexcept
    {
        const auto finite_vec3 = [](const glm::vec3& value) {
            return std::isfinite(value.x)
                && std::isfinite(value.y)
                && std::isfinite(value.z);
        };

        if (!finite_vec3(line.start) || !finite_vec3(line.end)) {
            return false;
        }

        if (!std::isfinite(line.widthPixels) || line.widthPixels <= 0.0f) {
            return false;
        }

        const glm::vec3 delta = line.end - line.start;
        return glm::dot(delta, delta) > 0.0000000001f;
    }
}
