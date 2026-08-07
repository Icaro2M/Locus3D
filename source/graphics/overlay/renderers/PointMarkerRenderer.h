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
#include "graphics/primitives/PointMarker.h"

#include <cstddef>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Configuration used by the point marker overlay renderer.
     */
    struct PointMarkerRendererConfig
    {
        std::string shaderName = "viewport/point_marker";
        float maxRadiusPixels = 48.0f;
        bool depthTest = true;
        bool depthWrite = false;
        DepthFunc depthFunc = DepthFunc::LessEqual;
        bool blend = true;
    };

    /**
     * @brief Renders world-space points as instanced circular screen-space billboards.
     */
    class PointMarkerRenderer
    {
    public:
        PointMarkerRenderer() = default;
        ~PointMarkerRenderer();

        PointMarkerRenderer(const PointMarkerRenderer&) = delete;
        PointMarkerRenderer& operator=(const PointMarkerRenderer&) = delete;

        PointMarkerRenderer(PointMarkerRenderer&& other) noexcept;
        PointMarkerRenderer& operator=(PointMarkerRenderer&& other) noexcept;

        /**
         * @brief Initializes GPU resources and resolves the point marker shader.
         *
         * @param shaderManager Shader registry that owns the marker shader.
         * @param config Renderer configuration.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const PointMarkerRendererConfig& config = {});

        /**
         * @brief Releases GPU resources.
         */
        void destroy();

        /**
         * @brief Uploads a sanitized marker batch.
         *
         * @param batch Source marker batch.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> set_markers(
            const PointMarkerBatch& batch);

        /**
         * @brief Draws the uploaded markers.
         *
         * @param viewProjection World-to-clip matrix.
         * @param viewport Viewport rectangle in framebuffer pixels.
         */
        void render(
            const glm::mat4& viewProjection,
            const ViewportRect& viewport) const;

        /**
         * @brief Draws the uploaded markers with explicit depth state.
         *
         * @param viewProjection World-to-clip matrix.
         * @param viewport Viewport rectangle in framebuffer pixels.
         * @param depthFunc Depth comparison used by this draw.
         * @param depthTest True when the scene depth buffer should be tested.
         */
        void render(
            const glm::mat4& viewProjection,
            const ViewportRect& viewport,
            DepthFunc depthFunc,
            bool depthTest) const;

        /**
         * @brief Checks whether the renderer has all GPU resources.
         *
         * @return True when ready to draw.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the number of drawable uploaded marker instances.
         *
         * @return Instance count.
         */
        [[nodiscard]] std::size_t marker_count() const noexcept;

        /**
         * @brief Returns the current instance-buffer capacity.
         *
         * @return Capacity in markers.
         */
        [[nodiscard]] std::size_t capacity() const noexcept;

    private:
        struct MarkerInstance
        {
            float position[3]{ 0.0f, 0.0f, 0.0f };
            float radiusPixels = 1.0f;
            float fillColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
            float borderColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
            float borderWidthPixels = 0.0f;
            float padding[3]{ 0.0f, 0.0f, 0.0f };
        };

        [[nodiscard]] GraphicsResult<void> create_instance_layout();
        [[nodiscard]] GraphicsResult<void> ensure_capacity(std::size_t count);
        [[nodiscard]] MarkerInstance make_instance(const PointMarker& marker) const noexcept;

    private:
        PointMarkerRendererConfig config_{};
        const Shader* shader_ = nullptr;
        VertexArray vertexArray_{};
        Buffer instanceBuffer_{};
        std::vector<MarkerInstance> instances_{};
        std::size_t capacity_ = 0;
        std::size_t markerCount_ = 0;
    };
}
