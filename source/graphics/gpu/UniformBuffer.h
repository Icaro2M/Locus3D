/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"

#include <cstddef>

namespace locus::graphics
{
    /**
     * @brief Owns an OpenGL uniform buffer object.
     *
     * UniformBuffer wraps UBO lifetime, range validation, and binding point
     * checks so renderer code can update shader constant data safely.
     *
     * @note Instances are move-only because each object owns one GPU buffer id.
     */
    class UniformBuffer
    {
    public:
        UniformBuffer() = default;
        ~UniformBuffer();

        UniformBuffer(const UniformBuffer&) = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;

        UniformBuffer(UniformBuffer&& other) noexcept;
        UniformBuffer& operator=(UniformBuffer&& other) noexcept;

        /**
         * @brief Creates the GPU buffer storage.
         *
         * @param size Buffer size in bytes.
         * @param usage Expected update frequency for the buffer storage.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(std::size_t size, BufferUsage usage = BufferUsage::Dynamic);

        /**
         * @brief Releases the owned GPU buffer.
         */
        void destroy();

        /**
         * @brief Binds this buffer to the uniform buffer target.
         */
        void bind() const;

        /**
         * @brief Clears the current uniform buffer target binding.
         */
        static void unbind();

        /**
         * @brief Reallocates the buffer storage while preserving the usage hint.
         *
         * @param size New buffer size in bytes.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> resize(std::size_t size);

        /**
         * @brief Uploads a byte range into the buffer.
         *
         * @param data Pointer to the source bytes.
         * @param size Number of bytes to upload.
         * @param offset Destination byte offset inside the buffer.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> set_data(const void* data, std::size_t size, std::size_t offset = 0);

        /**
         * @brief Binds the whole buffer to a uniform block binding point.
         *
         * @param bindingPoint OpenGL uniform buffer binding point.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> bind_base(u32 bindingPoint) const;

        /**
         * @brief Binds a subrange of the buffer to a uniform block binding point.
         *
         * @param bindingPoint OpenGL uniform buffer binding point.
         * @param offset Byte offset of the range to bind.
         * @param size Size of the range in bytes.
         * @return Empty result on success, or a graphics error on failure.
         * @note The offset must satisfy GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT.
         */
        [[nodiscard]] GraphicsResult<void> bind_range(u32 bindingPoint, std::size_t offset, std::size_t size) const;

        /**
         * @brief Checks whether this object owns a GPU buffer.
         *
         * @return True when the OpenGL buffer id is non-zero.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the OpenGL buffer id.
         *
         * @return Owned buffer id, or zero when invalid.
         */
        [[nodiscard]] u32 id() const;

        /**
         * @brief Returns the allocated buffer size.
         *
         * @return Size in bytes.
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Returns the current buffer usage hint.
         *
         * @return Usage hint used for allocation and resize operations.
         */
        [[nodiscard]] BufferUsage usage() const;

        /**
         * @brief Queries the OpenGL uniform buffer range offset alignment.
         *
         * @return Required alignment in bytes, or zero if the driver reports none.
         */
        [[nodiscard]] static u32 uniform_buffer_offset_alignment();

    private:
        [[nodiscard]] GraphicsResult<void> validate_binding_point(u32 bindingPoint) const;
        [[nodiscard]] GraphicsResult<void> validate_range(std::size_t offset, std::size_t size) const;

    private:
        u32 id_ = 0;
        std::size_t size_ = 0;
        BufferUsage usage_ = BufferUsage::Dynamic;
    };
}
