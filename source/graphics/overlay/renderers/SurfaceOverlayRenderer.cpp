/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/SurfaceOverlayRenderer.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/RenderState.h"

#include <glad/glad.h>

#include <algorithm>
#include <utility>

namespace locus::graphics
{
    SurfaceOverlayRenderer::~SurfaceOverlayRenderer()
    {
        destroy();
    }

    SurfaceOverlayRenderer::SurfaceOverlayRenderer(
        SurfaceOverlayRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    SurfaceOverlayRenderer& SurfaceOverlayRenderer::operator=(
        SurfaceOverlayRenderer&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        shader_ = other.shader_;
        vertexArray_ = std::move(other.vertexArray_);
        vertexBuffer_ = std::move(other.vertexBuffer_);
        indexBuffer_ = std::move(other.indexBuffer_);
        vertices_ = std::move(other.vertices_);
        indices_ = std::move(other.indices_);
        modelMatrix_ = other.modelMatrix_;
        vertexCapacity_ = other.vertexCapacity_;
        indexCapacity_ = other.indexCapacity_;
        indexCount_ = other.indexCount_;

        other.shader_ = nullptr;
        other.vertexCapacity_ = 0;
        other.indexCapacity_ = 0;
        other.indexCount_ = 0;
        other.modelMatrix_ = glm::mat4{ 1.0f };
        other.vertices_.clear();
        other.indices_.clear();

        return *this;
    }

    GraphicsResult<void> SurfaceOverlayRenderer::create(
        const ShaderManager& shaderManager,
        const SurfaceOverlayRendererConfig& config)
    {
        destroy();

        const Shader* shader = shaderManager.find(config.shaderName);
        if (shader == nullptr) {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "SurfaceOverlayRenderer requires shader: " + config.shaderName + ".");
        }

        auto vertexArrayResult = vertexArray_.create();
        if (!vertexArrayResult) {
            return vertexArrayResult.error();
        }

        auto vertexBufferResult = vertexBuffer_.create(
            BufferType::Vertex,
            BufferUsage::Dynamic);
        if (!vertexBufferResult) {
            return vertexBufferResult.error();
        }

        auto indexBufferResult = indexBuffer_.create(
            BufferType::Index,
            BufferUsage::Dynamic);
        if (!indexBufferResult) {
            return indexBufferResult.error();
        }

        shader_ = shader;
        config_ = config;

        auto vertexCapacityResult = ensure_vertex_capacity(192u);
        if (!vertexCapacityResult) {
            destroy();
            return vertexCapacityResult.error();
        }

        auto indexCapacityResult = ensure_index_capacity(192u);
        if (!indexCapacityResult) {
            destroy();
            return indexCapacityResult.error();
        }

        return create_vertex_layout();
    }

    void SurfaceOverlayRenderer::destroy()
    {
        indexBuffer_.destroy();
        vertexBuffer_.destroy();
        vertexArray_.destroy();
        vertices_.clear();
        indices_.clear();
        shader_ = nullptr;
        modelMatrix_ = glm::mat4{ 1.0f };
        vertexCapacity_ = 0;
        indexCapacity_ = 0;
        indexCount_ = 0;
        config_ = {};
    }

    GraphicsResult<void> SurfaceOverlayRenderer::set_batch(
        const SurfaceOverlayBatch& batch)
    {
        if (shader_ == nullptr ||
            !vertexArray_.is_valid() ||
            !vertexBuffer_.is_valid() ||
            !indexBuffer_.is_valid()) {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload SurfaceOverlayRenderer because it was not created.");
        }

        vertices_.clear();
        indices_.clear();
        modelMatrix_ = batch.modelMatrix;
        vertices_.reserve(batch.vertices.size());
        indices_.reserve(batch.indices.size());

        for (std::size_t offset = 0; offset + 2u < batch.indices.size(); offset += 3u) {
            if (!is_drawable_triangle(batch, offset)) {
                continue;
            }

            const std::uint32_t base =
                static_cast<std::uint32_t>(vertices_.size());
            vertices_.push_back(make_vertex(batch.vertices[batch.indices[offset]]));
            vertices_.push_back(make_vertex(batch.vertices[batch.indices[offset + 1u]]));
            vertices_.push_back(make_vertex(batch.vertices[batch.indices[offset + 2u]]));
            indices_.push_back(base);
            indices_.push_back(base + 1u);
            indices_.push_back(base + 2u);
        }

        indexCount_ = indices_.size();

        if (indexCount_ == 0) {
            return {};
        }

        auto vertexCapacityResult = ensure_vertex_capacity(vertices_.size());
        if (!vertexCapacityResult) {
            return vertexCapacityResult.error();
        }

        auto indexCapacityResult = ensure_index_capacity(indices_.size());
        if (!indexCapacityResult) {
            return indexCapacityResult.error();
        }

        auto vertexUploadResult = vertexBuffer_.set_sub_data(
            vertices_.data(),
            vertices_.size() * sizeof(GpuVertex),
            0);
        if (!vertexUploadResult) {
            return vertexUploadResult.error();
        }

        return indexBuffer_.set_sub_data(
            indices_.data(),
            indices_.size() * sizeof(std::uint32_t),
            0);
    }

    void SurfaceOverlayRenderer::render(const glm::mat4& viewProjection) const
    {
        if (!is_valid() || indexCount_ == 0) {
            return;
        }

        RenderState::set_depth_test(config_.depthTest);
        RenderState::set_depth_write(config_.depthWrite);
        RenderState::set_depth_func(config_.depthFunc);
        RenderState::set_blend(config_.blend);
        RenderState::set_blend_func(
            BlendFactor::SourceAlpha,
            BlendFactor::OneMinusSourceAlpha);
        RenderState::set_cull_face(config_.cullFace);
        RenderState::set_polygon_mode(RenderPolygonMode::Fill);

        shader_->bind();
        const glm::mat4 mvp = viewProjection * modelMatrix_;
        shader_->set_mat4("u_MVP", &mvp[0][0]);

        vertexArray_.bind();
        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indexCount_),
            GL_UNSIGNED_INT,
            nullptr);
        vertexArray_.unbind();

        shader_->unbind();
        RenderState::reset_default();
    }

    bool SurfaceOverlayRenderer::is_valid() const
    {
        return shader_ != nullptr &&
            vertexArray_.is_valid() &&
            vertexBuffer_.is_valid() &&
            indexBuffer_.is_valid();
    }

    std::size_t SurfaceOverlayRenderer::index_count() const noexcept
    {
        return indexCount_;
    }

    std::size_t SurfaceOverlayRenderer::vertex_capacity() const noexcept
    {
        return vertexCapacity_;
    }

    std::size_t SurfaceOverlayRenderer::index_capacity() const noexcept
    {
        return indexCapacity_;
    }

    GraphicsResult<void> SurfaceOverlayRenderer::create_vertex_layout()
    {
        vertexArray_.bind();
        vertexBuffer_.bind();
        indexBuffer_.bind();

        const i32 stride = static_cast<i32>(sizeof(GpuVertex));

        auto positionResult = vertexArray_.set_attribute(
            VertexAttribute{
                0,
                3,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(GpuVertex, position)
            });
        if (!positionResult) {
            return positionResult.error();
        }

        auto colorResult = vertexArray_.set_attribute(
            VertexAttribute{
                1,
                4,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(GpuVertex, color)
            });
        if (!colorResult) {
            return colorResult.error();
        }

        vertexArray_.unbind();
        indexBuffer_.unbind();
        vertexBuffer_.unbind();

        return {};
    }

    GraphicsResult<void> SurfaceOverlayRenderer::ensure_vertex_capacity(
        std::size_t count)
    {
        if (count <= vertexCapacity_) {
            return {};
        }

        const std::size_t newCapacity =
            std::max<std::size_t>(count, vertexCapacity_ * 2u + 192u);
        auto result = vertexBuffer_.set_data(
            nullptr,
            newCapacity * sizeof(GpuVertex));

        if (!result) {
            return result.error();
        }

        vertexCapacity_ = newCapacity;
        return {};
    }

    GraphicsResult<void> SurfaceOverlayRenderer::ensure_index_capacity(
        std::size_t count)
    {
        if (count <= indexCapacity_) {
            return {};
        }

        const std::size_t newCapacity =
            std::max<std::size_t>(count, indexCapacity_ * 2u + 192u);
        auto result = indexBuffer_.set_data(
            nullptr,
            newCapacity * sizeof(std::uint32_t));

        if (!result) {
            return result.error();
        }

        indexCapacity_ = newCapacity;
        return {};
    }

    SurfaceOverlayRenderer::GpuVertex SurfaceOverlayRenderer::make_vertex(
        const SurfaceOverlayVertex& vertex) const noexcept
    {
        GpuVertex output{};
        output.position[0] = vertex.position.x;
        output.position[1] = vertex.position.y;
        output.position[2] = vertex.position.z;
        output.color[0] = vertex.color.r;
        output.color[1] = vertex.color.g;
        output.color[2] = vertex.color.b;
        output.color[3] = vertex.color.a;
        return output;
    }
}
