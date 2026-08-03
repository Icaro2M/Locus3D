/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Buffer.h"
#include "graphics/gpu/RenderState.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/gpu/VertexArray.h"
#include "graphics/primitives/ScreenSpaceLine.h"

#include <cstddef>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Configuration used by the screen-space line overlay renderer.
     */
    struct ScreenSpaceLineRendererConfig
    {
        std::string shaderName = "viewport/screen_space_line";
        float maxWidthPixels = 24.0f;
        bool depthTest = true;
        bool depthWrite = false;
        DepthFunc depthFunc = DepthFunc::LessEqual;
        bool blend = true;
    };

    /**
     * @brief Renders world-space segments as camera-projected quads with pixel width.
     */
    class ScreenSpaceLineRenderer
    {
    public:
        ScreenSpaceLineRenderer() = default;
        ~ScreenSpaceLineRenderer();

        ScreenSpaceLineRenderer(const ScreenSpaceLineRenderer&) = delete;
        ScreenSpaceLineRenderer& operator=(const ScreenSpaceLineRenderer&) = delete;

        ScreenSpaceLineRenderer(ScreenSpaceLineRenderer&& other) noexcept;
        ScreenSpaceLineRenderer& operator=(ScreenSpaceLineRenderer&& other) noexcept;

        /**
         * @brief Initializes GPU resources and resolves the line shader.
         *
         * @param shaderManager Shader registry that owns the line shader.
         * @param config Renderer configuration.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const ScreenSpaceLineRendererConfig& config = {});

        /**
         * @brief Releases GPU resources.
         */
        void destroy();

        /**
         * @brief Uploads a sanitized segment batch.
         *
         * @param batch Source line batch.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> set_lines(
            const ScreenSpaceLineBatch& batch);

        /**
         * @brief Draws the uploaded lines.
         *
         * @param viewProjection World-to-clip matrix.
         * @param viewport Viewport rectangle in framebuffer pixels.
         */
        void render(
            const glm::mat4& viewProjection,
            const ViewportRect& viewport) const;

        /**
         * @brief Draws the uploaded lines with a per-pass depth function.
         *
         * @param viewProjection World-to-clip matrix.
         * @param viewport Viewport rectangle in framebuffer pixels.
         * @param depthFunc Depth comparison used by this draw.
         */
        void render(
            const glm::mat4& viewProjection,
            const ViewportRect& viewport,
            DepthFunc depthFunc) const;

        /**
         * @brief Checks whether the renderer has all GPU resources.
         *
         * @return True when ready to draw.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the number of drawable uploaded line instances.
         *
         * @return Instance count.
         */
        [[nodiscard]] std::size_t line_count() const noexcept;

        /**
         * @brief Returns the current instance-buffer capacity.
         *
         * @return Capacity in lines.
         */
        [[nodiscard]] std::size_t capacity() const noexcept;

    private:
        struct LineInstance
        {
            float start[3]{ 0.0f, 0.0f, 0.0f };
            float widthPixels = 1.0f;
            float end[3]{ 0.0f, 0.0f, 0.0f };
            float padding = 0.0f;
            float color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
        };

        [[nodiscard]] GraphicsResult<void> create_instance_layout();
        [[nodiscard]] GraphicsResult<void> ensure_capacity(std::size_t count);
        [[nodiscard]] LineInstance make_instance(const ScreenSpaceLine& line) const noexcept;

    private:
        ScreenSpaceLineRendererConfig config_{};
        const Shader* shader_ = nullptr;
        VertexArray vertexArray_{};
        Buffer instanceBuffer_{};
        std::vector<LineInstance> instances_{};
        std::size_t capacity_ = 0;
        std::size_t lineCount_ = 0;
    };
}
