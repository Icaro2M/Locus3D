/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/picking/PickingBuffer.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <algorithm>
#include <utility>
#include <vector>

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

    std::vector<PickingId> PickingBuffer::read_region(
        i32 x,
        i32 y,
        i32 width,
        i32 height) const
    {
        std::vector<PickingId> result;

        if (!framebuffer_.is_valid() || width <= 0 || height <= 0)
        {
            return result;
        }

        const i32 minX = std::clamp(x, 0, framebuffer_.width());
        const i32 minY = std::clamp(y, 0, framebuffer_.height());
        const i32 maxX = std::clamp(x + width, 0, framebuffer_.width());
        const i32 maxY = std::clamp(y + height, 0, framebuffer_.height());

        const i32 clippedWidth = maxX - minX;
        const i32 clippedHeight = maxY - minY;

        if (clippedWidth <= 0 || clippedHeight <= 0)
        {
            return result;
        }

        std::vector<unsigned char> pixels(
            static_cast<std::size_t>(clippedWidth) *
            static_cast<std::size_t>(clippedHeight) *
            4u);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_.id());
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glReadPixels(
            minX,
            minY,
            clippedWidth,
            clippedHeight,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels.data()
        );

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        for (std::size_t index = 0u; index + 2u < pixels.size();
            index += 4u)
        {
            const PickingId id = decode_picking_id(
                pixels[index],
                pixels[index + 1u],
                pixels[index + 2u]);

            if (!id.is_valid())
            {
                continue;
            }

            const auto exists = std::find_if(
                result.begin(),
                result.end(),
                [id](const PickingId candidate)
                {
                    return candidate == id;
                });

            if (exists == result.end())
            {
                result.push_back(id);
            }
        }

        return result;
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
