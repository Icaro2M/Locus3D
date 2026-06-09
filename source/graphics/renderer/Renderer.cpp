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

    void Renderer::set_light_environment(const LightEnvironment* environment)
    {
        lightEnvironment_ = environment;
    }

    void Renderer::render(const RenderScene& scene)
    {
        RenderQueue queue;
        queue.build_from_scene(scene);
        queue.sort();

        render(queue);
    }

    void Renderer::render(const RenderQueue& queue)
    {
        stats_.reset();

        for (const RenderCommand& command : queue.commands())
        {
            ++stats_.objectsSubmitted;

            if (command.object == nullptr)
            {
                ++stats_.objectsSkipped;
                continue;
            }

            const RenderObject& object = *command.object;

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

    const LightEnvironment* Renderer::light_environment() const
    {
        return lightEnvironment_;
    }

    const RenderStats& Renderer::stats() const
    {
        return stats_;
    }

    void Renderer::render_object(const RenderObject& object)
    {
        const Shader* shader = object.resolved_shader();

        if (shader == nullptr)
        {
            ++stats_.objectsSkipped;
            return;
        }

        const glm::mat4 model = object.transform.matrix();
        const glm::mat4 mvp = projectionMatrix_ * viewMatrix_ * model;
        const ColorRGBA color = object.resolved_color();

        shader->bind();

        shader->set_mat4("u_Model", &model[0][0]);
        shader->set_mat4("u_MVP", &mvp[0][0]);
        shader->set_vec4("u_BaseColor", color.r, color.g, color.b, color.a);
        shader->set_int("u_UseVertexColor", object.uses_vertex_color() ? 1 : 0);

        apply_lighting_uniforms(*shader);

        object.mesh->draw();

        shader->unbind();

        ++stats_.objectsDrawn;
        ++stats_.drawCalls;
    }

    void Renderer::apply_lighting_uniforms(const Shader& shader) const
    {
        if (lightEnvironment_ == nullptr)
        {
            shader.set_int("u_ShadingMode", static_cast<int>(ShadingMode::Solid));
            shader.set_vec4("u_AmbientColor", 1.0f, 1.0f, 1.0f, 1.0f);
            shader.set_float("u_AmbientIntensity", 1.0f);
            shader.set_vec3("u_LightDirection", 0.0f, 0.0f, -1.0f);
            shader.set_vec4("u_LightColor", 1.0f, 1.0f, 1.0f, 1.0f);
            shader.set_float("u_LightIntensity", 0.0f);
            return;
        }

        const ColorRGBA& ambientColor = lightEnvironment_->ambient_color();

        shader.set_int("u_ShadingMode", static_cast<int>(lightEnvironment_->shading_mode()));
        shader.set_vec4(
            "u_AmbientColor",
            ambientColor.r,
            ambientColor.g,
            ambientColor.b,
            ambientColor.a
        );
        shader.set_float("u_AmbientIntensity", lightEnvironment_->ambient_intensity());

        const Light* light = lightEnvironment_->light(0);

        if (light == nullptr || !light->enabled)
        {
            shader.set_vec3("u_LightDirection", 0.0f, 0.0f, -1.0f);
            shader.set_vec4("u_LightColor", 1.0f, 1.0f, 1.0f, 1.0f);
            shader.set_float("u_LightIntensity", 0.0f);
            return;
        }

        shader.set_vec3(
            "u_LightDirection",
            light->direction.x,
            light->direction.y,
            light->direction.z
        );

        shader.set_vec4(
            "u_LightColor",
            light->color.r,
            light->color.g,
            light->color.b,
            light->color.a
        );

        shader.set_float("u_LightIntensity", light->intensity);
    }
}