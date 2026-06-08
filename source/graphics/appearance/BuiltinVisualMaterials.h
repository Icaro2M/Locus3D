/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/appearance/VisualMaterial.h"

namespace locus::graphics
{
    /**
     * @brief Factory helpers for common viewport material presets.
     */
    namespace BuiltinVisualMaterials
    {
        /**
         * @brief Creates a solid-color material.
         *
         * @param shader Shader used by the material.
         * @param color Base material color.
         * @param name Material name.
         * @return Configured visual material.
         */
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

        /**
         * @brief Creates a material that uses mesh vertex colors.
         *
         * @param shader Shader used by the material.
         * @param name Material name.
         * @return Configured visual material.
         */
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

        /**
         * @brief Creates a highlight material for selected or hovered objects.
         *
         * @param shader Shader used by the material.
         * @param color Highlight color.
         * @param name Material name.
         * @return Configured visual material.
         */
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

        /**
         * @brief Creates a double-sided wireframe material.
         *
         * @param shader Shader used by the material.
         * @param color Wireframe color.
         * @param name Material name.
         * @return Configured visual material.
         */
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
