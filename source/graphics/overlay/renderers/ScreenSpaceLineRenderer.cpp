/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/ScreenSpaceLineRenderer.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/RenderState.h"

#include <glad/glad.h>

#include <algorithm>
#include <utility>

namespace locus::graphics
{
    ScreenSpaceLineRenderer::~ScreenSpaceLineRenderer()
    {
        destroy();
    }

    ScreenSpaceLineRenderer::ScreenSpaceLineRenderer(ScreenSpaceLineRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    ScreenSpaceLineRenderer& ScreenSpaceLineRenderer::operator=(
        ScreenSpaceLineRenderer&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        shader_ = other.shader_;
        vertexArray_ = std::move(other.vertexArray_);
        instanceBuffer_ = std::move(other.instanceBuffer_);
        instances_ = std::move(other.instances_);
        capacity_ = other.capacity_;
        lineCount_ = other.lineCount_;

        other.shader_ = nullptr;
        other.capacity_ = 0;
        other.lineCount_ = 0;
        other.instances_.clear();

        return *this;
    }

    GraphicsResult<void> ScreenSpaceLineRenderer::create(
        const ShaderManager& shaderManager,
        const ScreenSpaceLineRendererConfig& config)
    {
        destroy();

        const Shader* shader = shaderManager.find(config.shaderName);
        if (shader == nullptr) {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "ScreenSpaceLineRenderer requires shader: " + config.shaderName + ".");
        }

        auto vertexArrayResult = vertexArray_.create();
        if (!vertexArrayResult) {
            return vertexArrayResult.error();
        }

        auto bufferResult = instanceBuffer_.create(
            BufferType::Vertex,
            BufferUsage::Dynamic);
        if (!bufferResult) {
            return bufferResult.error();
        }

        shader_ = shader;
        config_ = config;

        auto capacityResult = ensure_capacity(64u);
        if (!capacityResult) {
            destroy();
            return capacityResult.error();
        }

        return create_instance_layout();
    }

    void ScreenSpaceLineRenderer::destroy()
    {
        instanceBuffer_.destroy();
        vertexArray_.destroy();
        instances_.clear();
        shader_ = nullptr;
        capacity_ = 0;
        lineCount_ = 0;
        config_ = {};
    }

    GraphicsResult<void> ScreenSpaceLineRenderer::set_lines(
        const ScreenSpaceLineBatch& batch)
    {
        if (shader_ == nullptr || !vertexArray_.is_valid() || !instanceBuffer_.is_valid()) {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload ScreenSpaceLineRenderer because it was not created.");
        }

        instances_.clear();
        instances_.reserve(batch.lines.size());

        for (const ScreenSpaceLine& line : batch.lines) {
            if (!is_drawable(line)) {
                continue;
            }

            instances_.push_back(make_instance(line));
        }

        lineCount_ = instances_.size();

        if (lineCount_ == 0) {
            return {};
        }

        auto capacityResult = ensure_capacity(lineCount_);
        if (!capacityResult) {
            return capacityResult.error();
        }

        return instanceBuffer_.set_sub_data(
            instances_.data(),
            instances_.size() * sizeof(LineInstance),
            0);
    }

    void ScreenSpaceLineRenderer::render(
        const glm::mat4& viewProjection,
        const ViewportRect& viewport) const
    {
        render(viewProjection, viewport, config_.depthFunc);
    }

    void ScreenSpaceLineRenderer::render(
        const glm::mat4& viewProjection,
        const ViewportRect& viewport,
        DepthFunc depthFunc) const
    {
        render(
            viewProjection,
            viewport,
            depthFunc,
            config_.depthTest);
    }

    void ScreenSpaceLineRenderer::render(
        const glm::mat4& viewProjection,
        const ViewportRect& viewport,
        DepthFunc depthFunc,
        const bool depthTest) const
    {
        if (!is_valid() || lineCount_ == 0 || viewport.width <= 0 || viewport.height <= 0) {
            return;
        }

        RenderState::set_depth_test(depthTest);
        RenderState::set_depth_write(config_.depthWrite);
        RenderState::set_depth_func(depthFunc);
        RenderState::set_blend(config_.blend);
        RenderState::set_blend_func(
            BlendFactor::SourceAlpha,
            BlendFactor::OneMinusSourceAlpha);
        RenderState::set_cull_face(false);
        RenderState::set_polygon_mode(RenderPolygonMode::Fill);

        shader_->bind();
        shader_->set_mat4("u_ViewProjection", &viewProjection[0][0]);
        shader_->set_vec2(
            "u_ViewportSize",
            static_cast<float>(viewport.width),
            static_cast<float>(viewport.height));

        vertexArray_.bind();
        glDrawArraysInstanced(
            GL_TRIANGLE_STRIP,
            0,
            4,
            static_cast<GLsizei>(lineCount_));
        vertexArray_.unbind();

        shader_->unbind();
        RenderState::reset_default();
    }

    bool ScreenSpaceLineRenderer::is_valid() const
    {
        return shader_ != nullptr
            && vertexArray_.is_valid()
            && instanceBuffer_.is_valid();
    }

    std::size_t ScreenSpaceLineRenderer::line_count() const noexcept
    {
        return lineCount_;
    }

    std::size_t ScreenSpaceLineRenderer::capacity() const noexcept
    {
        return capacity_;
    }

    GraphicsResult<void> ScreenSpaceLineRenderer::create_instance_layout()
    {
        vertexArray_.bind();
        instanceBuffer_.bind();

        const i32 stride = static_cast<i32>(sizeof(LineInstance));

        auto startResult = vertexArray_.set_attribute(
            VertexAttribute{
                0,
                3,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(LineInstance, start)
            });
        if (!startResult) {
            return startResult.error();
        }

        auto widthResult = vertexArray_.set_attribute(
            VertexAttribute{
                1,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(LineInstance, widthPixels)
            });
        if (!widthResult) {
            return widthResult.error();
        }

        auto endResult = vertexArray_.set_attribute(
            VertexAttribute{
                2,
                3,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(LineInstance, end)
            });
        if (!endResult) {
            return endResult.error();
        }

        auto colorResult = vertexArray_.set_attribute(
            VertexAttribute{
                3,
                4,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(LineInstance, color)
            });
        if (!colorResult) {
            return colorResult.error();
        }

        vertexArray_.set_attribute_divisor(0, 1);
        vertexArray_.set_attribute_divisor(1, 1);
        vertexArray_.set_attribute_divisor(2, 1);
        vertexArray_.set_attribute_divisor(3, 1);

        vertexArray_.unbind();
        instanceBuffer_.unbind();

        return {};
    }

    GraphicsResult<void> ScreenSpaceLineRenderer::ensure_capacity(std::size_t count)
    {
        if (count <= capacity_) {
            return {};
        }

        const std::size_t newCapacity = std::max<std::size_t>(count, capacity_ * 2u + 64u);
        auto result = instanceBuffer_.set_data(
            nullptr,
            newCapacity * sizeof(LineInstance));

        if (!result) {
            return result.error();
        }

        capacity_ = newCapacity;
        return {};
    }

    ScreenSpaceLineRenderer::LineInstance ScreenSpaceLineRenderer::make_instance(
        const ScreenSpaceLine& line) const noexcept
    {
        LineInstance instance{};
        instance.start[0] = line.start.x;
        instance.start[1] = line.start.y;
        instance.start[2] = line.start.z;
        instance.end[0] = line.end.x;
        instance.end[1] = line.end.y;
        instance.end[2] = line.end.z;
        instance.widthPixels = std::min(line.widthPixels, config_.maxWidthPixels);
        instance.color[0] = line.color.r;
        instance.color[1] = line.color.g;
        instance.color[2] = line.color.b;
        instance.color[3] = line.color.a;
        return instance;
    }
}
