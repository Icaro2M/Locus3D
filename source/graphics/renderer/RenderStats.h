/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Per-frame counters collected by the renderer.
     */
    struct RenderStats
    {
        /**
         * @brief Number of scene objects visited by the renderer.
         */
        u32 objectsSubmitted = 0;

        /**
         * @brief Number of objects that issued a draw call.
         */
        u32 objectsDrawn = 0;

        /**
         * @brief Number of objects skipped because they were not drawable.
         */
        u32 objectsSkipped = 0;

        /**
         * @brief Number of GPU draw calls issued.
         */
        u32 drawCalls = 0;

        /**
         * @brief Clears all counters for a new frame.
         */
        void reset()
        {
            objectsSubmitted = 0;
            objectsDrawn = 0;
            objectsSkipped = 0;
            drawCalls = 0;
        }
    };
}
