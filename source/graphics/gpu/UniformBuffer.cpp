#include "graphics/gpu/UniformBuffer.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <utility>

namespace locus::graphics
{
    namespace
    {
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

            return GL_DYNAMIC_DRAW;
        }

        u32 max_uniform_buffer_bindings()
        {
            GLint value = 0;
            glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &value);
            return value > 0 ? static_cast<u32>(value) : 0;
        }
    }

    UniformBuffer::~UniformBuffer()
    {
        destroy();
    }

    UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
    {
        *this = std::move(other);
    }

    UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        id_ = other.id_;
        size_ = other.size_;
        usage_ = other.usage_;

        other.id_ = 0;
        other.size_ = 0;
        other.usage_ = BufferUsage::Dynamic;

        return *this;
    }

    GraphicsResult<void> UniformBuffer::create(std::size_t size, BufferUsage usage)
    {
        if (id_ != 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create UniformBuffer because it already owns a GPU buffer."
            );
        }

        if (size == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot create UniformBuffer with zero bytes."
            );
        }

        usage_ = usage;

        glCreateBuffers(1, &id_);

        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::BufferCreationFailed,
                "Failed to create OpenGL uniform buffer."
            );
        }

        glNamedBufferData(
            id_,
            static_cast<GLsizeiptr>(size),
            nullptr,
            to_gl_usage(usage_)
        );

        size_ = size;

        return {};
    }

    void UniformBuffer::destroy()
    {
        if (id_ != 0)
        {
            glDeleteBuffers(1, &id_);
            id_ = 0;
        }

        size_ = 0;
        usage_ = BufferUsage::Dynamic;
    }

    void UniformBuffer::bind() const
    {
        glBindBuffer(GL_UNIFORM_BUFFER, id_);
    }

    void UniformBuffer::unbind()
    {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    GraphicsResult<void> UniformBuffer::resize(std::size_t size)
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot resize an invalid UniformBuffer."
            );
        }

        if (size == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot resize UniformBuffer to zero bytes."
            );
        }

        glNamedBufferData(
            id_,
            static_cast<GLsizeiptr>(size),
            nullptr,
            to_gl_usage(usage_)
        );

        size_ = size;

        return {};
    }

    GraphicsResult<void> UniformBuffer::set_data(const void* data, std::size_t size, std::size_t offset)
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot update an invalid UniformBuffer."
            );
        }

        if (data == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot update UniformBuffer from null data."
            );
        }

        if (size == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot update UniformBuffer with zero bytes."
            );
        }

        GraphicsResult<void> rangeResult = validate_range(offset, size);
        if (!rangeResult)
        {
            return rangeResult.error();
        }

        glNamedBufferSubData(
            id_,
            static_cast<GLintptr>(offset),
            static_cast<GLsizeiptr>(size),
            data
        );

        return {};
    }

    GraphicsResult<void> UniformBuffer::bind_base(u32 bindingPoint) const
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot bind an invalid UniformBuffer."
            );
        }

        GraphicsResult<void> bindingResult = validate_binding_point(bindingPoint);
        if (!bindingResult)
        {
            return bindingResult.error();
        }

        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id_);

        return {};
    }

    GraphicsResult<void> UniformBuffer::bind_range(u32 bindingPoint, std::size_t offset, std::size_t size) const
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot bind a range from an invalid UniformBuffer."
            );
        }

        GraphicsResult<void> bindingResult = validate_binding_point(bindingPoint);
        if (!bindingResult)
        {
            return bindingResult.error();
        }

        GraphicsResult<void> rangeResult = validate_range(offset, size);
        if (!rangeResult)
        {
            return rangeResult.error();
        }

        const u32 alignment = uniform_buffer_offset_alignment();
        if (alignment != 0 && offset % alignment != 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "UniformBuffer range offset is not aligned to GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT."
            );
        }

        glBindBufferRange(
            GL_UNIFORM_BUFFER,
            bindingPoint,
            id_,
            static_cast<GLintptr>(offset),
            static_cast<GLsizeiptr>(size)
        );

        return {};
    }

    bool UniformBuffer::is_valid() const
    {
        return id_ != 0;
    }

    u32 UniformBuffer::id() const
    {
        return id_;
    }

    std::size_t UniformBuffer::size() const
    {
        return size_;
    }

    BufferUsage UniformBuffer::usage() const
    {
        return usage_;
    }

    u32 UniformBuffer::uniform_buffer_offset_alignment()
    {
        GLint value = 0;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &value);
        return value > 0 ? static_cast<u32>(value) : 0;
    }

    GraphicsResult<void> UniformBuffer::validate_binding_point(u32 bindingPoint) const
    {
        const u32 maxBindings = max_uniform_buffer_bindings();

        if (maxBindings != 0 && bindingPoint >= maxBindings)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "UniformBuffer binding point is outside GL_MAX_UNIFORM_BUFFER_BINDINGS."
            );
        }

        return {};
    }

    GraphicsResult<void> UniformBuffer::validate_range(std::size_t offset, std::size_t size) const
    {
        if (size == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "UniformBuffer range size cannot be zero."
            );
        }

        if (offset > size_)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "UniformBuffer range offset is outside the allocated buffer size."
            );
        }

        if (size > size_ - offset)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "UniformBuffer range is outside the allocated buffer size."
            );
        }

        return {};
    }
}