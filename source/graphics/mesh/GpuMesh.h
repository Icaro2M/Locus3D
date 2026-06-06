/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Buffer.h"
#include "graphics/gpu/VertexArray.h"
#include "graphics/mesh/MeshDrawData.h"
#include "graphics/mesh/MeshUploadData.h"

namespace locus::graphics
{

    /**
     * @brief Owns GPU buffers and vertex-array state for a mesh.
     */
    class GpuMesh
    {
    public:
        /**
         * @brief Creates an empty mesh wrapper.
         */
        GpuMesh() = default;

        /**
         * @brief Releases owned GPU resources.
         */
        ~GpuMesh();

        GpuMesh(const GpuMesh&) = delete;
        GpuMesh& operator=(const GpuMesh&) = delete;

        GpuMesh(GpuMesh&& other) noexcept;
        GpuMesh& operator=(GpuMesh&& other) noexcept;

        /**
         * @brief Uploads mesh data and creates the required GPU objects.
         *
         * @param uploadData CPU-side mesh data and usage hints.
         * @return Success or upload error.
         */
        [[nodiscard]] GraphicsResult<void> create(const MeshUploadData& uploadData);

        /**
         * @brief Destroys all owned GPU resources and resets draw data.
         */
        void destroy();

        /**
         * @brief Draws the mesh using its configured topology.
         */
        void draw() const;

        /**
         * @brief Checks whether the mesh has the GPU resources required for drawing.
         *
         * @return True when the vertex array and vertex buffer are valid.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the draw parameters cached during upload.
         *
         * @return Read-only draw data.
         */
        [[nodiscard]] const MeshDrawData& draw_data() const;

    private:
        [[nodiscard]] GraphicsResult<void> create_vertex_buffer(const MeshUploadData& uploadData);
        [[nodiscard]] GraphicsResult<void> create_index_buffer(const MeshUploadData& uploadData);
        [[nodiscard]] GraphicsResult<void> create_vertex_array();

        [[nodiscard]] u32 gl_topology() const;
        [[nodiscard]] u32 gl_index_type() const;

    private:
        VertexArray vertexArray_;
        Buffer vertexBuffer_;
        Buffer indexBuffer_;

        MeshDrawData drawData_{};
    };

}
