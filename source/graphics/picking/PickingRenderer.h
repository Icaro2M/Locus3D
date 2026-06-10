/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/picking/PickingBuffer.h"
#include "graphics/renderer/RenderQueue.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    class ShaderManager;

    /**
     * @brief Renders selectable objects into a picking buffer.
     */
    class PickingRenderer
    {
    public:
        /**
         * @brief Creates an empty picking renderer.
         */
        PickingRenderer() = default;

        /**
         * @brief Destroys the picking renderer state.
         */
        ~PickingRenderer() = default;

        PickingRenderer(const PickingRenderer&) = delete;
        PickingRenderer& operator=(const PickingRenderer&) = delete;

        PickingRenderer(PickingRenderer&&) noexcept = default;
        PickingRenderer& operator=(PickingRenderer&&) noexcept = default;

        /**
         * @brief Resolves the picking shader from a shader manager.
         *
         * @param shaderManager Shader registry.
         * @return Success or missing-shader error.
         */
        [[nodiscard]] GraphicsResult<void> create(const ShaderManager& shaderManager);

        /**
         * @brief Sets the view transform used by the picking pass.
         *
         * @param view World-to-view matrix.
         */
        void set_view_matrix(const glm::mat4& view);

        /**
         * @brief Sets the projection transform used by the picking pass.
         *
         * @param projection View-to-clip matrix.
         */
        void set_projection_matrix(const glm::mat4& projection);

        /**
         * @brief Builds a queue from a scene and renders picking IDs.
         *
         * @param pickingBuffer Target picking buffer.
         * @param scene Scene to render.
         */
        void render(
            PickingBuffer& pickingBuffer,
            const RenderScene& scene
        ) const;

        /**
         * @brief Renders queued objects into a picking buffer.
         *
         * @param pickingBuffer Target picking buffer.
         * @param queue Queue to render.
         */
        void render(
            PickingBuffer& pickingBuffer,
            const RenderQueue& queue
        ) const;

        /**
         * @brief Checks whether the picking renderer has a valid shader.
         *
         * @return True when the picking shader is available.
         */
        [[nodiscard]] bool is_valid() const;

    private:
        void render_object(const RenderObject& object) const;

    private:
        const Shader* shader_ = nullptr;

        glm::mat4 viewMatrix_{ 1.0f };
        glm::mat4 projectionMatrix_{ 1.0f };
    };
}
