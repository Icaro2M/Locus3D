/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/picking/PickingBuffer.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <utility>

namespace locus::graphics
{
    PickingBuffer::~PickingBuffer()
    {
        destroy();
    }

    PickingBuffer::PickingBuffer(PickingBuffer&& other) noexcept
    {
        *this = std::move(other);
    }

    PickingBuffer& PickingBuffer::operator=(PickingBuffer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        framebuffer_ = std::move(other.framebuffer_);

        return *this;
    }

    GraphicsResult<void> PickingBuffer::create(i32 width, i32 height)
    {
        if (framebuffer_.is_valid())
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create PickingBuffer because it already owns a framebuffer."
            );
        }

        if (width <= 0 || height <= 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot create PickingBuffer with invalid dimensions."
            );
        }

        FramebufferCreateInfo info;
        info.width = width;
        info.height = height;
        // RGBA8 keeps picking IDs easy to read back as byte channels.
        info.colorFormat = TextureFormat::RGBA8;
        info.depthStencilFormat = TextureFormat::Depth24Stencil8;
        info.createColorAttachment = true;
        info.createDepthStencilAttachment = true;

        return framebuffer_.create(info);
    }

    GraphicsResult<void> PickingBuffer::resize(i32 width, i32 height)
    {
        if (width <= 0 || height <= 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot resize PickingBuffer to invalid dimensions."
            );
        }

        if (
            framebuffer_.is_valid() &&
            framebuffer_.width() == width &&
            framebuffer_.height() == height
            )
        {
            return {};
        }

        destroy();

        return create(width, height);
    }

    void PickingBuffer::destroy()
    {
        framebuffer_.destroy();
    }

    void PickingBuffer::bind() const
    {
        framebuffer_.bind();
    }

    void PickingBuffer::bind_default()
    {
        Framebuffer::bind_default();
    }

    void PickingBuffer::clear() const
    {
        if (!framebuffer_.is_valid())
        {
            return;
        }

        framebuffer_.clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        framebuffer_.clear_depth_stencil(1.0f, 0);
    }

    PickingId PickingBuffer::read_id(i32 x, i32 y) const
    {
        if (!framebuffer_.is_valid())
        {
            return PickingId::invalid();
        }

        if (x < 0 || y < 0 || x >= framebuffer_.width() || y >= framebuffer_.height())
        {
            return PickingId::invalid();
        }

        unsigned char pixel[4] = { 0, 0, 0, 0 };

        // Read only the color attachment; depth is handled by the picking pass.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_.id());
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glReadPixels(
            x,
            y,
            1,
            1,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixel
        );

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        return decode_picking_id(pixel[0], pixel[1], pixel[2]);
    }

    bool PickingBuffer::is_valid() const
    {
        return framebuffer_.is_valid();
    }

    i32 PickingBuffer::width() const
    {
        return framebuffer_.width();
    }

    i32 PickingBuffer::height() const
    {
        return framebuffer_.height();
    }

    const Framebuffer& PickingBuffer::framebuffer() const
    {
        return framebuffer_;
    }
}
