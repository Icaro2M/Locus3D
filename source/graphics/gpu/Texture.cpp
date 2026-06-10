#include "graphics/gpu/Texture.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <utility>

namespace locus::graphics
{
    Texture::~Texture()
    {
        destroy();
    }

    Texture::Texture(Texture&& other) noexcept
    {
        *this = std::move(other);
    }

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        id_ = other.id_;
        width_ = other.width_;
        height_ = other.height_;
        format_ = other.format_;

        other.id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.format_ = TextureFormat::Unknown;

        return *this;
    }

    GraphicsResult<void> Texture::create_2d(const TextureCreateInfo& info)
    {
        if (id_ != 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create Texture because it already owns a GPU texture."
            );
        }

        if (info.width <= 0 || info.height <= 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot create Texture with invalid dimensions."
            );
        }

        if (info.format == TextureFormat::Unknown)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot create Texture with unknown format."
            );
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &id_);

        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::TextureCreationFailed,
                "Failed to create OpenGL texture."
            );
        }

        width_ = info.width;
        height_ = info.height;
        format_ = info.format;

        glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(gl_filter(info.minFilter)));
        glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(gl_filter(info.magFilter)));
        glTextureParameteri(id_, GL_TEXTURE_WRAP_S, static_cast<GLint>(gl_wrap(info.wrapS)));
        glTextureParameteri(id_, GL_TEXTURE_WRAP_T, static_cast<GLint>(gl_wrap(info.wrapT)));

        glTextureStorage2D(
            id_,
            1,
            gl_internal_format(format_),
            width_,
            height_
        );

        return {};
    }

    void Texture::destroy()
    {
        if (id_ != 0)
        {
            glDeleteTextures(1, &id_);
            id_ = 0;
        }

        width_ = 0;
        height_ = 0;
        format_ = TextureFormat::Unknown;
    }

    void Texture::bind(u32 unit) const
    {
        glBindTextureUnit(unit, id_);
    }

    void Texture::unbind(u32 unit) const
    {
        glBindTextureUnit(unit, 0);
    }

    GraphicsResult<void> Texture::set_data(
        TextureFormat sourceFormat,
        const void* data
    )
    {
        if (id_ == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload data to an invalid Texture."
            );
        }

        if (data == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot upload null data to Texture."
            );
        }

        if (is_depth_stencil_format(format_))
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload color data to a depth-stencil Texture."
            );
        }

        if (sourceFormat == TextureFormat::Unknown || is_depth_stencil_format(sourceFormat))
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot upload Texture data with invalid source format."
            );
        }

        glTextureSubImage2D(
            id_,
            0,
            0,
            0,
            width_,
            height_,
            gl_pixel_format(sourceFormat),
            gl_pixel_type(sourceFormat),
            data
        );

        return {};
    }

    bool Texture::is_valid() const
    {
        return id_ != 0;
    }

    u32 Texture::id() const
    {
        return id_;
    }

    i32 Texture::width() const
    {
        return width_;
    }

    i32 Texture::height() const
    {
        return height_;
    }

    TextureFormat Texture::format() const
    {
        return format_;
    }

    u32 Texture::gl_internal_format(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::R8:
            return GL_R8;

        case TextureFormat::RGB8:
            return GL_RGB8;

        case TextureFormat::RGBA8:
            return GL_RGBA8;

        case TextureFormat::Depth24Stencil8:
            return GL_DEPTH24_STENCIL8;

        case TextureFormat::Unknown:
            break;
        }

        return GL_RGBA8;
    }

    u32 Texture::gl_pixel_format(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::R8:
            return GL_RED;

        case TextureFormat::RGB8:
            return GL_RGB;

        case TextureFormat::RGBA8:
            return GL_RGBA;

        case TextureFormat::Depth24Stencil8:
            return GL_DEPTH_STENCIL;

        case TextureFormat::Unknown:
            break;
        }

        return GL_RGBA;
    }

    u32 Texture::gl_pixel_type(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::Depth24Stencil8:
            return GL_UNSIGNED_INT_24_8;

        case TextureFormat::R8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:
        case TextureFormat::Unknown:
            break;
        }

        return GL_UNSIGNED_BYTE;
    }

    u32 Texture::gl_filter(TextureFilter filter)
    {
        switch (filter)
        {
        case TextureFilter::Nearest:
            return GL_NEAREST;

        case TextureFilter::Linear:
            return GL_LINEAR;
        }

        return GL_LINEAR;
    }

    u32 Texture::gl_wrap(TextureWrap wrap)
    {
        switch (wrap)
        {
        case TextureWrap::ClampToEdge:
            return GL_CLAMP_TO_EDGE;

        case TextureWrap::Repeat:
            return GL_REPEAT;
        }

        return GL_CLAMP_TO_EDGE;
    }

    bool Texture::is_depth_stencil_format(TextureFormat format)
    {
        return format == TextureFormat::Depth24Stencil8;
    }
}