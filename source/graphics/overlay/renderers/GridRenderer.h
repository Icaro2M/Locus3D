/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/camera/Camera.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/scene/RenderObject.h"

namespace locus::graphics
{
    /**
     * @brief Runtime configuration for the infinite-style viewport grid.
     */
    struct GridRendererConfig
    {
        /**
         * @brief Half-size of the grid mesh generated around the origin.
         */
        float halfExtent = 500.0f;

        /**
         * @brief Distance between minor grid lines.
         */
        float minorSpacing = 1.0f;

        /**
         * @brief Distance between emphasized major grid lines.
         */
        float majorSpacing = 5.0f;

        /**
         * @brief Camera-relative distance where grid fading begins.
         */
        float fadeStart = 80.0f;

        /**
         * @brief Camera-relative distance where grid fading ends.
         */
        float fadeEnd = 280.0f;

        /**
         * @brief Visibility multiplier for minor lines.
         */
        float lineStrength = 0.65f;

        /**
         * @brief Visibility multiplier for major lines.
         */
        float majorLineStrength = 0.95f;

        /**
         * @brief Color used by minor grid lines.
         */
        ColorRGBA minorColor{ 0.24f, 0.24f, 0.27f, 1.0f };

        /**
         * @brief Color used by major grid lines.
         */
        ColorRGBA majorColor{ 0.38f, 0.38f, 0.42f, 1.0f };
    };

    /**
     * @brief Builds and owns the render object used for the viewport grid.
     */
    class GridRenderer
    {
    public:
        /**
         * @brief Creates an empty grid renderer.
         */
        GridRenderer() = default;

        /**
         * @brief Releases grid GPU resources.
         */
        ~GridRenderer();

        GridRenderer(const GridRenderer&) = delete;
        GridRenderer& operator=(const GridRenderer&) = delete;

        GridRenderer(GridRenderer&& other) noexcept;
        GridRenderer& operator=(GridRenderer&& other) noexcept;

        /**
         * @brief Creates mesh and render object state using a managed grid shader.
         *
         * @param uploader Mesh uploader used to create GPU buffers.
         * @param shaderManager Shader manager that owns the viewport grid shader.
         * @param config Grid appearance and sizing parameters.
         * @return Success or creation error.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const MeshUploader& uploader,
            const ShaderManager& shaderManager,
            const GridRendererConfig& config
        );

        /**
         * @brief Repositions the grid around the camera.
         *
         * @param camera Camera used to determine the snapped grid origin.
         */
        void update(const Camera& camera);

        /**
         * @brief Destroys all owned GPU resources.
         */
        void destroy();

        /**
         * @brief Checks whether the grid render object can be drawn.
         *
         * @return True when mesh and shader resources are valid.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the render object submitted for the grid.
         *
         * @return Read-only render object.
         */
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        [[nodiscard]] static MeshUploadData build_mesh_data(float halfExtent);

    private:
        GridRendererConfig config_{};
        GpuMesh mesh_;
        RenderObject object_{};
    };
}