#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Texture.h"

namespace locus::graphics
{
    struct FramebufferCreateInfo
    {
        i32 width = 1;
        i32 height = 1;

        TextureFormat colorFormat = TextureFormat::RGBA8;
        TextureFormat depthStencilFormat = TextureFormat::Depth24Stencil8;

        bool createColorAttachment = true;
        bool createDepthStencilAttachment = true;
    };

    class Framebuffer
    {
    public:
        Framebuffer() = default;
        ~Framebuffer();

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        Framebuffer(Framebuffer&& other) noexcept;
        Framebuffer& operator=(Framebuffer&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(const FramebufferCreateInfo& info);
        void destroy();

        void bind() const;
        static void bind_default();

        void clear_color(float r, float g, float b, float a) const;
        void clear_depth_stencil(float depth, i32 stencil) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] u32 id() const;
        [[nodiscard]] i32 width() const;
        [[nodiscard]] i32 height() const;

        [[nodiscard]] const Texture* color_attachment() const;
        [[nodiscard]] const Texture* depth_stencil_attachment() const;

    private:
        [[nodiscard]] GraphicsResult<void> validate() const;

    private:
        u32 id_ = 0;
        i32 width_ = 0;
        i32 height_ = 0;

        Texture colorAttachment_;
        Texture depthStencilAttachment_;

        bool hasColorAttachment_ = false;
        bool hasDepthStencilAttachment_ = false;
    };
}