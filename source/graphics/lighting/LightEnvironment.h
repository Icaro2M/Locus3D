/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "graphics/lighting/Light.h"
#include "graphics/lighting/ShadingMode.h"

#include <array>
#include <cstddef>

namespace locus::graphics
{
    /**
     * @brief Stores viewport lighting and shading settings.
     */
    class LightEnvironment
    {
    public:
        /**
         * @brief Maximum number of active lights supported by the environment.
         */
        static constexpr std::size_t MaxLights = 4;

        /**
         * @brief Creates a default light environment.
         */
        LightEnvironment() = default;

        /**
         * @brief Destroys the light environment state.
         */
        ~LightEnvironment() = default;

        /**
         * @brief Sets the active shading mode.
         *
         * @param mode New shading mode.
         */
        void set_shading_mode(ShadingMode mode)
        {
            shadingMode_ = mode;
        }

        /**
         * @brief Returns the active shading mode.
         *
         * @return Current shading mode.
         */
        [[nodiscard]] ShadingMode shading_mode() const
        {
            return shadingMode_;
        }

        /**
         * @brief Sets the ambient light color.
         *
         * @param color Ambient color.
         */
        void set_ambient_color(const ColorRGBA& color)
        {
            ambientColor_ = color;
        }

        /**
         * @brief Returns the ambient light color.
         *
         * @return Ambient color.
         */
        [[nodiscard]] const ColorRGBA& ambient_color() const
        {
            return ambientColor_;
        }

        /**
         * @brief Sets the ambient light intensity.
         *
         * @param intensity Ambient strength multiplier.
         */
        void set_ambient_intensity(float intensity)
        {
            ambientIntensity_ = intensity;
        }

        /**
         * @brief Returns the ambient light intensity.
         *
         * @return Ambient strength multiplier.
         */
        [[nodiscard]] float ambient_intensity() const
        {
            return ambientIntensity_;
        }

        /**
         * @brief Adds a light if capacity is available.
         *
         * @param light Light to append.
         * @return True when the light was stored.
         */
        bool add_light(const Light& light)
        {
            if (lightCount_ >= MaxLights)
            {
                return false;
            }

            lights_[lightCount_] = light;
            ++lightCount_;
            return true;
        }

        /**
         * @brief Replaces an existing light.
         *
         * @param index Light index to replace.
         * @param light Replacement light.
         */
        void set_light(std::size_t index, const Light& light)
        {
            if (index >= lightCount_)
            {
                return;
            }

            lights_[index] = light;
        }

        /**
         * @brief Returns a light by index.
         *
         * @param index Light index.
         * @return Light pointer, or nullptr when index is inactive.
         */
        [[nodiscard]] const Light* light(std::size_t index) const
        {
            if (index >= lightCount_)
            {
                return nullptr;
            }

            return &lights_[index];
        }

        /**
         * @brief Returns the fixed light storage.
         *
         * @return Read-only light array.
         */
        [[nodiscard]] const std::array<Light, MaxLights>& lights() const
        {
            return lights_;
        }

        /**
         * @brief Returns the number of active lights.
         *
         * @return Active light count.
         */
        [[nodiscard]] std::size_t light_count() const
        {
            return lightCount_;
        }

        /**
         * @brief Removes all active lights.
         */
        void clear_lights()
        {
            lightCount_ = 0;
        }

        /**
         * @brief Restores the default lighting used by the viewport.
         */
        void reset_default_viewport_lighting()
        {
            shadingMode_ = ShadingMode::Lit;
            ambientColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
            ambientIntensity_ = 0.35f;
            lightCount_ = 0;

            add_light(
                Light::directional(
                    { -0.35f, -0.75f, -0.45f },
                    { 1.0f, 1.0f, 1.0f, 1.0f },
                    0.85f
                )
            );
        }

    private:
        ShadingMode shadingMode_ = ShadingMode::Lit;
        ColorRGBA ambientColor_{ 1.0f, 1.0f, 1.0f, 1.0f };
        float ambientIntensity_ = 0.35f;

        std::array<Light, MaxLights> lights_{};
        std::size_t lightCount_ = 0;
    };
}
