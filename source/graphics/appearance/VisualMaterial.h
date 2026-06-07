#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Shader.h"

#include <string>

namespace locus::graphics
{
    enum class VisualMaterialMode
    {
        Solid,
        VertexColor,
        Wireframe,
        Highlight
    };

    struct VisualMaterial
    {
        std::string name;
        const Shader* shader = nullptr;
        ColorRGBA baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        VisualMaterialMode mode = VisualMaterialMode::VertexColor;
        bool useVertexColor = true;
        bool depthTest = true;
        bool doubleSided = false;

        [[nodiscard]] bool is_valid() const;
    };
}