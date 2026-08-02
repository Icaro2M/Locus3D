/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/appearance/ViewportPalette.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/VertexArray.h"

namespace locus::graphics
{
    class Shader;
    class ShaderManager;
    class Texture;

    /**
     * @brief Configuration for screen-space object outline composition.
     */
    struct ObjectOutlinePassConfig
    {
        ColorRGBA hoveredColor{ 1.0f, 0.85f, 0.20f, 0.70f };
        float hoveredWidthPixels = 2.0f;
        ColorRGBA selectedColor{ 1.0f, 0.55f, 0.05f, 0.95f };
        float selectedWidthPixels = 3.5f;
    };

    /**
     * @brief Composes object outlines from a selection mask onto the active framebuffer.
     */
    class ObjectOutlinePass
    {
    public:
        ObjectOutlinePass() = default;
        ~ObjectOutlinePass();

        ObjectOutlinePass(const ObjectOutlinePass&) = delete;
        ObjectOutlinePass& operator=(const ObjectOutlinePass&) = delete;

        ObjectOutlinePass(ObjectOutlinePass&& other) noexcept;
        ObjectOutlinePass& operator=(ObjectOutlinePass&& other) noexcept;

        /**
         * @brief Creates fullscreen resources and resolves the outline shader.
         *
         * @param shaderManager Shader registry.
         * @return Empty result on success, or a graphics error.
         */
        [[nodiscard]] GraphicsResult<void> create(const ShaderManager& shaderManager);

        /**
         * @brief Releases GPU resources.
         */
        void destroy();

        /**
         * @brief Updates visual style used by subsequent draws.
         *
         * @param config Outline style.
         */
        void set_config(const ObjectOutlinePassConfig& config) noexcept;

        /**
         * @brief Composes the outline over the active framebuffer.
         *
         * @param maskTexture Selection mask texture.
         * @param viewport Active viewport rectangle.
         */
        void render(const Texture& maskTexture, const ViewportRect& viewport) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] const ObjectOutlinePassConfig& config() const noexcept;

    private:
        ObjectOutlinePassConfig config_{};
        const Shader* shader_ = nullptr;
        VertexArray fullscreenVertexArray_{};
    };
}
