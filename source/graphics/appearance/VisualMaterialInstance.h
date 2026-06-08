/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/appearance/VisualMaterial.h"

#include <string>

namespace locus::graphics
{
    /**
     * @brief Per-object material binding with optional local overrides.
     */
    struct VisualMaterialInstance
    {
        /**
         * @brief Instance name used by editor and diagnostics.
         */
        std::string name;

        /**
         * @brief Shared material definition referenced by this instance.
         */
        const VisualMaterial* material = nullptr;

        /**
         * @brief Replacement color used when overrideColor is true.
         */
        ColorRGBA colorOverride{ 1.0f, 1.0f, 1.0f, 1.0f };

        /**
         * @brief True when colorOverride should replace the material base color.
         */
        bool overrideColor = false;

        /**
         * @brief Checks whether the referenced material can be drawn.
         *
         * @return True when a valid material is assigned.
         */
        [[nodiscard]] bool is_valid() const
        {
            return material != nullptr && material->is_valid();
        }

        /**
         * @brief Returns the shader from the assigned material.
         *
         * @return Shader pointer, or nullptr when no material is assigned.
         */
        [[nodiscard]] const Shader* shader() const
        {
            if (material == nullptr)
            {
                return nullptr;
            }

            return material->shader;
        }

        /**
         * @brief Resolves the effective instance color.
         *
         * @return Override color, material base color, or default color when unbound.
         */
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

        /**
         * @brief Checks whether the referenced material uses mesh vertex colors.
         *
         * @return True when vertex colors should be sampled.
         */
        [[nodiscard]] bool use_vertex_color() const
        {
            return material != nullptr && material->useVertexColor;
        }
    };
}
