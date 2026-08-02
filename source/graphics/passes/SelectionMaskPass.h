/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Framebuffer.h"
#include "graphics/primitives/ObjectHighlight.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    class Shader;
    class ShaderManager;

    /**
     * @brief Renders object highlight IDs into a depth-aware offscreen mask.
     */
    class SelectionMaskPass
    {
    public:
        SelectionMaskPass() = default;
        ~SelectionMaskPass();

        SelectionMaskPass(const SelectionMaskPass&) = delete;
        SelectionMaskPass& operator=(const SelectionMaskPass&) = delete;

        SelectionMaskPass(SelectionMaskPass&& other) noexcept;
        SelectionMaskPass& operator=(SelectionMaskPass&& other) noexcept;

        /**
         * @brief Resolves the mask shader from the shader manager.
         *
         * @param shaderManager Shader registry.
         * @return Empty result on success, or a graphics error.
         */
        [[nodiscard]] GraphicsResult<void> create(const ShaderManager& shaderManager);

        /**
         * @brief Releases GPU resources owned by the pass.
         */
        void destroy();

        /**
         * @brief Ensures the mask framebuffer matches the viewport dimensions.
         *
         * Non-positive dimensions destroy the framebuffer and leave the pass idle.
         *
         * @param width Width in pixels.
         * @param height Height in pixels.
         * @return Empty result on success, or a graphics error.
         */
        [[nodiscard]] GraphicsResult<void> resize(i32 width, i32 height);

        /**
         * @brief Renders the depth-aware object highlight mask.
         *
         * @param scene Visible scene used as the depth authority.
         * @param highlights Objects that should write mask IDs.
         * @param view View matrix.
         * @param projection Projection matrix.
         */
        void render(
            const RenderScene& scene,
            const ObjectHighlightBatch& highlights,
            const glm::mat4& view,
            const glm::mat4& projection) const;

        /**
         * @brief Checks whether the pass has all resources required for drawing.
         */
        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] i32 width() const noexcept;
        [[nodiscard]] i32 height() const noexcept;
        [[nodiscard]] u64 resource_revision() const noexcept;
        [[nodiscard]] const Texture* mask_texture() const;

    private:
        void render_depth_object(
            const RenderObject& object,
            const glm::mat4& view,
            const glm::mat4& projection) const;

        void render_highlight(
            const ObjectHighlight& highlight,
            const glm::mat4& view,
            const glm::mat4& projection) const;

        [[nodiscard]] static ColorRGBA encode_mask(
            u32 maskId,
            ObjectHighlightCategory category) noexcept;

    private:
        Framebuffer framebuffer_{};
        const Shader* shader_ = nullptr;
        u64 resourceRevision_ = 0;
    };
}
