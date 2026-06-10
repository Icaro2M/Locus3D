/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Framebuffer.h"
#include "graphics/picking/PickingId.h"

namespace locus::graphics
{
    /**
     * @brief Offscreen framebuffer used to render and read picking IDs.
     */
    class PickingBuffer
    {
    public:
        /**
         * @brief Creates an empty picking buffer wrapper.
         */
        PickingBuffer() = default;

        /**
         * @brief Releases the owned framebuffer.
         */
        ~PickingBuffer();

        PickingBuffer(const PickingBuffer&) = delete;
        PickingBuffer& operator=(const PickingBuffer&) = delete;

        PickingBuffer(PickingBuffer&& other) noexcept;
        PickingBuffer& operator=(PickingBuffer&& other) noexcept;

        /**
         * @brief Creates the framebuffer used for picking.
         *
         * @param width Framebuffer width in pixels.
         * @param height Framebuffer height in pixels.
         * @return Success or creation error.
         */
        [[nodiscard]] GraphicsResult<void> create(i32 width, i32 height);

        /**
         * @brief Resizes the picking framebuffer.
         *
         * @param width New width in pixels.
         * @param height New height in pixels.
         * @return Success or resize error.
         */
        [[nodiscard]] GraphicsResult<void> resize(i32 width, i32 height);

        /**
         * @brief Destroys the owned framebuffer.
         */
        void destroy();

        /**
         * @brief Binds the picking framebuffer for rendering.
         */
        void bind() const;

        /**
         * @brief Binds the default framebuffer.
         */
        static void bind_default();

        /**
         * @brief Clears color, depth, and stencil values.
         */
        void clear() const;

        /**
         * @brief Reads a picking ID from a pixel.
         *
         * @param x Pixel X coordinate.
         * @param y Pixel Y coordinate.
         * @return Decoded picking ID, or invalid when out of range.
         */
        [[nodiscard]] PickingId read_id(i32 x, i32 y) const;

        /**
         * @brief Checks whether the picking framebuffer exists.
         *
         * @return True when the underlying framebuffer is valid.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the framebuffer width.
         *
         * @return Width in pixels.
         */
        [[nodiscard]] i32 width() const;

        /**
         * @brief Returns the framebuffer height.
         *
         * @return Height in pixels.
         */
        [[nodiscard]] i32 height() const;

        /**
         * @brief Returns the underlying framebuffer.
         *
         * @return Read-only framebuffer reference.
         */
        [[nodiscard]] const Framebuffer& framebuffer() const;

    private:
        Framebuffer framebuffer_;
    };
}
