#include "graphics/picking/PickingRenderer.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/scene/RenderObject.h"

#include <glad/glad.h>

namespace locus::graphics
{
    GraphicsResult<void> PickingRenderer::create(const ShaderManager& shaderManager)
    {
        shader_ = shaderManager.find("picking/object");

        if (shader_ == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "PickingRenderer could not find shader 'picking/object'."
            );
        }

        return {};
    }

    void PickingRenderer::set_view_matrix(const glm::mat4& view)
    {
        viewMatrix_ = view;
    }

    void PickingRenderer::set_projection_matrix(const glm::mat4& projection)
    {
        projectionMatrix_ = projection;
    }

    void PickingRenderer::render(
        PickingBuffer& pickingBuffer,
        const RenderScene& scene
    ) const
    {
        RenderQueue queue;
        queue.build_from_scene(scene);
        queue.sort();

        render(pickingBuffer, queue);
    }

    void PickingRenderer::render(
        PickingBuffer& pickingBuffer,
        const RenderQueue& queue
    ) const
    {
        if (!is_valid() || !pickingBuffer.is_valid())
        {
            return;
        }

        pickingBuffer.bind();
        pickingBuffer.clear();

        glViewport(
            0,
            0,
            pickingBuffer.width(),
            pickingBuffer.height()
        );

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        for (const RenderCommand& command : queue.commands())
        {
            if (command.object == nullptr)
            {
                continue;
            }

            render_object(*command.object);
        }

        PickingBuffer::bind_default();
    }

    bool PickingRenderer::is_valid() const
    {
        return shader_ != nullptr && shader_->is_valid();
    }

    void PickingRenderer::render_object(const RenderObject& object) const
    {
        if (!object.is_drawable())
        {
            return;
        }

        if (object.id == 0)
        {
            return;
        }

        const PickingId pickingId = PickingId::from_u32(
            static_cast<u32>(object.id & 0x00FFFFFFu)
        );

        if (!pickingId.is_valid())
        {
            return;
        }

        const ColorRGBA pickingColor = encode_picking_id(pickingId);

        const glm::mat4 model = object.transform.matrix();
        const glm::mat4 mvp = projectionMatrix_ * viewMatrix_ * model;

        shader_->bind();

        shader_->set_mat4("u_MVP", &mvp[0][0]);
        shader_->set_vec4(
            "u_PickingColor",
            pickingColor.r,
            pickingColor.g,
            pickingColor.b,
            pickingColor.a
        );

        object.mesh->draw();

        shader_->unbind();
    }
}