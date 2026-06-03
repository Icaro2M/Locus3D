#include "graphics/mesh/GpuMesh.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <cstddef>
#include <utility>

namespace locus::graphics
{

    GpuMesh::~GpuMesh()
    {
        destroy();
    }

    GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    {
        *this = std::move(other);
    }

    GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        vertexArray_ = std::move(other.vertexArray_);
        vertexBuffer_ = std::move(other.vertexBuffer_);
        indexBuffer_ = std::move(other.indexBuffer_);
        drawData_ = other.drawData_;

        other.drawData_ = {};

        return *this;
    }

    GraphicsResult<void> GpuMesh::create(const MeshUploadData& uploadData)
    {
        if (uploadData.is_empty())
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot create GpuMesh from empty MeshUploadData.");
        }

        auto vertexBufferResult = create_vertex_buffer(uploadData);

        if (!vertexBufferResult)
        {
            return vertexBufferResult.error();
        }

        if (uploadData.has_indices())
        {
            auto indexBufferResult = create_index_buffer(uploadData);

            if (!indexBufferResult)
            {
                return indexBufferResult.error();
            }
        }

        auto vertexArrayResult = create_vertex_array();

        if (!vertexArrayResult)
        {
            return vertexArrayResult.error();
        }

        drawData_.topology = uploadData.topology;
        drawData_.indexed = uploadData.has_indices();
        drawData_.vertexCount = static_cast<u32>(uploadData.vertices.size());
        drawData_.indexCount = static_cast<u32>(uploadData.indices.size());
        drawData_.indexType = IndexType::UInt32;

        return {};
    }

    void GpuMesh::destroy()
    {
        indexBuffer_.destroy();
        vertexBuffer_.destroy();
        vertexArray_.destroy();

        drawData_ = {};
    }

    void GpuMesh::draw() const
    {
        if (!is_valid())
        {
            return;
        }

        vertexArray_.bind();

        if (drawData_.indexed)
        {
            glDrawElements(
                gl_topology(),
                static_cast<GLsizei>(drawData_.indexCount),
                gl_index_type(),
                nullptr);
        }
        else
        {
            glDrawArrays(
                gl_topology(),
                0,
                static_cast<GLsizei>(drawData_.vertexCount));
        }

        vertexArray_.unbind();
    }

    bool GpuMesh::is_valid() const
    {
        return vertexArray_.is_valid() && vertexBuffer_.is_valid();
    }

    const MeshDrawData& GpuMesh::draw_data() const
    {
        return drawData_;
    }

    GraphicsResult<void> GpuMesh::create_vertex_buffer(const MeshUploadData& uploadData)
    {
        auto createResult = vertexBuffer_.create(
            BufferType::Vertex,
            uploadData.usage);

        if (!createResult)
        {
            return createResult.error();
        }

        return vertexBuffer_.set_data(
            uploadData.vertices.data(),
            uploadData.vertices.size() * sizeof(MeshVertex));
    }

    GraphicsResult<void> GpuMesh::create_index_buffer(const MeshUploadData& uploadData)
    {
        auto createResult = indexBuffer_.create(
            BufferType::Index,
            uploadData.usage);

        if (!createResult)
        {
            return createResult.error();
        }

        return indexBuffer_.set_data(
            uploadData.indices.data(),
            uploadData.indices.size() * sizeof(u32));
    }

    GraphicsResult<void> GpuMesh::create_vertex_array()
    {
        auto createResult = vertexArray_.create();

        if (!createResult)
        {
            return createResult.error();
        }

        vertexArray_.bind();
        vertexBuffer_.bind();

        auto positionAttributeResult = vertexArray_.set_attribute(
            VertexAttribute{
                0,
                3,
                VertexAttributeType::Float,
                false,
                static_cast<i32>(sizeof(MeshVertex)),
                offsetof(MeshVertex, position)
            });

        if (!positionAttributeResult)
        {
            return positionAttributeResult.error();
        }

        auto normalAttributeResult = vertexArray_.set_attribute(
            VertexAttribute{
                1,
                3,
                VertexAttributeType::Float,
                false,
                static_cast<i32>(sizeof(MeshVertex)),
                offsetof(MeshVertex, normal)
            });

        if (!normalAttributeResult)
        {
            return normalAttributeResult.error();
        }

        auto colorAttributeResult = vertexArray_.set_attribute(
            VertexAttribute{
                2,
                4,
                VertexAttributeType::Float,
                false,
                static_cast<i32>(sizeof(MeshVertex)),
                offsetof(MeshVertex, color)
            });

        if (!colorAttributeResult)
        {
            return colorAttributeResult.error();
        }

        if (indexBuffer_.is_valid())
        {
            indexBuffer_.bind();
        }

        vertexArray_.unbind();
        vertexBuffer_.unbind();

        return {};
    }

    u32 GpuMesh::gl_topology() const
    {
        switch (drawData_.topology)
        {
        case PrimitiveTopology::Points:
            return GL_POINTS;

        case PrimitiveTopology::Lines:
            return GL_LINES;

        case PrimitiveTopology::LineStrip:
            return GL_LINE_STRIP;

        case PrimitiveTopology::Triangles:
            return GL_TRIANGLES;

        case PrimitiveTopology::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        }

        return GL_TRIANGLES;
    }

    u32 GpuMesh::gl_index_type() const
    {
        switch (drawData_.indexType)
        {
        case IndexType::UInt16:
            return GL_UNSIGNED_SHORT;

        case IndexType::UInt32:
            return GL_UNSIGNED_INT;
        }

        return GL_UNSIGNED_INT;
    }

}