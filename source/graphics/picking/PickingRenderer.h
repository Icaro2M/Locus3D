#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/picking/PickingBuffer.h"
#include "graphics/renderer/RenderQueue.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    class ShaderManager;

    class PickingRenderer
    {
    public:
        PickingRenderer() = default;
        ~PickingRenderer() = default;

        PickingRenderer(const PickingRenderer&) = delete;
        PickingRenderer& operator=(const PickingRenderer&) = delete;

        PickingRenderer(PickingRenderer&&) noexcept = default;
        PickingRenderer& operator=(PickingRenderer&&) noexcept = default;

        [[nodiscard]] GraphicsResult<void> create(const ShaderManager& shaderManager);

        void set_view_matrix(const glm::mat4& view);
        void set_projection_matrix(const glm::mat4& projection);

        void render(
            PickingBuffer& pickingBuffer,
            const RenderScene& scene
        ) const;

        void render(
            PickingBuffer& pickingBuffer,
            const RenderQueue& queue
        ) const;

        [[nodiscard]] bool is_valid() const;

    private:
        void render_object(const RenderObject& object) const;

    private:
        const Shader* shader_ = nullptr;

        glm::mat4 viewMatrix_{ 1.0f };
        glm::mat4 projectionMatrix_{ 1.0f };
    };
}