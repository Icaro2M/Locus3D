/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <vector>

namespace locus::graphics
{

    /**
     * @brief CPU-side vertex layout expected by GpuMesh.
     */
    struct MeshVertex
    {
        /**
         * @brief Vertex position in object space.
         */
        f32 position[3]{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Vertex normal in object space.
         */
        f32 normal[3]{ 0.0f, 0.0f, 1.0f };

        /**
         * @brief Vertex color as RGBA components.
         */
        f32 color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    /**
     * @brief CPU-side mesh payload used to create a GpuMesh.
     */
    struct MeshUploadData
    {
        /**
         * @brief Vertex buffer contents.
         */
        std::vector<MeshVertex> vertices;

        /**
         * @brief Optional 32-bit index buffer contents.
         */
        std::vector<u32> indices;

        /**
         * @brief Primitive topology used for drawing.
         */
        PrimitiveTopology topology = PrimitiveTopology::Triangles;

        /**
         * @brief Expected GPU buffer update frequency.
         */
        BufferUsage usage = BufferUsage::Static;

        /**
         * @brief Checks whether an index buffer should be uploaded.
         *
         * @return True when at least one index is present.
         */
        [[nodiscard]] bool has_indices() const
        {
            return !indices.empty();
        }

        /**
         * @brief Checks whether the upload payload has no vertices.
         *
         * @return True when the vertex list is empty.
         */
        [[nodiscard]] bool is_empty() const
        {
            return vertices.empty();
        }
    };

}
