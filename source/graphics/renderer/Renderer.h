/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/lighting/LightEnvironment.h"
#include "graphics/renderer/RenderStats.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Simple scene renderer that submits drawable objects to the GPU.
     */
    class Renderer
    {
    public:
        /**
         * @brief Creates a renderer with identity view and projection matrices.
         */
        Renderer() = default;

        /**
         * @brief Destroys the renderer state.
         */
        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        Renderer(Renderer&&) noexcept = default;
        Renderer& operator=(Renderer&&) noexcept = default;

        /**
         * @brief Sets the view transform used for subsequent draws.
         *
         * @param view World-to-view matrix.
         */
        void set_view_matrix(const glm::mat4& view);

        /**
         * @brief Sets the projection transform used for subsequent draws.
         *
         * @param projection View-to-clip matrix.
         */
        void set_projection_matrix(const glm::mat4& projection);

        /**
         * @brief Sets the lighting environment used by material-aware shaders.
         *
         * @param environment Lighting environment or nullptr to disable scene lighting uniforms.
         */
        void set_light_environment(const LightEnvironment* environment);

        /**
         * @brief Renders every drawable object in the scene.
         *
         * @param scene Scene to submit.
         */
        void render(const RenderScene& scene);

        /**
         * @brief Returns the current view matrix.
         *
         * @return World-to-view matrix.
         */
        [[nodiscard]] const glm::mat4& view_matrix() const;

        /**
         * @brief Returns the current projection matrix.
         *
         * @return View-to-clip matrix.
         */
        [[nodiscard]] const glm::mat4& projection_matrix() const;

        /**
         * @brief Returns the current lighting environment.
         *
         * @return Lighting environment or nullptr.
         */
        [[nodiscard]] const LightEnvironment* light_environment() const;

        /**
         * @brief Returns counters for the most recent render call.
         *
         * @return Read-only render statistics.
         */
        [[nodiscard]] const RenderStats& stats() const;

    private:
        void render_object(const RenderObject& object);
        void apply_lighting_uniforms(const Shader& shader) const;

    private:
        glm::mat4 viewMatrix_{ 1.0f };
        glm::mat4 projectionMatrix_{ 1.0f };
        const LightEnvironment* lightEnvironment_ = nullptr;
        RenderStats stats_{};
    };
}