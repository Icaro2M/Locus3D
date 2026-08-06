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

#include <string>

#include <glm/vec2.hpp>

namespace locus::graphics
{
    /**
     * @brief Configuration used by the selection shape overlay renderer.
     */
    struct SelectionShapeRendererConfig
    {
        std::string shaderName = "viewport/selection_shape";
        ColorRGBA fillColor{ 0.20f, 0.55f, 1.0f, 0.16f };
        ColorRGBA borderColor{ 0.10f, 0.42f, 1.0f, 0.95f };
        float borderThicknessPixels = 1.5f;
        bool depthTest = false;
        bool depthWrite = false;
        bool blend = true;
    };

    /**
     * @brief Draw data for a rectangular selection region in viewport pixels.
     */
    struct SelectionRectangleDrawData
    {
        bool visible = false;
        glm::vec2 minimum{ 0.0f, 0.0f };
        glm::vec2 maximum{ 0.0f, 0.0f };
        glm::vec2 viewportOrigin{ 0.0f, 0.0f };
        glm::vec2 viewportSize{ 0.0f, 0.0f };
    };

    /**
     * @brief Renders 2D selection shape overlays on top of the viewport.
     */
    class SelectionShapeRenderer
    {
    public:
        SelectionShapeRenderer() = default;
        ~SelectionShapeRenderer();

        SelectionShapeRenderer(const SelectionShapeRenderer&) = delete;
        SelectionShapeRenderer& operator=(const SelectionShapeRenderer&) = delete;

        SelectionShapeRenderer(SelectionShapeRenderer&& other) noexcept;
        SelectionShapeRenderer& operator=(SelectionShapeRenderer&& other) noexcept;

        /**
         * @brief Initializes GPU resources and resolves the selection shape shader.
         *
         * @param shaderManager Shader registry that owns the overlay shader.
         * @param config Renderer configuration.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const SelectionShapeRendererConfig& config = {});

        /**
         * @brief Releases GPU resources.
         */
        void destroy();

        /**
         * @brief Stores the latest rectangle draw data.
         *
         * @param data Rectangle overlay state in viewport-local pixels.
         */
        void set_rectangle(const SelectionRectangleDrawData& data) noexcept;

        /**
         * @brief Draws the current selection rectangle overlay.
         */
        void render() const;

        /**
         * @brief Checks whether the renderer has all GPU resources.
         *
         * @return True when ready to draw.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Checks whether the current draw data can produce pixels.
         *
         * @return True when the overlay is visible and non-empty.
         */
        [[nodiscard]] bool has_drawable_rectangle() const noexcept;

    private:
        [[nodiscard]] GraphicsResult<void> create_vertex_layout();
        [[nodiscard]] GraphicsResult<void> upload_unit_quad();

    private:
        SelectionShapeRendererConfig config_{};
        SelectionRectangleDrawData rectangle_{};
        const Shader* shader_ = nullptr;
        VertexArray vertexArray_{};
        Buffer vertexBuffer_{};
    };
}
