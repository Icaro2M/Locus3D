#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Buffer.h"
#include "graphics/gpu/VertexArray.h"
#include "graphics/mesh/MeshDrawData.h"
#include "graphics/mesh/MeshUploadData.h"

namespace locus::graphics
{

    class GpuMesh
    {
    public:
        GpuMesh() = default;
        ~GpuMesh();

        GpuMesh(const GpuMesh&) = delete;
        GpuMesh& operator=(const GpuMesh&) = delete;

        GpuMesh(GpuMesh&& other) noexcept;
        GpuMesh& operator=(GpuMesh&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(const MeshUploadData& uploadData);
        void destroy();

        void draw() const;

        [[nodiscard]] bool is_valid() const;
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