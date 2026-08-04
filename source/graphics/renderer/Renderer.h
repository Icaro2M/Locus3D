/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/gpu/RenderState.h"
#include "graphics/lighting/LightEnvironment.h"
#include "graphics/renderer/RenderQueue.h"
#include "graphics/renderer/RenderStats.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Generic render-state policy for scene surface submission.
     */
    struct RendererSurfaceState
    {
        bool depthTest = true;
        bool depthWrite = true;
        DepthFunc depthFunc = DepthFunc::Less;
        bool colorWrite = true;
        bool blend = false;
        BlendFactor sourceBlend = BlendFactor::SourceAlpha;
        BlendFactor destinationBlend = BlendFactor::OneMinusSourceAlpha;
        float vertexAlphaMultiplier = 1.0f;
        bool cullFace = false;
        RenderPolygonMode polygonMode = RenderPolygonMode::Fill;
    };

    /**
     * @brief Diagnostic front/back surface coloring for scene rendering.
     */
    struct FaceOrientationDisplay
    {
        bool enabled = false;
        ColorRGBA frontColor{ 0.22f, 0.48f, 0.86f, 1.0f };
        ColorRGBA backColor{ 0.95f, 0.08f, 0.08f, 1.0f };
    };

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
         * @brief Sets diagnostic surface coloring for subsequent draw calls.
         *
         * @param display Face-orientation display state.
         */
        void set_face_orientation_display(
            const FaceOrientationDisplay& display) noexcept;

        /**
         * @brief Returns the current diagnostic face orientation display state.
         *
         * @return Read-only face orientation display state.
         */
        [[nodiscard]] const FaceOrientationDisplay&
            face_orientation_display() const noexcept;

        /**
         * @brief Renders every drawable object in the scene.
         *
         * @param scene Scene to submit.
         */
        void render(const RenderScene& scene);

        /**
         * @brief Renders every drawable object already stored in a sorted render queue.
         *
         * @param queue Queue containing render commands.
         */
        void render(const RenderQueue& queue);

        /**
         * @brief Renders every drawable object in a queue with an explicit surface state.
         *
         * @param queue Queue containing render commands.
         * @param state Render-state policy to apply for the full queue.
         */
        void render_with_state(
            const RenderQueue& queue,
            const RendererSurfaceState& state);

        /**
         * @brief Renders drawable scene surfaces into depth only.
         *
         * @param queue Queue containing the surfaces used as depth authority.
         */
        void render_depth_only(const RenderQueue& queue);

        /**
         * @brief Returns the render-state policy used by render_depth_only().
         *
         * @return Depth-only surface state.
         */
        [[nodiscard]] static RendererSurfaceState depth_only_surface_state()
            noexcept;

        /**
         * @brief Returns the render-state policy used by foreground viewport helpers.
         *
         * @return Foreground overlay surface state.
         */
        [[nodiscard]] static RendererSurfaceState foreground_overlay_state()
            noexcept;

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
        void apply_face_orientation_uniforms(const Shader& shader) const;
        void apply_surface_state(const RendererSurfaceState& state);

    private:
        glm::mat4 viewMatrix_{ 1.0f };
        glm::mat4 projectionMatrix_{ 1.0f };
        const LightEnvironment* lightEnvironment_ = nullptr;
        FaceOrientationDisplay faceOrientationDisplay_{};
        RenderStats stats_{};
        float vertexAlphaMultiplier_ = 1.0f;
    };
}
