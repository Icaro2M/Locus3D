/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/scene/RenderObject.h"

#include <glm/glm.hpp>

namespace locus::graphics
{
    /**
     * @brief Runtime configuration for viewport axis guide lines.
     */
    struct AxisRendererConfig
    {
        /**
         * @brief Horizontal length of X and Z axis lines in each direction.
         */
        float extent = 500.0f;

        /**
         * @brief Vertical length of the Y axis line in each direction.
         */
        float verticalExtent = 40.0f;

        /**
         * @brief Small Y offset used to avoid overlap with the grid plane.
         */
        float planeOffset = 0.003f;

        /**
         * @brief Color used for the X axis.
         */
        ColorRGBA xColor{ 0.95f, 0.16f, 0.14f, 1.0f };

        /**
         * @brief Color used for the Y axis.
         */
        ColorRGBA yColor{ 0.20f, 0.95f, 0.25f, 1.0f };

        /**
         * @brief Color used for the Z axis.
         */
        ColorRGBA zColor{ 0.20f, 0.42f, 1.0f, 1.0f };
    };

    /**
     * @brief Builds and owns the render object used for viewport axis guides.
     */
    class AxisRenderer
    {
    public:
        /**
         * @brief Creates an empty axis renderer.
         */
        AxisRenderer() = default;

        /**
         * @brief Releases axis GPU resources.
         */
        ~AxisRenderer();

        AxisRenderer(const AxisRenderer&) = delete;
        AxisRenderer& operator=(const AxisRenderer&) = delete;

        AxisRenderer(AxisRenderer&& other) noexcept;
        AxisRenderer& operator=(AxisRenderer&& other) noexcept;

        /**
         * @brief Creates shader, mesh, and render object state.
         *
         * @param uploader Mesh uploader used to create GPU buffers.
         * @param config Axis sizing and color parameters.
         * @return Success or creation error.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const MeshUploader& uploader,
            const AxisRendererConfig& config
        );

        /**
         * @brief Destroys all owned GPU resources.
         */
        void destroy();

        /**
         * @brief Checks whether the axis render object can be drawn.
         *
         * @return True when mesh and shader resources are valid.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the render object submitted for the axes.
         *
         * @return Read-only render object.
         */
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        [[nodiscard]] static MeshUploadData build_mesh_data(const AxisRendererConfig& config);
        [[nodiscard]] static const char* vertex_shader_source();
        [[nodiscard]] static const char* fragment_shader_source();

        static void add_line(
            MeshUploadData& data,
            const glm::vec3& a,
            const glm::vec3& b,
            const ColorRGBA& color
        );

    private:
        AxisRendererConfig config_{};
        Shader shader_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}
