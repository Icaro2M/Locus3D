/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/gpu/Buffer.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <utility>

namespace locus::graphics
{

    namespace
    {
        /*
         * Keep OpenGL enum translation local to the backend implementation so
         * public headers remain independent from GLAD/OpenGL symbols.
         */
        u32 to_gl_target(BufferType type)
        {
            switch (type)
            {
            case BufferType::Vertex:
                return GL_ARRAY_BUFFER;

            case BufferType::Index:
                return GL_ELEMENT_ARRAY_BUFFER;

            case BufferType::Uniform:
                return GL_UNIFORM_BUFFER;
            }

            return GL_ARRAY_BUFFER;
        }

        u32 to_gl_usage(BufferUsage usage)
        {
            switch (usage)
            {
            case BufferUsage::Static:
                return GL_STATIC_DRAW;

            case BufferUsage::Dynamic:
                return GL_DYNAMIC_DRAW;

            case BufferUsage::Stream:
                return GL_STREAM_DRAW;
            }

            return GL_STATIC_DRAW;
        }

    }

    Buffer::~Buffer()
    {
        destroy();
    }

    Buffer::Buffer(Buffer&& other) noexcept
    {
        *this = std::move(other);
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        /*
         * Transfer raw OpenGL ownership and leave the moved-from wrapper inert
         * so its destructor cannot delete the buffer twice.
         */
        id_ = other.id_;
        type_ = other.type_;
        usage_ = other.usage_;
        size_ = other.size_;

        other.id_ = 0;
        other.size_ = 0;

        return *this;
    }

    GraphicsResult<void> Buffer::create(BufferType type, BufferUsage usage)
    {
        if (id_ != 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create Buffer because it already owns a GPU buffer.");
        }

        type_ = type;
        usage_ = usage;

        glCreateBuffers(1, &id_);

        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::BufferCreationFailed,
                "Failed to create OpenGL buffer.");
        }

        return {};
    }

    void Buffer::destroy()
    {
        if (id_ != 0)
        {
            glDeleteBuffers(1, &id_);
            id_ = 0;
        }

        size_ = 0;
    }

    void Buffer::bind() const
    {
        glBindBuffer(gl_target(), id_);
    }

    void Buffer::unbind() const
    {
        glBindBuffer(gl_target(), 0);
    }

    GraphicsResult<void> Buffer::set_data(const void* data, std::size_t size)
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload data to an invalid Buffer.");
        }

        if (size == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot upload zero bytes to Buffer.");
        }

        bind();
        glBufferData(gl_target(), static_cast<GLsizeiptr>(size), data, gl_usage());

        size_ = size;

        return {};
    }

    GraphicsResult<void> Buffer::set_sub_data(const void* data, std::size_t size, std::size_t offset)
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot update an invalid Buffer.");
        }

        if (size == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot update Buffer with zero bytes.");
        }

        /*
         * OpenGL accepts out-of-range updates as an error, but validating here
         * keeps the failure explicit and easier to diagnose from callers.
         */
        if (offset + size > size_)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Buffer update range is outside the allocated buffer size.");
        }

        bind();
        glBufferSubData(
            gl_target(),
            static_cast<GLintptr>(offset),
            static_cast<GLsizeiptr>(size),
            data);

        return {};
    }

    bool Buffer::is_valid() const
    {
        return id_ != 0;
    }

    u32 Buffer::id() const
    {
        return id_;
    }

    BufferType Buffer::type() const
    {
        return type_;
    }

    std::size_t Buffer::size() const
    {
        return size_;
    }

    u32 Buffer::gl_target() const
    {
        return to_gl_target(type_);
    }

    u32 Buffer::gl_usage() const
    {
        return to_gl_usage(usage_);
    }

}
