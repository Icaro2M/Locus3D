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

#include <cstddef>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Visual state used by pivot marker rendering.
     */
    enum class PivotVisualState
    {
        Normal,
        Hovered,
        Active
    };

    /**
     * @brief One world-space object pivot drawn as a screen-space crosshair.
     */
    struct PivotDrawData
    {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        PivotVisualState state = PivotVisualState::Normal;
        bool visible = false;
    };

    /**
     * @brief Configuration used by the pivot marker overlay renderer.
     */
    struct PivotRendererConfig
    {
        std::string shaderName = "viewport/pivot_marker";
        float centerRadiusPixels = 3.0f;
        float gapPixels = 5.0f;
        float armLengthPixels = 9.0f;
        float armThicknessPixels = 1.6f;
        float hitPaddingPixels = 0.0f;
        bool depthTest = false;
        bool depthWrite = false;
        DepthFunc depthFunc = DepthFunc::LessEqual;
        bool blend = true;
        ColorRGBA normalColor{ 1.0f, 0.12f, 0.08f, 1.0f };
        ColorRGBA hoveredColor{ 1.0f, 0.34f, 0.28f, 1.0f };
        ColorRGBA activeColor{ 1.0f, 0.62f, 0.12f, 1.0f };
    };

    /**
     * @brief Renders object pivots as constant-pixel screen-facing crosshairs.
     */
    class PivotRenderer
    {
    public:
        PivotRenderer() = default;
        ~PivotRenderer();

        PivotRenderer(const PivotRenderer&) = delete;
        PivotRenderer& operator=(const PivotRenderer&) = delete;

        PivotRenderer(PivotRenderer&& other) noexcept;
        PivotRenderer& operator=(PivotRenderer&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const PivotRendererConfig& config = {});

        void destroy();

        [[nodiscard]] GraphicsResult<void> set_pivots(
            const std::vector<PivotDrawData>& pivots);

        void render(
            const glm::mat4& viewProjection,
            const ViewportRect& viewport) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] std::size_t pivot_count() const noexcept;
        [[nodiscard]] std::size_t capacity() const noexcept;

    private:
        struct PivotInstance
        {
            float position[3]{ 0.0f, 0.0f, 0.0f };
            float halfExtentPixels = 1.0f;
            float color[4]{ 1.0f, 0.0f, 0.0f, 1.0f };
            float centerRadiusPixels = 3.0f;
            float gapPixels = 5.0f;
            float armLengthPixels = 9.0f;
            float armThicknessPixels = 1.6f;
        };

        [[nodiscard]] GraphicsResult<void> create_instance_layout();
        [[nodiscard]] GraphicsResult<void> ensure_capacity(std::size_t count);
        [[nodiscard]] PivotInstance make_instance(
            const PivotDrawData& pivot) const noexcept;
        [[nodiscard]] ColorRGBA color_for(PivotVisualState state) const noexcept;
        [[nodiscard]] float half_extent_pixels() const noexcept;

        PivotRendererConfig config_{};
        const Shader* shader_ = nullptr;
        VertexArray vertexArray_{};
        Buffer instanceBuffer_{};
        std::vector<PivotInstance> instances_{};
        std::size_t capacity_ = 0;
        std::size_t pivotCount_ = 0;
    };
}
