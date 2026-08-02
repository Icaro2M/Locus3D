/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/passes/ObjectOutlinePass.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/RenderState.h"
#include "graphics/gpu/Shader.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/gpu/Texture.h"

#include <glad/glad.h>

#include <algorithm>
#include <utility>

namespace locus::graphics
{
    ObjectOutlinePass::~ObjectOutlinePass()
    {
        destroy();
    }

    ObjectOutlinePass::ObjectOutlinePass(ObjectOutlinePass&& other) noexcept
    {
        *this = std::move(other);
    }

    ObjectOutlinePass& ObjectOutlinePass::operator=(ObjectOutlinePass&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        config_ = other.config_;
        shader_ = other.shader_;
        fullscreenVertexArray_ = std::move(other.fullscreenVertexArray_);

        other.shader_ = nullptr;
        other.config_ = {};

        return *this;
    }

    GraphicsResult<void> ObjectOutlinePass::create(const ShaderManager& shaderManager)
    {
        destroy();

        shader_ = shaderManager.find("viewport/object_outline");
        if (shader_ == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "ObjectOutlinePass requires shader: viewport/object_outline."
            );
        }

        return fullscreenVertexArray_.create();
    }

    void ObjectOutlinePass::destroy()
    {
        fullscreenVertexArray_.destroy();
        shader_ = nullptr;
        config_ = {};
    }

    void ObjectOutlinePass::set_config(const ObjectOutlinePassConfig& config) noexcept
    {
        config_ = config;
    }

    void ObjectOutlinePass::render(
        const Texture& maskTexture,
        const ViewportRect& viewport) const
    {
        if (!is_valid()
            || !maskTexture.is_valid()
            || viewport.width <= 0
            || viewport.height <= 0)
        {
            return;
        }

        RenderState::set_viewport(
            viewport.x,
            viewport.y,
            viewport.width,
            viewport.height);
        RenderState::set_depth_test(false);
        RenderState::set_depth_write(false);
        RenderState::set_blend(true);
        RenderState::set_blend_func(
            BlendFactor::SourceAlpha,
            BlendFactor::OneMinusSourceAlpha);
        RenderState::set_cull_face(false);
        RenderState::set_polygon_mode(RenderPolygonMode::Fill);
        RenderState::set_color_write(true, true, true, true);

        maskTexture.bind(0);

        shader_->bind();
        shader_->set_int("u_MaskTexture", 0);
        shader_->set_vec2(
            "u_TextureSize",
            static_cast<float>(maskTexture.width()),
            static_cast<float>(maskTexture.height()));
        shader_->set_float(
            "u_HoveredWidthPixels",
            std::max(config_.hoveredWidthPixels, 0.0f));
        shader_->set_float(
            "u_SelectedWidthPixels",
            std::max(config_.selectedWidthPixels, 0.0f));
        shader_->set_vec4(
            "u_HoveredColor",
            config_.hoveredColor.r,
            config_.hoveredColor.g,
            config_.hoveredColor.b,
            config_.hoveredColor.a);
        shader_->set_vec4(
            "u_SelectedColor",
            config_.selectedColor.r,
            config_.selectedColor.g,
            config_.selectedColor.b,
            config_.selectedColor.a);

        fullscreenVertexArray_.bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        fullscreenVertexArray_.unbind();

        shader_->unbind();
        maskTexture.unbind(0);

        RenderState::reset_default();
    }

    bool ObjectOutlinePass::is_valid() const
    {
        return shader_ != nullptr
            && shader_->is_valid()
            && fullscreenVertexArray_.is_valid();
    }

    const ObjectOutlinePassConfig& ObjectOutlinePass::config() const noexcept
    {
        return config_;
    }
}
