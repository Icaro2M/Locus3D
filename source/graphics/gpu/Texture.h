#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    enum class TextureFilter
    {
        Nearest,
        Linear
    };

    enum class TextureWrap
    {
        ClampToEdge,
        Repeat
    };

    struct TextureCreateInfo
    {
        i32 width = 1;
        i32 height = 1;
        TextureFormat format = TextureFormat::RGBA8;
        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;
        TextureWrap wrapS = TextureWrap::ClampToEdge;
        TextureWrap wrapT = TextureWrap::ClampToEdge;
    };

    class Texture
    {
    public:
        Texture() = default;
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create_2d(const TextureCreateInfo& info);
        void destroy();

        void bind(u32 unit) const;
        void unbind(u32 unit) const;

        [[nodiscard]] GraphicsResult<void> set_data(
            TextureFormat sourceFormat,
            const void* data
        );

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] u32 id() const;
        [[nodiscard]] i32 width() const;
        [[nodiscard]] i32 height() const;
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