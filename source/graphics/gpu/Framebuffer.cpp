/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/gpu/Framebuffer.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <utility>

namespace locus::graphics
{
    Framebuffer::~Framebuffer()
    {
        destroy();
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    {
        *this = std::move(other);
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        id_ = other.id_;
        width_ = other.width_;
        height_ = other.height_;

        colorAttachment_ = std::move(other.colorAttachment_);
        depthStencilAttachment_ = std::move(other.depthStencilAttachment_);

        hasColorAttachment_ = other.hasColorAttachment_;
        hasDepthStencilAttachment_ = other.hasDepthStencilAttachment_;

        other.id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.hasColorAttachment_ = false;
        other.hasDepthStencilAttachment_ = false;

        return *this;
    }

    GraphicsResult<void> Framebuffer::create(const FramebufferCreateInfo& info)
    {
        if (id_ != 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create Framebuffer because it already owns a GPU framebuffer."
            );
        }

        if (info.width <= 0 || info.height <= 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot create Framebuffer with invalid dimensions."
            );
        }

        if (!info.createColorAttachment && !info.createDepthStencilAttachment)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot create Framebuffer without attachments."
            );
        }

        glCreateFramebuffers(1, &id_);

        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::FramebufferCreationFailed,
                "Failed to create OpenGL framebuffer."
            );
        }

        width_ = info.width;
        height_ = info.height;

        // Attachments are owned textures so framebuffer lifetime stays self-contained.
        if (info.createColorAttachment)
        {
            TextureCreateInfo textureInfo;
            textureInfo.width = info.width;
            textureInfo.height = info.height;
            textureInfo.format = info.colorFormat;
            textureInfo.minFilter = TextureFilter::Nearest;
            textureInfo.magFilter = TextureFilter::Nearest;
            textureInfo.wrapS = TextureWrap::ClampToEdge;
            textureInfo.wrapT = TextureWrap::ClampToEdge;

            auto textureResult = colorAttachment_.create_2d(textureInfo);

            if (!textureResult)
            {
                destroy();
                return textureResult;
            }

            glNamedFramebufferTexture(
                id_,
                GL_COLOR_ATTACHMENT0,
                colorAttachment_.id(),
                0
            );

            const GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
            glNamedFramebufferDrawBuffers(id_, 1, &drawBuffer);
            glNamedFramebufferReadBuffer(id_, GL_COLOR_ATTACHMENT0);

            hasColorAttachment_ = true;
        }
        else
        {
            // Depth-only framebuffers must opt out of color reads and writes.
            glNamedFramebufferDrawBuffer(id_, GL_NONE);
            glNamedFramebufferReadBuffer(id_, GL_NONE);
        }

        if (info.createDepthStencilAttachment)
        {
            TextureCreateInfo textureInfo;
            textureInfo.width = info.width;
            textureInfo.height = info.height;
            textureInfo.format = info.depthStencilFormat;
            textureInfo.minFilter = TextureFilter::Nearest;
            textureInfo.magFilter = TextureFilter::Nearest;
            textureInfo.wrapS = TextureWrap::ClampToEdge;
            textureInfo.wrapT = TextureWrap::ClampToEdge;

            auto textureResult = depthStencilAttachment_.create_2d(textureInfo);

            if (!textureResult)
            {
                destroy();
                return textureResult;
            }

            glNamedFramebufferTexture(
                id_,
                GL_DEPTH_STENCIL_ATTACHMENT,
                depthStencilAttachment_.id(),
                0
            );

            hasDepthStencilAttachment_ = true;
        }

        auto validationResult = validate();

        if (!validationResult)
        {
            destroy();
            return validationResult;
        }

        return {};
    }

    void Framebuffer::destroy()
    {
        colorAttachment_.destroy();
        depthStencilAttachment_.destroy();

        if (id_ != 0)
        {
            glDeleteFramebuffers(1, &id_);
            id_ = 0;
        }

        width_ = 0;
        height_ = 0;
        hasColorAttachment_ = false;
        hasDepthStencilAttachment_ = false;
    }

    void Framebuffer::bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, id_);
    }

    void Framebuffer::bind_default()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::clear_color(float r, float g, float b, float a) const
    {
        const float color[4] = { r, g, b, a };
        glClearNamedFramebufferfv(id_, GL_COLOR, 0, color);
    }

    void Framebuffer::clear_depth_stencil(float depth, i32 stencil) const
    {
        glClearNamedFramebufferfi(id_, GL_DEPTH_STENCIL, 0, depth, stencil);
    }

    bool Framebuffer::is_valid() const
    {
        return id_ != 0;
    }

    u32 Framebuffer::id() const
    {
        return id_;
    }

    i32 Framebuffer::width() const
    {
        return width_;
    }

    i32 Framebuffer::height() const
    {
        return height_;
    }

    const Texture* Framebuffer::color_attachment() const
    {
        if (!hasColorAttachment_)
        {
            return nullptr;
        }

        return &colorAttachment_;
    }

    const Texture* Framebuffer::depth_stencil_attachment() const
    {
        if (!hasDepthStencilAttachment_)
        {
            return nullptr;
        }

        return &depthStencilAttachment_;
    }

    GraphicsResult<void> Framebuffer::validate() const
    {
        const GLenum status = glCheckNamedFramebufferStatus(id_, GL_FRAMEBUFFER);

        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            return {};
        }

        return GraphicsError::make(
            GraphicsErrorCode::FramebufferCreationFailed,
            "OpenGL framebuffer is incomplete."
        );
    }
}
