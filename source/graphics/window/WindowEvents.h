/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Event emitted when the logical window size changes.
     */
    struct WindowResizeEvent
    {
        /**
         * @brief New logical width in screen coordinates.
         */
        i32 width = 0;

        /**
         * @brief New logical height in screen coordinates.
         */
        i32 height = 0;
    };

    /**
     * @brief Event emitted when the framebuffer size changes.
     */
    struct FramebufferResizeEvent
    {
        /**
         * @brief New framebuffer width in pixels.
         */
        i32 width = 0;

        /**
         * @brief New framebuffer height in pixels.
         */
        i32 height = 0;
    };

    /**
     * @brief Event emitted when the window focus state changes.
     */
    struct WindowFocusEvent
    {
        /**
         * @brief True when the window is focused.
         */
        bool focused = false;
    };

    /**
     * @brief Event emitted when the window receives a close request.
     */
    struct WindowCloseEvent
    {
    };

    /**
     * @brief Event emitted when the cursor moves.
     */
    struct CursorMoveEvent
    {
        /**
         * @brief Horizontal cursor coordinate.
         */
        double x = 0.0;

        /**
         * @brief Vertical cursor coordinate.
         */
        double y = 0.0;
    };

    /**
     * @brief Event emitted when a mouse button changes state.
     */
    struct MouseButtonEvent
    {
        /**
         * @brief Backend mouse button identifier.
         */
        i32 button = 0;

        /**
         * @brief Backend action identifier.
         */
        i32 action = 0;

        /**
         * @brief Backend modifier bitmask.
         */
        i32 mods = 0;
    };

    /**
     * @brief Event emitted when the mouse wheel or touchpad scrolls.
     */
    struct ScrollEvent
    {
        /**
         * @brief Horizontal scroll offset.
         */
        double xOffset = 0.0;

        /**
         * @brief Vertical scroll offset.
         */
        double yOffset = 0.0;
    };

    /**
     * @brief Event emitted when a keyboard key changes state.
     */
    struct KeyEvent
    {
        /**
         * @brief Backend key identifier.
         */
        i32 key = 0;

        /**
         * @brief Platform scancode for the key.
         */
        i32 scancode = 0;

        /**
         * @brief Backend action identifier.
         */
        i32 action = 0;

        /**
         * @brief Backend modifier bitmask.
         */
        i32 mods = 0;
    };

}
