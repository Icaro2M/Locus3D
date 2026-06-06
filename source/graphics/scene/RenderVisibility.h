/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::graphics
{
    /**
     * @brief Visibility and interaction flags for a renderable scene object.
     */
    struct RenderVisibility
    {
        /**
         * @brief True when the object may be submitted to the renderer.
         */
        bool visible = true;

        /**
         * @brief True when viewport picking may select this object.
         */
        bool selectable = true;

        /**
         * @brief True when the object contributes to shadow maps.
         */
        bool castsShadow = false;

        /**
         * @brief True when the object receives shadowing during lighting.
         */
        bool receivesShadow = false;
    };
}
