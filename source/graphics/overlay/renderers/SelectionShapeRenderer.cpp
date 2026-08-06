/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/SelectionShapeRenderer.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/RenderState.h"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace locus::graphics
{
    namespace
    {
        struct QuadVertex
        {
            float position[2]{ 0.0f, 0.0f };
        };

        constexpr QuadVertex UnitQuad[] = {
            { { 0.0f, 0.0f } },
            { { 1.0f, 0.0f } },
            { { 0.0f, 1.0f } },
            { { 1.0f, 1.0f } }
        };

        [[nodiscard]] bool finite_vec2(const glm::vec2& value) noexcept
        {
            return std::isfinite(value.x) && std::isfinite(value.y);
        }
    }

    SelectionShapeRenderer::~SelectionShapeRenderer()
    {
        destroy();
    }

    SelectionShapeRenderer::SelectionShapeRenderer(
        SelectionShapeRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    SelectionShapeRenderer& SelectionShapeRenderer::operator=(
        SelectionShapeRenderer&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        rectangle_ = other.rectangle_;
        shader_ = other.shader_;
        vertexArray_ = std::move(other.vertexArray_);
        vertexBuffer_ = std::move(other.vertexBuffer_);

        other.shader_ = nullptr;
        other.rectangle_ = {};

        return *this;
    }

    GraphicsResult<void> SelectionShapeRenderer::create(
        const ShaderManager& shaderManager,
        const SelectionShapeRendererConfig& config)
    {
        destroy();

        const Shader* shader = shaderManager.find(config.shaderName);
        if (shader == nullptr) {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "SelectionShapeRenderer requires shader: " + config.shaderName + ".");
        }

        auto vertexArrayResult = vertexArray_.create();
        if (!vertexArrayResult) {
            return vertexArrayResult.error();
        }

        auto vertexBufferResult = vertexBuffer_.create(
            BufferType::Vertex,
            BufferUsage::Static);
        if (!vertexBufferResult) {
            return vertexBufferResult.error();
        }

        shader_ = shader;
        config_ = config;

        auto uploadResult = upload_unit_quad();
        if (!uploadResult) {
            destroy();
            return uploadResult.error();
        }

        auto layoutResult = create_vertex_layout();
        if (!layoutResult) {
            destroy();
            return layoutResult.error();
        }

        return {};
    }

    void SelectionShapeRenderer::destroy()
    {
        vertexBuffer_.destroy();
        vertexArray_.destroy();
        shader_ = nullptr;
        rectangle_ = {};
        config_ = {};
    }

    void SelectionShapeRenderer::set_rectangle(
        const SelectionRectangleDrawData& data) noexcept
    {
        rectangle_ = data;
    }

    void SelectionShapeRenderer::render() const
    {
        if (!is_valid() || !has_drawable_rectangle()) {
            return;
        }

        RenderState::set_viewport(
            static_cast<i32>(rectangle_.viewportOrigin.x),
            static_cast<i32>(rectangle_.viewportOrigin.y),
            static_cast<i32>(rectangle_.viewportSize.x),
            static_cast<i32>(rectangle_.viewportSize.y));
        RenderState::set_depth_test(config_.depthTest);
        RenderState::set_depth_write(config_.depthWrite);
        RenderState::set_blend(config_.blend);
        RenderState::set_blend_func(
            BlendFactor::SourceAlpha,
            BlendFactor::OneMinusSourceAlpha);
        RenderState::set_cull_face(false);
        RenderState::set_polygon_mode(RenderPolygonMode::Fill);

        shader_->bind();
        shader_->set_vec2(
            "u_RectMin",
            rectangle_.minimum.x,
            rectangle_.minimum.y);
        shader_->set_vec2(
            "u_RectMax",
            rectangle_.maximum.x,
            rectangle_.maximum.y);
        shader_->set_vec2(
            "u_ViewportSize",
            rectangle_.viewportSize.x,
            rectangle_.viewportSize.y);
        shader_->set_float(
            "u_BorderThicknessPixels",
            config_.borderThicknessPixels);
        shader_->set_vec4(
            "u_FillColor",
            config_.fillColor.r,
            config_.fillColor.g,
            config_.fillColor.b,
            config_.fillColor.a);
        shader_->set_vec4(
            "u_BorderColor",
            config_.borderColor.r,
            config_.borderColor.g,
            config_.borderColor.b,
            config_.borderColor.a);

        vertexArray_.bind();
        glDrawArrays(
            GL_TRIANGLE_STRIP,
            0,
            4);
        vertexArray_.unbind();

        shader_->unbind();
        RenderState::reset_default();
    }

    bool SelectionShapeRenderer::is_valid() const
    {
        return shader_ != nullptr &&
            vertexArray_.is_valid() &&
            vertexBuffer_.is_valid();
    }

    bool SelectionShapeRenderer::has_drawable_rectangle() const noexcept
    {
        return rectangle_.visible &&
            rectangle_.viewportSize.x > 0.0f &&
            rectangle_.viewportSize.y > 0.0f &&
            rectangle_.maximum.x > rectangle_.minimum.x &&
            rectangle_.maximum.y > rectangle_.minimum.y &&
            config_.borderThicknessPixels > 0.0f &&
            finite_vec2(rectangle_.minimum) &&
            finite_vec2(rectangle_.maximum) &&
            finite_vec2(rectangle_.viewportOrigin) &&
            finite_vec2(rectangle_.viewportSize);
    }

    GraphicsResult<void> SelectionShapeRenderer::create_vertex_layout()
    {
        vertexArray_.bind();
        vertexBuffer_.bind();

        auto positionResult = vertexArray_.set_attribute(
            VertexAttribute{
                0,
                2,
                VertexAttributeType::Float,
                false,
                static_cast<i32>(sizeof(QuadVertex)),
                offsetof(QuadVertex, position)
            });
        if (!positionResult) {
            return positionResult.error();
        }

        vertexArray_.unbind();
        vertexBuffer_.unbind();

        return {};
    }

    GraphicsResult<void> SelectionShapeRenderer::upload_unit_quad()
    {
        vertexBuffer_.bind();
        auto result = vertexBuffer_.set_data(
            UnitQuad,
            sizeof(UnitQuad));
        vertexBuffer_.unbind();
        return result;
    }
}
