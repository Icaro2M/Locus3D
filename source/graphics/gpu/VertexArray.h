/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"

#include <cstddef>

namespace locus::graphics
{
    /**
     * @brief Vertex attribute scalar data type.
     */
    enum class VertexAttributeType
    {
        Float,
        Int,
        UInt
    };

    /**
     * @brief Describes one vertex attribute layout entry.
     */
    struct VertexAttribute
    {
        /**
         * @brief Attribute location index in the shader.
         */
        u32 index = 0;

        /**
         * @brief Number of scalar components in the attribute.
         */
        i32 componentCount = 0;

        /**
         * @brief Scalar data type.
         */
        VertexAttributeType type = VertexAttributeType::Float;

        /**
         * @brief True when fixed-point values should be normalized.
         */
        bool normalized = false;

        /**
         * @brief Byte distance between consecutive vertices.
         */
        i32 stride = 0;

        /**
         * @brief Byte offset from the start of the bound vertex buffer.
         */
        std::size_t offset = 0;
    };

    /**
     * @brief RAII wrapper for an OpenGL vertex array object.
     */
    class VertexArray
    {
    public:
        /**
         * @brief Creates an empty vertex array wrapper.
         */
        VertexArray() = default;

        /**
         * @brief Deletes the owned vertex array object, if any.
         */
        ~VertexArray();

        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;

        VertexArray(VertexArray&& other) noexcept;
        VertexArray& operator=(VertexArray&& other) noexcept;

        /**
         * @brief Creates a GPU vertex array object.
         *
         * @return Success or creation error.
         */
        [[nodiscard]] GraphicsResult<void> create();

        /**
         * @brief Deletes the owned vertex array object.
         */
        void destroy();

        /**
         * @brief Binds this vertex array.
         */
        void bind() const;

        /**
         * @brief Unbinds any current vertex array.
         */
        void unbind() const;

        /**
         * @brief Enables and defines one vertex attribute.
         *
         * @param attribute Attribute layout description.
         * @return Success or attribute configuration error.
         */
        [[nodiscard]] GraphicsResult<void> set_attribute(const VertexAttribute& attribute);

        /**
         * @brief Sets how often one vertex attribute advances during instanced draws.
         *
         * @param index Attribute location index in the shader.
         * @param divisor Instance divisor. Zero advances per vertex.
         */
        void set_attribute_divisor(u32 index, u32 divisor);

        /**
         * @brief Checks whether this wrapper owns a vertex array object.
         *
         * @return True when the OpenGL object ID is non-zero.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the OpenGL vertex array object ID.
         *
         * @return OpenGL object ID.
         */
        [[nodiscard]] u32 id() const;

    private:
        [[nodiscard]] u32 gl_type(VertexAttributeType type) const;

    private:
        u32 id_ = 0;
    };

}
