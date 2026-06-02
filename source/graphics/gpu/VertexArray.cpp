/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/gpu/VertexArray.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <utility>

namespace locus::graphics
{

    VertexArray::~VertexArray()
    {
        destroy();
    }

    VertexArray::VertexArray(VertexArray&& other) noexcept
    {
        *this = std::move(other);
    }

    VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        /*
         * Move raw OpenGL ownership and reset the source object so destruction
         * remains single-owner.
         */
        id_ = other.id_;
        other.id_ = 0;

        return *this;
    }

    GraphicsResult<void> VertexArray::create()
    {
        if (id_ != 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create VertexArray because it already owns a GPU vertex array.");
        }

        glCreateVertexArrays(1, &id_);

        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::VertexArrayCreationFailed,
                "Failed to create OpenGL vertex array.");
        }

        return {};
    }

    void VertexArray::destroy()
    {
        if (id_ != 0)
        {
            glDeleteVertexArrays(1, &id_);
            id_ = 0;
        }
    }

    void VertexArray::bind() const
    {
        glBindVertexArray(id_);
    }

    void VertexArray::unbind() const
    {
        glBindVertexArray(0);
    }

    GraphicsResult<void> VertexArray::set_attribute(const VertexAttribute& attribute)
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot set attribute on an invalid VertexArray.");
        }

        if (attribute.componentCount <= 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Vertex attribute component count must be greater than zero.");
        }

        bind();

        glEnableVertexAttribArray(attribute.index);

        /*
         * Integer attributes must use glVertexAttribIPointer; using the float
         * path would reinterpret integer data during vertex fetch.
         */
        if (attribute.type == VertexAttributeType::Float)
        {
            glVertexAttribPointer(
                attribute.index,
                attribute.componentCount,
                gl_type(attribute.type),
                attribute.normalized ? GL_TRUE : GL_FALSE,
                attribute.stride,
                reinterpret_cast<const void*>(attribute.offset));
        }
        else
        {
            glVertexAttribIPointer(
                attribute.index,
                attribute.componentCount,
                gl_type(attribute.type),
                attribute.stride,
                reinterpret_cast<const void*>(attribute.offset));
        }

        return {};
    }

    bool VertexArray::is_valid() const
    {
        return id_ != 0;
    }

    u32 VertexArray::id() const
    {
        return id_;
    }

    u32 VertexArray::gl_type(VertexAttributeType type) const
    {
        switch (type)
        {
        case VertexAttributeType::Float:
            return GL_FLOAT;

        case VertexAttributeType::Int:
            return GL_INT;

        case VertexAttributeType::UInt:
            return GL_UNSIGNED_INT;
        }

        return GL_FLOAT;
    }

}
