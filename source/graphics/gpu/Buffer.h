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
     * @brief RAII wrapper for an OpenGL buffer object.
     */
    class Buffer
    {
    public:
        /**
         * @brief Creates an empty buffer wrapper.
         */
        Buffer() = default;

        /**
         * @brief Deletes the owned GPU buffer, if any.
         */
        ~Buffer();

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        /**
         * @brief Creates a GPU buffer object.
         *
         * @param type Buffer binding category.
         * @param usage Expected update frequency.
         * @return Success or creation error.
         */
        [[nodiscard]] GraphicsResult<void> create(BufferType type, BufferUsage usage);

        /**
         * @brief Deletes the owned GPU buffer.
         */
        void destroy();

        /**
         * @brief Binds this buffer to its target.
         */
        void bind() const;

        /**
         * @brief Unbinds this buffer's target.
         */
        void unbind() const;

        /**
         * @brief Allocates and uploads full buffer contents.
         *
         * @param data Source data pointer.
         * @param size Number of bytes to upload.
         * @return Success or upload error.
         */
        [[nodiscard]] GraphicsResult<void> set_data(const void* data, std::size_t size);

        /**
         * @brief Updates a subrange of the existing buffer storage.
         *
         * @param data Source data pointer.
         * @param size Number of bytes to upload.
         * @param offset Destination byte offset.
         * @return Success or update error.
         */
        [[nodiscard]] GraphicsResult<void> set_sub_data(const void* data, std::size_t size, std::size_t offset);

        /**
         * @brief Checks whether this wrapper owns a GPU buffer.
         *
         * @return True when the OpenGL buffer ID is non-zero.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the OpenGL buffer ID.
         *
         * @return OpenGL object ID.
         */
        [[nodiscard]] u32 id() const;

        /**
         * @brief Returns the buffer binding category.
         *
         * @return Buffer type.
         */
        [[nodiscard]] BufferType type() const;

        /**
         * @brief Returns the allocated buffer size.
         *
         * @return Buffer size in bytes.
         */
        [[nodiscard]] std::size_t size() const;

    private:
        [[nodiscard]] u32 gl_target() const;
        [[nodiscard]] u32 gl_usage() const;

    private:
        u32 id_ = 0;
        BufferType type_ = BufferType::Vertex;
        BufferUsage usage_ = BufferUsage::Static;
        std::size_t size_ = 0;
    };

}
