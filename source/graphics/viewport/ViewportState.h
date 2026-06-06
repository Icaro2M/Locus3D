/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Derived runtime state for a viewport.
     */
    struct ViewportState
    {
        /**
         * @brief Pixel rectangle used for the OpenGL viewport.
         */
        ViewportRect rect{ 0, 0, 1, 1 };

        /**
         * @brief Cached width divided by height.
         */
        f32 aspectRatio = 1.0f;
    };
}
