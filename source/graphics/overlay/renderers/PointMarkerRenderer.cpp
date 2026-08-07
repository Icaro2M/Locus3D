/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/PointMarkerRenderer.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/RenderState.h"

#include <glad/glad.h>

#include <algorithm>
#include <cstdio>
#include <utility>

namespace locus::graphics
{
    namespace
    {
        void append_marker_log(const char* message)
        {
            FILE* file = nullptr;
            if (fopen_s(&file, "locus3d_startup.log", "a") != 0 || file == nullptr) {
                return;
            }

            fputs(message, file);
            fputc('\n', file);
            fclose(file);
        }
    }

    PointMarkerRenderer::~PointMarkerRenderer()
    {
        destroy();
    }

    PointMarkerRenderer::PointMarkerRenderer(PointMarkerRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    PointMarkerRenderer& PointMarkerRenderer::operator=(
        PointMarkerRenderer&& other) noexcept
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
        markerCount_ = other.markerCount_;

        other.shader_ = nullptr;
        other.capacity_ = 0;
        other.markerCount_ = 0;
        other.instances_.clear();

        return *this;
    }

    GraphicsResult<void> PointMarkerRenderer::create(
        const ShaderManager& shaderManager,
        const PointMarkerRendererConfig& config)
    {
        if (shader_ != nullptr ||
            vertexArray_.is_valid() ||
            instanceBuffer_.is_valid()) {
            destroy();
        }
        append_marker_log("PointMarkerRenderer: after optional destroy");

        const Shader* shader = shaderManager.find(config.shaderName);
        append_marker_log("PointMarkerRenderer: after shader find");
        if (shader == nullptr) {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "PointMarkerRenderer requires shader: " + config.shaderName + ".");
        }

        auto vertexArrayResult = vertexArray_.create();
        append_marker_log("PointMarkerRenderer: after vertex array create");
        if (!vertexArrayResult) {
            return vertexArrayResult.error();
        }

        auto bufferResult = instanceBuffer_.create(
            BufferType::Vertex,
            BufferUsage::Dynamic);
        append_marker_log("PointMarkerRenderer: after instance buffer create");
        if (!bufferResult) {
            return bufferResult.error();
        }

        shader_ = shader;
        config_ = config;

        auto capacityResult = ensure_capacity(64u);
        append_marker_log("PointMarkerRenderer: after ensure capacity");
        if (!capacityResult) {
            destroy();
            return capacityResult.error();
        }

        auto layoutResult = create_instance_layout();
        append_marker_log("PointMarkerRenderer: after instance layout");
        return layoutResult;
    }

    void PointMarkerRenderer::destroy()
    {
        instanceBuffer_.destroy();
        vertexArray_.destroy();
        instances_.clear();
        shader_ = nullptr;
        capacity_ = 0;
        markerCount_ = 0;
        config_ = {};
    }

    GraphicsResult<void> PointMarkerRenderer::set_markers(
        const PointMarkerBatch& batch)
    {
        if (shader_ == nullptr || !vertexArray_.is_valid() || !instanceBuffer_.is_valid()) {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload PointMarkerRenderer because it was not created.");
        }

        instances_.clear();
        instances_.reserve(batch.markers.size());

        for (const PointMarker& marker : batch.markers) {
            if (!is_drawable(marker)) {
                continue;
            }

            instances_.push_back(make_instance(marker));
        }

        markerCount_ = instances_.size();

        if (markerCount_ == 0) {
            return {};
        }

        auto capacityResult = ensure_capacity(markerCount_);
        if (!capacityResult) {
            return capacityResult.error();
        }

        return instanceBuffer_.set_sub_data(
            instances_.data(),
            instances_.size() * sizeof(MarkerInstance),
            0);
    }

    void PointMarkerRenderer::render(
        const glm::mat4& viewProjection,
        const ViewportRect& viewport) const
    {
        render(
            viewProjection,
            viewport,
            config_.depthFunc,
            config_.depthTest);
    }

    void PointMarkerRenderer::render(
        const glm::mat4& viewProjection,
        const ViewportRect& viewport,
        const DepthFunc depthFunc,
        const bool depthTest) const
    {
        if (!is_valid() || markerCount_ == 0 || viewport.width <= 0 || viewport.height <= 0) {
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
            static_cast<GLsizei>(markerCount_));
        vertexArray_.unbind();

        shader_->unbind();
        RenderState::reset_default();
    }

    bool PointMarkerRenderer::is_valid() const
    {
        return shader_ != nullptr
            && vertexArray_.is_valid()
            && instanceBuffer_.is_valid();
    }

    std::size_t PointMarkerRenderer::marker_count() const noexcept
    {
        return markerCount_;
    }

    std::size_t PointMarkerRenderer::capacity() const noexcept
    {
        return capacity_;
    }

    GraphicsResult<void> PointMarkerRenderer::create_instance_layout()
    {
        vertexArray_.bind();
        instanceBuffer_.bind();

        const i32 stride = static_cast<i32>(sizeof(MarkerInstance));

        auto positionResult = vertexArray_.set_attribute(
            VertexAttribute{
                0,
                3,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(MarkerInstance, position)
            });
        if (!positionResult) {
            return positionResult.error();
        }

        auto radiusResult = vertexArray_.set_attribute(
            VertexAttribute{
                1,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(MarkerInstance, radiusPixels)
            });
        if (!radiusResult) {
            return radiusResult.error();
        }

        auto fillResult = vertexArray_.set_attribute(
            VertexAttribute{
                2,
                4,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(MarkerInstance, fillColor)
            });
        if (!fillResult) {
            return fillResult.error();
        }

        auto borderResult = vertexArray_.set_attribute(
            VertexAttribute{
                3,
                4,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(MarkerInstance, borderColor)
            });
        if (!borderResult) {
            return borderResult.error();
        }

        auto borderWidthResult = vertexArray_.set_attribute(
            VertexAttribute{
                4,
                1,
                VertexAttributeType::Float,
                false,
                stride,
                offsetof(MarkerInstance, borderWidthPixels)
            });
        if (!borderWidthResult) {
            return borderWidthResult.error();
        }

        vertexArray_.set_attribute_divisor(0, 1);
        vertexArray_.set_attribute_divisor(1, 1);
        vertexArray_.set_attribute_divisor(2, 1);
        vertexArray_.set_attribute_divisor(3, 1);
        vertexArray_.set_attribute_divisor(4, 1);

        vertexArray_.unbind();
        instanceBuffer_.unbind();

        return {};
    }

    GraphicsResult<void> PointMarkerRenderer::ensure_capacity(std::size_t count)
    {
        if (count <= capacity_) {
            return {};
        }

        const std::size_t newCapacity = std::max<std::size_t>(count, capacity_ * 2u + 64u);
        auto result = instanceBuffer_.set_data(
            nullptr,
            newCapacity * sizeof(MarkerInstance));

        if (!result) {
            return result.error();
        }

        capacity_ = newCapacity;
        return {};
    }

    PointMarkerRenderer::MarkerInstance PointMarkerRenderer::make_instance(
        const PointMarker& marker) const noexcept
    {
        MarkerInstance instance{};
        instance.position[0] = marker.position.x;
        instance.position[1] = marker.position.y;
        instance.position[2] = marker.position.z;
        instance.radiusPixels = std::min(marker.radiusPixels, config_.maxRadiusPixels);
        instance.fillColor[0] = marker.fillColor.r;
        instance.fillColor[1] = marker.fillColor.g;
        instance.fillColor[2] = marker.fillColor.b;
        instance.fillColor[3] = marker.fillColor.a;
        instance.borderColor[0] = marker.borderColor.r;
        instance.borderColor[1] = marker.borderColor.g;
        instance.borderColor[2] = marker.borderColor.b;
        instance.borderColor[3] = marker.borderColor.a;
        instance.borderWidthPixels = std::min(
            marker.borderWidthPixels,
            instance.radiusPixels);
        return instance;
    }
}
