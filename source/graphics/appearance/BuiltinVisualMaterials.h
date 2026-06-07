#pragma once

#include "graphics/appearance/VisualMaterial.h"

namespace locus::graphics
{
    namespace BuiltinVisualMaterials
    {
        inline VisualMaterial solid(
            const Shader* shader,
            const ColorRGBA& color,
            const std::string& name = "Solid")
        {
            VisualMaterial material;
            material.name = name;
            material.shader = shader;
            material.baseColor = color;
            material.mode = VisualMaterialMode::Solid;
            material.useVertexColor = false;
            material.depthTest = true;
            material.doubleSided = false;
            return material;
        }

        inline VisualMaterial vertex_color(
            const Shader* shader,
            const std::string& name = "VertexColor")
        {
            VisualMaterial material;
            material.name = name;
            material.shader = shader;
            material.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            material.mode = VisualMaterialMode::VertexColor;
            material.useVertexColor = true;
            material.depthTest = true;
            material.doubleSided = false;
            return material;
        }

        inline VisualMaterial highlight(
            const Shader* shader,
            const ColorRGBA& color,
            const std::string& name = "Highlight")
        {
            VisualMaterial material;
            material.name = name;
            material.shader = shader;
            material.baseColor = color;
            material.mode = VisualMaterialMode::Highlight;
            material.useVertexColor = false;
            material.depthTest = true;
            material.doubleSided = false;
            return material;
        }

        inline VisualMaterial wireframe(
            const Shader* shader,
            const ColorRGBA& color,
            const std::string& name = "Wireframe")
        {
            VisualMaterial material;
            material.name = name;
            material.shader = shader;
            material.baseColor = color;
            material.mode = VisualMaterialMode::Wireframe;
            material.useVertexColor = false;
            material.depthTest = true;
            material.doubleSided = true;
            return material;
        }
    }
}