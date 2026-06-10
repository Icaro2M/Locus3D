/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Texture.h"

namespace locus::graphics
{
    /**
     * @brief Parameters used to create a framebuffer and its attachments.
     */
    struct FramebufferCreateInfo
    {
        /**
         * @brief Framebuffer width in pixels.
         */
        i32 width = 1;

        /**
         * @brief Framebuffer height in pixels.
         */
        i32 height = 1;

        /**
         * @brief Texture format for the optional color attachment.
         */
        TextureFormat colorFormat = TextureFormat::RGBA8;

        /**
         * @brief Texture format for the optional depth-stencil attachment.
         */
        TextureFormat depthStencilFormat = TextureFormat::Depth24Stencil8;

        /**
         * @brief True to create and attach a color texture.
         */
        bool createColorAttachment = true;

        /**
         * @brief True to create and attach a depth-stencil texture.
         */
        bool createDepthStencilAttachment = true;
    };

    /**
     * @brief RAII wrapper for an OpenGL framebuffer and owned attachments.
     */
    class Framebuffer
    {
    public:
        /**
         * @brief Creates an empty framebuffer wrapper.
         */
        Framebuffer() = default;

        /**
         * @brief Deletes the owned framebuffer and attachments.
         */
        ~Framebuffer();

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        Framebuffer(Framebuffer&& other) noexcept;
        Framebuffer& operator=(Framebuffer&& other) noexcept;

        /**
         * @brief Creates a framebuffer and configured attachments.
         *
         * @param info Framebuffer creation parameters.
         * @return Success or creation error.
         */
        [[nodiscard]] GraphicsResult<void> create(const FramebufferCreateInfo& info);

        /**
         * @brief Destroys the framebuffer and its attachments.
         */
        void destroy();

        /**
         * @brief Binds this framebuffer for drawing and reading.
         */
        void bind() const;

        /**
         * @brief Binds the default framebuffer.
         */
        static void bind_default();

        /**
         * @brief Clears the first color attachment.
         *
         * @param r Red channel.
         * @param g Green channel.
         * @param b Blue channel.
         * @param a Alpha channel.
         */
        void clear_color(float r, float g, float b, float a) const;

        /**
         * @brief Clears the depth-stencil attachment.
         *
         * @param depth Depth clear value.
         * @param stencil Stencil clear value.
         */
        void clear_depth_stencil(float depth, i32 stencil) const;

        /**
         * @brief Checks whether this wrapper owns an OpenGL framebuffer.
         *
         * @return True when the framebuffer ID is non-zero.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the OpenGL framebuffer ID.
         *
         * @return Framebuffer object ID.
         */
        [[nodiscard]] u32 id() const;

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
         * @brief Returns the color attachment, if one exists.
         *
         * @return Color texture pointer or nullptr.
         */
        [[nodiscard]] const Texture* color_attachment() const;

        /**
         * @brief Returns the depth-stencil attachment, if one exists.
         *
         * @return Depth-stencil texture pointer or nullptr.
         */
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
