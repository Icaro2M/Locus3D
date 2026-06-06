/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/camera/Camera.h"
#include "graphics/viewport/ViewportSettings.h"
#include "graphics/viewport/ViewportState.h"
#include "graphics/window/Window.h"

namespace locus::graphics
{
    /**
     * @brief Couples viewport state, clear settings, and a camera.
     */
    class Viewport
    {
    public:
        /**
         * @brief Creates a default viewport.
         */
        Viewport() = default;

        /**
         * @brief Destroys the viewport state.
         */
        ~Viewport() = default;

        Viewport(const Viewport&) = default;
        Viewport& operator=(const Viewport&) = default;

        Viewport(Viewport&&) noexcept = default;
        Viewport& operator=(Viewport&&) noexcept = default;

        /**
         * @brief Sets the viewport rectangle in framebuffer pixels.
         *
         * @param rect New viewport rectangle.
         */
        void set_rect(const ViewportRect& rect);

        /**
         * @brief Resizes the viewport while preserving its origin.
         *
         * @param width New viewport width in pixels.
         * @param height New viewport height in pixels.
         */
        void resize(i32 width, i32 height);

        /**
         * @brief Matches the viewport rectangle to a window framebuffer.
         *
         * @param window Source window.
         */
        void sync_with_window(const Window& window);

        /**
         * @brief Applies viewport GPU state and clears enabled buffers.
         */
        void begin_frame();

        /**
         * @brief Sets the color used when clearing the viewport.
         *
         * @param color Clear color.
         */
        void set_clear_color(const ColorRGBA& color);

        /**
         * @brief Enables or disables depth testing for the viewport.
         *
         * @param enabled True to enable depth testing.
         */
        void set_depth_test_enabled(bool enabled);

        /**
         * @brief Returns the viewport camera.
         *
         * @return Mutable camera reference.
         */
        [[nodiscard]] Camera& camera();

        /**
         * @brief Returns the viewport camera.
         *
         * @return Read-only camera reference.
         */
        [[nodiscard]] const Camera& camera() const;

        /**
         * @brief Returns mutable viewport settings.
         *
         * @return Viewport settings.
         */
        [[nodiscard]] ViewportSettings& settings();

        /**
         * @brief Returns read-only viewport settings.
         *
         * @return Viewport settings.
         */
        [[nodiscard]] const ViewportSettings& settings() const;

        /**
         * @brief Returns derived viewport state.
         *
         * @return Read-only viewport state.
         */
        [[nodiscard]] const ViewportState& state() const;

    private:
        void update_aspect_ratio();
        void update_camera_projection();

    private:
        ViewportSettings settings_{};
        ViewportState state_{};
        Camera camera_{};
    };
}
