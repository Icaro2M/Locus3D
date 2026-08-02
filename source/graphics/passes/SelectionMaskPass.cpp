/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/passes/SelectionMaskPass.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/gpu/RenderState.h"
#include "graphics/gpu/Shader.h"
#include "graphics/gpu/ShaderManager.h"

#include <glad/glad.h>

#include <algorithm>
#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr float ByteScale = 1.0f / 255.0f;

        [[nodiscard]] bool has_drawable_mesh(const RenderObject& object)
        {
            return object.visibility.visible
                && object.mesh != nullptr
                && object.mesh->is_valid();
        }
    }

    SelectionMaskPass::~SelectionMaskPass()
    {
        destroy();
    }

    SelectionMaskPass::SelectionMaskPass(SelectionMaskPass&& other) noexcept
    {
        *this = std::move(other);
    }

    SelectionMaskPass& SelectionMaskPass::operator=(SelectionMaskPass&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        framebuffer_ = std::move(other.framebuffer_);
        shader_ = other.shader_;
        resourceRevision_ = other.resourceRevision_;

        other.shader_ = nullptr;
        other.resourceRevision_ = 0;

        return *this;
    }

    GraphicsResult<void> SelectionMaskPass::create(const ShaderManager& shaderManager)
    {
        shader_ = shaderManager.find("viewport/selection_mask");

        if (shader_ == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "SelectionMaskPass requires shader: viewport/selection_mask."
            );
        }

        return {};
    }

    void SelectionMaskPass::destroy()
    {
        framebuffer_.destroy();
        shader_ = nullptr;
        ++resourceRevision_;
    }

    GraphicsResult<void> SelectionMaskPass::resize(i32 width, i32 height)
    {
        if (width <= 0 || height <= 0)
        {
            if (framebuffer_.is_valid())
            {
                framebuffer_.destroy();
                ++resourceRevision_;
            }

            return {};
        }

        if (framebuffer_.is_valid()
            && framebuffer_.width() == width
            && framebuffer_.height() == height)
        {
            return {};
        }

        framebuffer_.destroy();

        FramebufferCreateInfo info{};
        info.width = width;
        info.height = height;
        info.colorFormat = TextureFormat::RGBA8;
        info.depthStencilFormat = TextureFormat::Depth24Stencil8;
        info.createColorAttachment = true;
        info.createDepthStencilAttachment = true;

        auto result = framebuffer_.create(info);
        if (!result)
        {
            return result.error();
        }

        ++resourceRevision_;
        return {};
    }

    void SelectionMaskPass::render(
        const RenderScene& scene,
        const ObjectHighlightBatch& highlights,
        const glm::mat4& view,
        const glm::mat4& projection) const
    {
        if (!is_valid() || highlights.empty())
        {
            return;
        }

        framebuffer_.bind();
        framebuffer_.clear_color(0.0f, 0.0f, 0.0f, 0.0f);
        framebuffer_.clear_depth_stencil(1.0f, 0);

        RenderState::set_viewport(0, 0, framebuffer_.width(), framebuffer_.height());
        RenderState::set_depth_test(true);
        RenderState::set_depth_func(DepthFunc::Less);
        RenderState::set_depth_write(true);
        RenderState::set_blend(false);
        RenderState::set_cull_face(false);
        RenderState::set_polygon_mode(RenderPolygonMode::Fill);
        RenderState::set_color_write(false, false, false, false);

        for (const RenderObject& object : scene.objects())
        {
            render_depth_object(object, view, projection);
        }

        RenderState::set_depth_func(DepthFunc::LessEqual);
        RenderState::set_depth_write(false);
        RenderState::set_color_write(true, true, true, true);

        for (const ObjectHighlight& highlight : highlights.highlights)
        {
            render_highlight(highlight, view, projection);
        }

        Framebuffer::bind_default();
        RenderState::reset_default();
    }

    bool SelectionMaskPass::is_valid() const
    {
        return shader_ != nullptr
            && shader_->is_valid()
            && framebuffer_.is_valid();
    }

    i32 SelectionMaskPass::width() const noexcept
    {
        return framebuffer_.width();
    }

    i32 SelectionMaskPass::height() const noexcept
    {
        return framebuffer_.height();
    }

    u64 SelectionMaskPass::resource_revision() const noexcept
    {
        return resourceRevision_;
    }

    const Texture* SelectionMaskPass::mask_texture() const
    {
        return framebuffer_.color_attachment();
    }

    void SelectionMaskPass::render_depth_object(
        const RenderObject& object,
        const glm::mat4& view,
        const glm::mat4& projection) const
    {
        if (!has_drawable_mesh(object))
        {
            return;
        }

        const glm::mat4 model = object.transform.matrix();
        const glm::mat4 mvp = projection * view * model;

        shader_->bind();
        shader_->set_mat4("u_MVP", &mvp[0][0]);
        shader_->set_vec4("u_MaskColor", 0.0f, 0.0f, 0.0f, 0.0f);

        object.mesh->draw();

        shader_->unbind();
    }

    void SelectionMaskPass::render_highlight(
        const ObjectHighlight& highlight,
        const glm::mat4& view,
        const glm::mat4& projection) const
    {
        if (!highlight.is_valid() || !has_drawable_mesh(*highlight.object))
        {
            return;
        }

        const glm::mat4 model = highlight.object->transform.matrix();
        const glm::mat4 mvp = projection * view * model;
        const ColorRGBA maskColor = encode_mask(
            highlight.maskId,
            highlight.category);

        shader_->bind();
        shader_->set_mat4("u_MVP", &mvp[0][0]);
        shader_->set_vec4(
            "u_MaskColor",
            maskColor.r,
            maskColor.g,
            maskColor.b,
            maskColor.a);

        highlight.object->mesh->draw();

        shader_->unbind();
    }

    ColorRGBA SelectionMaskPass::encode_mask(
        u32 maskId,
        ObjectHighlightCategory category) noexcept
    {
        const u32 clampedId = std::min(maskId, 0x00ffffffu);
        const u32 categoryByte =
            category == ObjectHighlightCategory::Selected ? 2u : 1u;

        return ColorRGBA{
            static_cast<float>(clampedId & 0xffu) * ByteScale,
            static_cast<float>((clampedId >> 8u) & 0xffu) * ByteScale,
            static_cast<float>((clampedId >> 16u) & 0xffu) * ByteScale,
            static_cast<float>(categoryByte) * ByteScale
        };
    }
}
