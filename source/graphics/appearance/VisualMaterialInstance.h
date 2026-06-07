#pragma once

#include "graphics/appearance/VisualMaterial.h"

#include <string>

namespace locus::graphics
{
    struct VisualMaterialInstance
    {
        std::string name;
        const VisualMaterial* material = nullptr;
        ColorRGBA colorOverride{ 1.0f, 1.0f, 1.0f, 1.0f };
        bool overrideColor = false;

        [[nodiscard]] bool is_valid() const
        {
            return material != nullptr && material->is_valid();
        }

        [[nodiscard]] const Shader* shader() const
        {
            if (material == nullptr)
            {
                return nullptr;
            }

            return material->shader;
        }

        [[nodiscard]] ColorRGBA color() const
        {
            if (overrideColor)
            {
                return colorOverride;
            }

            if (material == nullptr)
            {
                return {};
            }

            return material->baseColor;
        }

        [[nodiscard]] bool use_vertex_color() const
        {
            return material != nullptr && material->useVertexColor;
        }
    };
}