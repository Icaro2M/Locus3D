/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Supported light source categories.
     */
    enum class LightType
    {
        /**
         * @brief Infinite light with direction but no position.
         */
        Directional,

        /**
         * @brief Local light with position and range.
         */
        Point
    };

    /**
     * @brief Lightweight light description used by viewport shading.
     */
    struct Light
    {
        /**
         * @brief Light source category.
         */
        LightType type = LightType::Directional;

        /**
         * @brief Direction used by directional lights.
         */
        glm::vec3 direction{ -0.35f, -0.75f, -0.45f };

        /**
         * @brief Position used by point lights.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Light color and alpha.
         */
        ColorRGBA color{ 1.0f, 1.0f, 1.0f, 1.0f };

        /**
         * @brief Light strength multiplier.
         */
        float intensity = 1.0f;

        /**
         * @brief Point-light influence radius.
         */
        float range = 10.0f;

        /**
         * @brief True when the light contributes to shading.
         */
        bool enabled = true;

        /**
         * @brief Creates a directional light.
         *
         * @param direction Light direction.
         * @param color Light color.
         * @param intensity Light strength multiplier.
         * @return Configured directional light.
         */
        [[nodiscard]] static Light directional(
            const glm::vec3& direction,
            const ColorRGBA& color,
            float intensity = 1.0f)
        {
            Light light;
            light.type = LightType::Directional;
            light.direction = direction;
            light.color = color;
            light.intensity = intensity;
            light.enabled = true;
            return light;
        }

        /**
         * @brief Creates a point light.
         *
         * @param position Light position.
         * @param color Light color.
         * @param intensity Light strength multiplier.
         * @param range Light influence radius.
         * @return Configured point light.
         */
        [[nodiscard]] static Light point(
            const glm::vec3& position,
            const ColorRGBA& color,
            float intensity = 1.0f,
            float range = 10.0f)
        {
            Light light;
            light.type = LightType::Point;
            light.position = position;
            light.color = color;
            light.intensity = intensity;
            light.range = range;
            light.enabled = true;
            return light;
        }
    };
}
