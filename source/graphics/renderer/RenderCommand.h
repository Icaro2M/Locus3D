/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "graphics/scene/RenderLayer.h"

namespace locus::graphics
{
    struct RenderObject;

    /**
     * @brief Sortable draw submission entry generated from a render object.
     */
    struct RenderCommand
    {
        /**
         * @brief Object to draw.
         */
        const RenderObject* object = nullptr;

        /**
         * @brief Layer copied from the render object.
         */
        RenderLayer layer = RenderLayer::Default;

        /**
         * @brief Sort priority derived from the render layer.
         */
        u32 priority = 0;

        /**
         * @brief Insertion order used to keep sorting stable within a priority.
         */
        u32 sequence = 0;
    };
}
