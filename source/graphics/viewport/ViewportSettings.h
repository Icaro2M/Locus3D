/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Mutable GPU state applied at the start of a viewport frame.
     */
    struct ViewportSettings
    {
        /**
         * @brief Values used when clearing color, depth, and stencil buffers.
         */
        ClearState clearState{
            ColorRGBA{ 0.08f, 0.08f, 0.09f, 1.0f },
            1.0f,
            0
        };

        /**
         * @brief True when the color buffer should be cleared.
         */
        bool clearColor = true;

        /**
         * @brief True when the depth buffer should be cleared.
         */
        bool clearDepth = true;

        /**
         * @brief True when the stencil buffer should be cleared.
         */
        bool clearStencil = false;

        /**
         * @brief True when depth testing should be enabled for the viewport.
         */
        bool depthTest = true;
    };
}
