#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    enum class LightType
    {
        Directional,
        Point
    };

    struct Light
    {
        LightType type = LightType::Directional;

        glm::vec3 direction{ -0.35f, -0.75f, -0.45f };
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        ColorRGBA color{ 1.0f, 1.0f, 1.0f, 1.0f };

        float intensity = 1.0f;
        float range = 10.0f;

        bool enabled = true;

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