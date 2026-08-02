/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/gpu/Buffer.h"
#include "graphics/gpu/RenderState.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/gpu/VertexArray.h"
#include "graphics/primitives/SurfaceOverlay.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Configuration used by the surface overlay renderer.
     */
    struct SurfaceOverlayRendererConfig
    {
        std::string shaderName = "viewport/surface_overlay";
        bool depthTest = true;
        bool depthWrite = false;
        DepthFunc depthFunc = DepthFunc::LessEqual;
        bool blend = true;
        bool cullFace = false;
    };

    /**
     * @brief Renders translucent world-space triangle overlays.
     */
    class SurfaceOverlayRenderer
    {
    public:
        SurfaceOverlayRenderer() = default;
        ~SurfaceOverlayRenderer();

        SurfaceOverlayRenderer(const SurfaceOverlayRenderer&) = delete;
        SurfaceOverlayRenderer& operator=(const SurfaceOverlayRenderer&) = delete;

        SurfaceOverlayRenderer(SurfaceOverlayRenderer&& other) noexcept;
        SurfaceOverlayRenderer& operator=(SurfaceOverlayRenderer&& other) noexcept;

        /**
         * @brief Initializes GPU resources and resolves the surface overlay shader.
         *
         * @param shaderManager Shader registry that owns the overlay shader.
         * @param config Renderer configuration.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const SurfaceOverlayRendererConfig& config = {});

        /**
         * @brief Releases GPU resources.
         */
        void destroy();

        /**
         * @brief Uploads a sanitized surface overlay batch.
         *
         * @param batch Source overlay batch.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> set_batch(
            const SurfaceOverlayBatch& batch);

        /**
         * @brief Draws the uploaded overlay triangles.
         *
         * @param viewProjection World-to-clip matrix.
         */
        void render(const glm::mat4& viewProjection) const;

        /**
         * @brief Checks whether the renderer has all GPU resources.
         *
         * @return True when ready to draw.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the number of drawable uploaded indices.
         *
         * @return Index count.
         */
        [[nodiscard]] std::size_t index_count() const noexcept;

        /**
         * @brief Returns the current vertex-buffer capacity.
         *
         * @return Capacity in vertices.
         */
        [[nodiscard]] std::size_t vertex_capacity() const noexcept;

        /**
         * @brief Returns the current index-buffer capacity.
         *
         * @return Capacity in indices.
         */
        [[nodiscard]] std::size_t index_capacity() const noexcept;

    private:
        struct GpuVertex
        {
            float position[3]{ 0.0f, 0.0f, 0.0f };
            float color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
        };

        [[nodiscard]] GraphicsResult<void> create_vertex_layout();
        [[nodiscard]] GraphicsResult<void> ensure_vertex_capacity(std::size_t count);
        [[nodiscard]] GraphicsResult<void> ensure_index_capacity(std::size_t count);
        [[nodiscard]] GpuVertex make_vertex(const SurfaceOverlayVertex& vertex) const noexcept;

    private:
        SurfaceOverlayRendererConfig config_{};
        const Shader* shader_ = nullptr;
        VertexArray vertexArray_{};
        Buffer vertexBuffer_{};
        Buffer indexBuffer_{};
        std::vector<GpuVertex> vertices_{};
        std::vector<std::uint32_t> indices_{};
        glm::mat4 modelMatrix_{ 1.0f };
        std::size_t vertexCapacity_ = 0;
        std::size_t indexCapacity_ = 0;
        std::size_t indexCount_ = 0;
    };
}
