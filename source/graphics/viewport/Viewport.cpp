/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/viewport/Viewport.h"

#include <glad/glad.h>

#include <algorithm>

namespace locus::graphics
{
    void Viewport::set_rect(const ViewportRect& rect)
    {
        state_.rect = rect;

        // OpenGL rejects zero-sized viewports, which can happen while a window is minimized.
        if (state_.rect.width <= 0)
        {
            state_.rect.width = 1;
        }

        if (state_.rect.height <= 0)
        {
            state_.rect.height = 1;
        }

        update_aspect_ratio();
        update_camera_projection();
    }

    void Viewport::resize(i32 width, i32 height)
    {
        state_.rect.width = std::max(width, 1);
        state_.rect.height = std::max(height, 1);

        update_aspect_ratio();
        update_camera_projection();
    }

    void Viewport::sync_with_window(const Window& window)
    {
        set_rect(ViewportRect{
            0,
            0,
            window.framebuffer_width(),
            window.framebuffer_height()
            });
    }

    void Viewport::begin_frame()
    {
        // Apply the viewport before clearing so only this framebuffer region is affected.
        glViewport(
            state_.rect.x,
            state_.rect.y,
            state_.rect.width,
            state_.rect.height
        );

        if (settings_.depthTest)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        glClearColor(
            settings_.clearState.color.r,
            settings_.clearState.color.g,
            settings_.clearState.color.b,
            settings_.clearState.color.a
        );

        glClearDepth(settings_.clearState.depth);
        glClearStencil(settings_.clearState.stencil);

        GLbitfield clearMask = 0;

        // Build the clear mask from settings so color/depth/stencil can be toggled independently.
        if (settings_.clearColor)
        {
            clearMask |= GL_COLOR_BUFFER_BIT;
        }

        if (settings_.clearDepth)
        {
            clearMask |= GL_DEPTH_BUFFER_BIT;
        }

        if (settings_.clearStencil)
        {
            clearMask |= GL_STENCIL_BUFFER_BIT;
        }

        if (clearMask != 0)
        {
            glClear(clearMask);
        }
    }

    void Viewport::set_clear_color(const ColorRGBA& color)
    {
        settings_.clearState.color = color;
    }

    void Viewport::set_depth_test_enabled(bool enabled)
    {
        settings_.depthTest = enabled;
    }

    Camera& Viewport::camera()
    {
        return camera_;
    }

    const Camera& Viewport::camera() const
    {
        return camera_;
    }

    ViewportSettings& Viewport::settings()
    {
        return settings_;
    }

    const ViewportSettings& Viewport::settings() const
    {
        return settings_;
    }

    const ViewportState& Viewport::state() const
    {
        return state_;
    }

    void Viewport::update_aspect_ratio()
    {
        state_.aspectRatio =
            static_cast<f32>(state_.rect.width) /
            static_cast<f32>(state_.rect.height);
    }

    void Viewport::update_camera_projection()
    {
        camera_.projection().set_aspect_ratio(state_.aspectRatio);
    }
}
