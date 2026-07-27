/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/ApplicationResult.h"
#include "application/document/DocumentId.h"
#include "graphics/camera/OrbitCameraRig.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/mesh/MeshRenderCache.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/overlay/renderers/AxisRenderer.h"
#include "graphics/overlay/renderers/GridRenderer.h"
#include "graphics/renderer/RenderQueue.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/viewport/Viewport.h"

#include <cstdint>
#include <filesystem>

namespace locus::application {

    class DocumentSession;

    /**
     * @brief Composes the primary visual viewport for an editor document.
     *
     * EditorViewport owns viewport-specific camera and GPU presentation
     * resources. The document and its synchronized RenderScene remain owned by
     * DocumentSession and are only borrowed during render().
     */
    class EditorViewport {
    public:
        /**
         * @brief Creates an uninitialized editor viewport.
         */
        EditorViewport() = default;

        /**
         * @brief Releases viewport-owned GPU resources.
         */
        ~EditorViewport();

        EditorViewport(const EditorViewport&) = delete;
        EditorViewport& operator=(const EditorViewport&) = delete;
        EditorViewport(EditorViewport&&) = delete;
        EditorViewport& operator=(EditorViewport&&) = delete;

        /**
         * @brief Initializes visual resources for the current graphics context.
         *
         * @param framebufferWidth Initial framebuffer width in pixels.
         * @param framebufferHeight Initial framebuffer height in pixels.
         * @param shaderRoot Directory containing the shader asset tree.
         * @return Success or an application initialization error.
         */
        [[nodiscard]] ApplicationResult<void> initialize(
            std::int32_t framebufferWidth,
            std::int32_t framebufferHeight,
            const std::filesystem::path& shaderRoot = "assets/shaders");

        /**
         * @brief Releases all viewport-owned GPU resources.
         *
         * Calling this method more than once is safe. A valid graphics context
         * must remain current until the first call completes.
         */
        void shutdown();

        /**
         * @brief Checks whether the viewport is ready to render.
         *
         * @return True after successful initialization and before shutdown.
         */
        [[nodiscard]] bool initialized() const noexcept;

        /**
         * @brief Updates the viewport from framebuffer dimensions.
         *
         * Non-positive dimensions are clamped by graphics::Viewport so
         * minimized windows remain safe.
         *
         * @param framebufferWidth Framebuffer width in pixels.
         * @param framebufferHeight Framebuffer height in pixels.
         */
        void resize(
            std::int32_t framebufferWidth,
            std::int32_t framebufferHeight);

        /**
         * @brief Orbits the camera from a logical cursor drag.
         *
         * @param deltaX Horizontal cursor delta in logical pixels.
         * @param deltaY Vertical cursor delta in logical pixels.
         */
        void orbit_camera(double deltaX, double deltaY);

        /**
         * @brief Pans the camera from a logical cursor drag.
         *
         * @param deltaX Horizontal cursor delta in logical pixels.
         * @param deltaY Vertical cursor delta in logical pixels.
         */
        void pan_camera(double deltaX, double deltaY);

        /**
         * @brief Zooms the camera from vertical scroll input.
         *
         * @param scrollDelta Vertical scroll offset.
         */
        void zoom_camera(double scrollDelta);

        /**
         * @brief Synchronizes and renders one document into this viewport.
         *
         * @param document Document temporarily presented by the viewport.
         * @return Success or an application runtime error.
         */
        [[nodiscard]] ApplicationResult<void> render(
            DocumentSession& document);

        /**
         * @brief Returns the graphics viewport and its owned camera.
         *
         * @return Mutable graphics viewport reference.
         */
        [[nodiscard]] graphics::Viewport& viewport() noexcept;

        /**
         * @brief Returns the graphics viewport and its owned camera.
         *
         * @return Read-only graphics viewport reference.
         */
        [[nodiscard]] const graphics::Viewport& viewport() const noexcept;

        /**
         * @brief Returns the orbit rig applied before each render.
         *
         * @return Mutable orbit rig reference.
         */
        [[nodiscard]] graphics::OrbitCameraRig& orbit_rig() noexcept;

        /**
         * @brief Returns the orbit rig applied before each render.
         *
         * @return Read-only orbit rig reference.
         */
        [[nodiscard]] const graphics::OrbitCameraRig&
            orbit_rig() const noexcept;

        /**
         * @brief Returns renderer diagnostics for the latest frame.
         *
         * @return Read-only renderer reference.
         */
        [[nodiscard]] const graphics::Renderer& renderer() const noexcept;

        /**
         * @brief Returns the current framebuffer aspect ratio.
         *
         * @return Width divided by height.
         */
        [[nodiscard]] float aspect_ratio() const noexcept;

    private:
        graphics::ShaderManager shaderManager_{};
        graphics::MeshUploader meshUploader_{};
        graphics::MeshRenderCache meshCache_{};
        graphics::GridRenderer gridRenderer_{};
        graphics::AxisRenderer axisRenderer_{};
        graphics::Viewport viewport_{};
        graphics::OrbitCameraRig orbitRig_{};
        graphics::Renderer renderer_{};
        graphics::RenderQueue renderQueue_{};
        const graphics::Shader* documentShader_ = nullptr;
        DocumentId activeDocumentId_{};
        bool initialized_ = false;
    };

} // namespace locus::application
