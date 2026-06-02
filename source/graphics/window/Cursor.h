/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::graphics
{
    /**
     * @brief Cursor visibility and capture mode.
     */
    enum class CursorMode
    {
        Normal,
        Hidden,
        Disabled
    };

    /**
     * @brief Standard cursor shape requested by the window.
     */
    enum class CursorShape
    {
        Arrow,
        IBeam,
        Crosshair,
        Hand,
        HorizontalResize,
        VerticalResize
    };

    /**
     * @brief Cursor position in window coordinates.
     */
    struct CursorPosition
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

}
