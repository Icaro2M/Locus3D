/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <cstddef>

namespace locus::editor {

    /**
     * @brief Options used when converting render-ready geometry into graphics data.
     */
    struct RenderMeshUploadOptions {
        /**
         * @brief Vertex color written into generated graphics vertices.
         */
        graphics::ColorRGBA color{ 1.0f, 1.0f, 1.0f, 1.0f };

        /**
         * @brief GPU usage hint assigned to the generated upload data.
         */
        graphics::BufferUsage usage = graphics::BufferUsage::Static;

        /**
         * @brief Primitive topology assigned to generated triangle upload data.
         */
        graphics::PrimitiveTopology triangleTopology = graphics::PrimitiveTopology::Triangles;

        /**
         * @brief Primitive topology assigned to generated line upload data.
         */
        graphics::PrimitiveTopology lineTopology = graphics::PrimitiveTopology::Lines;
    };

    /**
     * @brief Statistics produced when converting kernel render meshes to graphics upload data.
     */
    struct RenderMeshUploadResult {
        /**
         * @brief Number of source render vertices copied.
         */
        std::size_t vertexCount = 0;

        /**
         * @brief Number of source render triangles copied.
         */
        std::size_t triangleCount = 0;

        /**
         * @brief Number of source render lines copied.
         */
        std::size_t lineCount = 0;

        /**
         * @brief Number of generated graphics indices.
         */
        std::size_t indexCount = 0;

        /**
         * @brief Checks whether triangle upload data was generated.
         *
         * @return True when at least one triangle was emitted.
         */
        [[nodiscard]] bool has_triangles() const
        {
            return triangleCount > 0;
        }

        /**
         * @brief Checks whether line upload data was generated.
         *
         * @return True when at least one line was emitted.
         */
        [[nodiscard]] bool has_lines() const
        {
            return lineCount > 0;
        }
    };

} // namespace locus::editor