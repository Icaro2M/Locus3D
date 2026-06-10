/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Texture sampling filter mode.
     */
    enum class TextureFilter
    {
        /**
         * @brief Samples the nearest texel.
         */
        Nearest,

        /**
         * @brief Samples using linear interpolation.
         */
        Linear
    };

    /**
     * @brief Texture coordinate wrapping mode.
     */
    enum class TextureWrap
    {
        /**
         * @brief Clamps coordinates to the texture edge.
         */
        ClampToEdge,

        /**
         * @brief Repeats texture coordinates outside the 0-1 range.
         */
        Repeat
    };

    /**
     * @brief Parameters used to create a 2D texture.
     */
    struct TextureCreateInfo
    {
        /**
         * @brief Texture width in pixels.
         */
        i32 width = 1;

        /**
         * @brief Texture height in pixels.
         */
        i32 height = 1;

        /**
         * @brief Internal storage format.
         */
        TextureFormat format = TextureFormat::RGBA8;

        /**
         * @brief Minification filter.
         */
        TextureFilter minFilter = TextureFilter::Linear;

        /**
         * @brief Magnification filter.
         */
        TextureFilter magFilter = TextureFilter::Linear;

        /**
         * @brief Horizontal wrap mode.
         */
        TextureWrap wrapS = TextureWrap::ClampToEdge;

        /**
         * @brief Vertical wrap mode.
         */
        TextureWrap wrapT = TextureWrap::ClampToEdge;
    };

    /**
     * @brief RAII wrapper for an OpenGL 2D texture.
     */
    class Texture
    {
    public:
        /**
         * @brief Creates an empty texture wrapper.
         */
        Texture() = default;

        /**
         * @brief Deletes the owned OpenGL texture, if any.
         */
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        /**
         * @brief Creates immutable 2D texture storage.
         *
         * @param info Texture creation parameters.
         * @return Success or creation error.
         */
        [[nodiscard]] GraphicsResult<void> create_2d(const TextureCreateInfo& info);

        /**
         * @brief Deletes the owned texture and resets cached metadata.
         */
        void destroy();

        /**
         * @brief Binds the texture to a texture unit.
         *
         * @param unit Texture unit index.
         */
        void bind(u32 unit) const;

        /**
         * @brief Unbinds any texture from a texture unit.
         *
         * @param unit Texture unit index.
         */
        void unbind(u32 unit) const;

        /**
         * @brief Uploads full color texture contents.
         *
         * @param sourceFormat Format of the source pixel data.
         * @param data Source pixel data.
         * @return Success or upload error.
         */
        [[nodiscard]] GraphicsResult<void> set_data(
            TextureFormat sourceFormat,
            const void* data
        );

        /**
         * @brief Checks whether this wrapper owns an OpenGL texture.
         *
         * @return True when the texture ID is non-zero.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the OpenGL texture ID.
         *
         * @return Texture object ID.
         */
        [[nodiscard]] u32 id() const;

        /**
         * @brief Returns the cached texture width.
         *
         * @return Width in pixels.
         */
        [[nodiscard]] i32 width() const;

        /**
         * @brief Returns the cached texture height.
         *
         * @return Height in pixels.
         */
        [[nodiscard]] i32 height() const;

        /**
         * @brief Returns the texture storage format.
         *
         * @return Texture format.
         */
        [[nodiscard]] TextureFormat format() const;

    private:
        [[nodiscard]] static u32 gl_internal_format(TextureFormat format);
        [[nodiscard]] static u32 gl_pixel_format(TextureFormat format);
        [[nodiscard]] static u32 gl_pixel_type(TextureFormat format);
        [[nodiscard]] static u32 gl_filter(TextureFilter filter);
        [[nodiscard]] static u32 gl_wrap(TextureWrap wrap);
        [[nodiscard]] static bool is_depth_stencil_format(TextureFormat format);

    private:
        u32 id_ = 0;
        i32 width_ = 0;
        i32 height_ = 0;
        TextureFormat format_ = TextureFormat::Unknown;
    };
}
