/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/renderer/Renderer.h"

namespace locus::graphics
{
    void Renderer::set_view_matrix(const glm::mat4& view)
    {
        viewMatrix_ = view;
    }

    void Renderer::set_projection_matrix(const glm::mat4& projection)
    {
        projectionMatrix_ = projection;
    }

    void Renderer::render(const RenderScene& scene)
    {
        stats_.reset();

        // Validate each object before touching GPU state so invalid editor entries are harmless.
        for (const RenderObject& object : scene.objects())
        {
            ++stats_.objectsSubmitted;

            if (!object.is_drawable())
            {
                ++stats_.objectsSkipped;
                continue;
            }

            render_object(object);
        }
    }

    const glm::mat4& Renderer::view_matrix() const
    {
        return viewMatrix_;
    }

    const glm::mat4& Renderer::projection_matrix() const
    {
        return projectionMatrix_;
    }

    const RenderStats& Renderer::stats() const
    {
        return stats_;
    }

    void Renderer::render_object(const RenderObject& object)
    {
        const glm::mat4 model = object.transform.matrix();
        const glm::mat4 mvp = projectionMatrix_ * viewMatrix_ * model;

        // The shader currently expects a single combined transform matrix.
        object.shader->bind();
        object.shader->set_mat4("u_MVP", &mvp[0][0]);
        object.mesh->draw();
        object.shader->unbind();

        ++stats_.objectsDrawn;
        ++stats_.drawCalls;
    }
}
