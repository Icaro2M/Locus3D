/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/PivotRenderer.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/RenderState.h"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace locus::graphics
{
    namespace
    {
        [[nodiscard]] bool is_drawable(const PivotDrawData& pivot) noexcept
        {
            return pivot.visible
                && std::isfinite(pivot.position.x)
                && std::isfinite(pivot.position.y)
                && std::isfinite(pivot.position.z);
        }
    }

    PivotRenderer::~PivotRenderer()
    {
        destroy();
    }

    PivotRenderer::PivotRenderer(PivotRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    PivotRenderer& PivotRenderer::operator=(PivotRenderer&& other) noexcept
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
        pivotCount_ = other.pivotCount_;

        other.shader_ = nullptr;
        other.capacity_ = 0;
        other.pivotCount_ = 0;
        other.instances_.clear();

        return *this;
    }

    GraphicsResult<void> PivotRenderer::create(
        const ShaderManager& shaderManager,
        const PivotRendererConfig& config)
    {
        destroy();

        if (config.centerRadiusPixels <= 0.0f ||
            config.gapPixels < 0.0f ||
            config.armLengthPixels <= 0.0f ||
            config.armThicknessPixels <= 0.0f) {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "PivotRenderer received invalid marker dimensions.");
        }

        const Shader* shader = shaderManager.find(config.shaderName);
        if (shader == nullptr) {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "PivotRenderer requires shader: " + config.shaderName + ".");
        }

        auto vertexArrayResult = vertexArray_.create();
        if (!vertexArrayResult) {
            return vertexArrayResult.error();
        }

        auto bufferResult = instanceBuffer_.create(
            BufferType::Vertex,
            BufferUsage::Dynamic);
        if (!bufferResult) {
            destroy();
            return bufferResult.error();
        }

        shader_ = shader;
        config_ = config;

        auto capacityResult = ensure_capacity(4u);
        if (!capacityResult) {
            destroy();
            return capacityResult.error();
        }

        auto layoutResult = create_instance_layout();
        if (!layoutResult) {
            destroy();
            return layoutResult.error();
        }

        return {};
    }

    void PivotRenderer::destroy()
    {
        instanceBuffer_.destroy();
        vertexArray_.destroy();
        instances_.clear();
        shader_ = nullptr;
        capacity_ = 0;
        pivotCount_ = 0;
        config_ = {};
    }

    GraphicsResult<void> PivotRenderer::set_pivots(
        const std::vector<PivotDrawData>& pivots)
    {
        if (!is_valid()) {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload PivotRenderer because it was not created.");
        }

        instances_.clear();
        instances_.reserve(pivots.size());

        for (const PivotDrawData& pivot : pivots) {
            if (!is_drawable(pivot)) {
                continue;
            }

            instances_.push_back(make_instance(pivot));
        }

        pivotCount_ = instances_.size();

        if (pivotCount_ == 0) {
            return {};
        }

        auto capacityResult = ensure_capacity(pivotCount_);
        if (!capacityResult) {
            return capacityResult.error();
        }

        return instanceBuffer_.set_sub_data(
            instances_.data(),
            instances_.size() * sizeof(PivotInstance),
            0);
    }

    void PivotRenderer::render(
        const glm::mat4& viewProjection,
        const ViewportRect& viewport) const
    {
        if (!is_valid() ||
            pivotCount_ == 0 ||
            viewport.width <= 0 ||
            viewport.height <= 0) {
            return;
        }

        RenderState::set_depth_test(config_.depthTest);
        RenderState::set_depth_write(config_.depthWrite);
        RenderState::set_depth_func(config_.depthFunc);
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
            static_cast<GLsizei>(pivotCount_));
        vertexArray_.unbind();

        shader_->unbind();
        RenderState::reset_default();
    }

    bool PivotRenderer::is_valid() const
    {
        return shader_ != nullptr
            && vertexArray_.is_valid()
            && instanceBuffer_.is_valid();
    }

    std::size_t PivotRenderer::pivot_count() const noexcept
    {
        return pivotCount_;
    }

    std::size_t PivotRenderer::capacity() const noexcept
    {
        return capacity_;
    }

    GraphicsResult<void> PivotRenderer::create_instance_layout()
    {
        vertexArray_.bind();
        instanceBuffer_.bind();

        const i32 stride = static_cast<i32>(sizeof(PivotInstance));

        auto positionResult = vertexArray_.set_attribute(
            VertexAttribute{
                0,
                3,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(PivotInstance, position)
            });
        if (!positionResult) {
            return positionResult.error();
        }

        auto halfExtentResult = vertexArray_.set_attribute(
            VertexAttribute{
                1,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(PivotInstance, halfExtentPixels)
            });
        if (!halfExtentResult) {
            return halfExtentResult.error();
        }

        auto colorResult = vertexArray_.set_attribute(
            VertexAttribute{
                2,
                4,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(PivotInstance, color)
            });
        if (!colorResult) {
            return colorResult.error();
        }

        auto centerResult = vertexArray_.set_attribute(
            VertexAttribute{
                3,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(PivotInstance, centerRadiusPixels)
            });
        if (!centerResult) {
            return centerResult.error();
        }

        auto gapResult = vertexArray_.set_attribute(
            VertexAttribute{
                4,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(PivotInstance, gapPixels)
            });
        if (!gapResult) {
            return gapResult.error();
        }

        auto armLengthResult = vertexArray_.set_attribute(
            VertexAttribute{
                5,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(PivotInstance, armLengthPixels)
            });
        if (!armLengthResult) {
            return armLengthResult.error();
        }

        auto armThicknessResult = vertexArray_.set_attribute(
            VertexAttribute{
                6,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(PivotInstance, armThicknessPixels)
            });
        if (!armThicknessResult) {
            return armThicknessResult.error();
        }

        for (GLuint index = 0; index <= 6; ++index) {
            vertexArray_.set_attribute_divisor(index, 1);
        }

        vertexArray_.unbind();
        instanceBuffer_.unbind();

        return {};
    }

    GraphicsResult<void> PivotRenderer::ensure_capacity(std::size_t count)
    {
        if (count <= capacity_) {
            return {};
        }

        const std::size_t newCapacity =
            std::max<std::size_t>(count, capacity_ * 2u + 4u);
        auto result = instanceBuffer_.set_data(
            nullptr,
            newCapacity * sizeof(PivotInstance));

        if (!result) {
            return result.error();
        }

        capacity_ = newCapacity;
        return {};
    }

    PivotRenderer::PivotInstance PivotRenderer::make_instance(
        const PivotDrawData& pivot) const noexcept
    {
        const ColorRGBA color = color_for(pivot.state);

        PivotInstance instance{};
        instance.position[0] = pivot.position.x;
        instance.position[1] = pivot.position.y;
        instance.position[2] = pivot.position.z;
        instance.halfExtentPixels = half_extent_pixels();
        instance.color[0] = color.r;
        instance.color[1] = color.g;
        instance.color[2] = color.b;
        instance.color[3] = color.a;
        instance.centerRadiusPixels = config_.centerRadiusPixels;
        instance.gapPixels = config_.gapPixels;
        instance.armLengthPixels = config_.armLengthPixels;
        instance.armThicknessPixels = config_.armThicknessPixels;
        return instance;
    }

    ColorRGBA PivotRenderer::color_for(PivotVisualState state) const noexcept
    {
        switch (state) {
        case PivotVisualState::Hovered:
            return config_.hoveredColor;
        case PivotVisualState::Active:
            return config_.activeColor;
        case PivotVisualState::Normal:
        default:
            return config_.normalColor;
        }
    }

    float PivotRenderer::half_extent_pixels() const noexcept
    {
        return config_.centerRadiusPixels
            + config_.gapPixels
            + config_.armLengthPixels
            + config_.hitPaddingPixels
            + 2.0f;
    }
}
