/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/appearance/VisualMaterialInstance.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/scene/RenderLayer.h"
#include "graphics/scene/RenderTransform.h"
#include "graphics/scene/RenderVisibility.h"

#include <cstdint>
#include <string>

namespace locus::graphics
{
    /**
     * @brief Lightweight scene entry consumed by the renderer.
     */
    struct RenderObject
    {
        /**
         * @brief Stable object identifier used by editor and scene systems.
         */
        using Id = std::uint64_t;

        /**
         * @brief Unique object identifier.
         */
        Id id = 0;

        /**
         * @brief Human-readable object name for tooling and diagnostics.
         */
        std::string name;

        /**
         * @brief Mesh submitted when drawing this object.
         */
        const GpuMesh* mesh = nullptr;

        /**
         * @brief Shader program used to render this object when no material is assigned.
         */
        const Shader* shader = nullptr;

        /**
         * @brief Optional material instance used to render this object.
         */
        const VisualMaterialInstance* material = nullptr;

        /**
         * @brief Object transform in render space.
         */
        RenderTransform transform;

        /**
         * @brief Visibility and selection participation flags.
         */
        RenderVisibility visibility;

        /**
         * @brief Logical render layer for filtering and ordering.
         */
        RenderLayer layer = RenderLayer::Default;

        /**
         * @brief True when the editor selection state targets this object.
         */
        bool selected = false;

        /**
         * @brief True when the cursor hover state targets this object.
         */
        bool hovered = false;

        /**
         * @brief True when the object should be drawn as wireframe.
         */
        bool wireframe = false;

        /**
         * @brief Checks whether the object has all resources required for drawing.
         *
         * @return True when visible and backed by valid mesh and shader resources.
         */
        [[nodiscard]] bool is_drawable() const;

        /**
         * @brief Resolves the shader used for drawing this object.
         *
         * @return Material shader when available, otherwise the fallback shader.
         */
        [[nodiscard]] const Shader* resolved_shader() const;

        /**
         * @brief Resolves the base color used by material-aware shaders.
         *
         * @return Material color when available, otherwise white.
         */
        [[nodiscard]] ColorRGBA resolved_color() const;

        /**
         * @brief Checks whether the shader should use per-vertex colors.
         *
         * @return True when vertex colors should be used.
         */
        [[nodiscard]] bool uses_vertex_color() const;
    };
}