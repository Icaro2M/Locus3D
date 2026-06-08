/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Shader.h"

#include <string>

namespace locus::graphics
{
    /**
     * @brief High-level rendering mode for a viewport material.
     */
    enum class VisualMaterialMode
    {
        /**
         * @brief Uses a uniform base color.
         */
        Solid,

        /**
         * @brief Uses per-vertex color data from the mesh.
         */
        VertexColor,

        /**
         * @brief Draws the object as wireframe geometry.
         */
        Wireframe,

        /**
         * @brief Draws an emphasized selection or hover material.
         */
        Highlight
    };

    /**
     * @brief Shared visual material state used by render objects.
     */
    struct VisualMaterial
    {
        /**
         * @brief Material name used by libraries and tooling.
         */
        std::string name;

        /**
         * @brief Shader program used to draw instances of this material.
         */
        const Shader* shader = nullptr;

        /**
         * @brief Base RGBA color used when vertex colors are disabled.
         */
        ColorRGBA baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        /**
         * @brief Rendering mode selected for this material.
         */
        VisualMaterialMode mode = VisualMaterialMode::VertexColor;

        /**
         * @brief True when mesh vertex colors should contribute to shading.
         */
        bool useVertexColor = true;

        /**
         * @brief True when depth testing should be enabled while drawing.
         */
        bool depthTest = true;

        /**
         * @brief True when back-face culling should be disabled.
         */
        bool doubleSided = false;

        /**
         * @brief Checks whether the material has a usable shader.
         *
         * @return True when the shader pointer is valid and compiled.
         */
        [[nodiscard]] bool is_valid() const;
    };
}
