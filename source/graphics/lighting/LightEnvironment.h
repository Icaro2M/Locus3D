#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "graphics/lighting/Light.h"
#include "graphics/lighting/ShadingMode.h"

#include <array>
#include <cstddef>

namespace locus::graphics
{
    class LightEnvironment
    {
    public:
        static constexpr std::size_t MaxLights = 4;

        LightEnvironment() = default;
        ~LightEnvironment() = default;

        void set_shading_mode(ShadingMode mode)
        {
            shadingMode_ = mode;
        }

        [[nodiscard]] ShadingMode shading_mode() const
        {
            return shadingMode_;
        }

        void set_ambient_color(const ColorRGBA& color)
        {
            ambientColor_ = color;
        }

        [[nodiscard]] const ColorRGBA& ambient_color() const
        {
            return ambientColor_;
        }

        void set_ambient_intensity(float intensity)
        {
            ambientIntensity_ = intensity;
        }

        [[nodiscard]] float ambient_intensity() const
        {
            return ambientIntensity_;
        }

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

        void set_light(std::size_t index, const Light& light)
        {
            if (index >= lightCount_)
            {
                return;
            }

            lights_[index] = light;
        }

        [[nodiscard]] const Light* light(std::size_t index) const
        {
            if (index >= lightCount_)
            {
                return nullptr;
            }

            return &lights_[index];
        }

        [[nodiscard]] const std::array<Light, MaxLights>& lights() const
        {
            return lights_;
        }

        [[nodiscard]] std::size_t light_count() const
        {
            return lightCount_;
        }

        void clear_lights()
        {
            lightCount_ = 0;
        }

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