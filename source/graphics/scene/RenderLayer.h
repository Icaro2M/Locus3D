/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Logical render buckets used to order and filter scene objects.
     */
    enum class RenderLayer : u32
    {
        /**
         * @brief Standard model geometry.
         */
        Default = 0,

        /**
         * @brief Construction grid and ground-reference helpers.
         */
        Grid = 1,

        /**
         * @brief Screen or viewport overlays.
         */
        Overlay = 2,

        /**
         * @brief Interactive transform handles.
         */
        Gizmo = 3,

        /**
         * @brief Diagnostic and development-only render primitives.
         */
        Debug = 4
    };
}
