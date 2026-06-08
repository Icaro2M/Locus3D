/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::graphics
{
    /**
     * @brief Viewport shading mode selected for object rendering.
     */
    enum class ShadingMode
    {
        /**
         * @brief Draws objects with flat material color.
         */
        Solid,

        /**
         * @brief Draws objects with lighting applied.
         */
        Lit,

        /**
         * @brief Visualizes object normals.
         */
        Normal,

        /**
         * @brief Draws objects as wireframes.
         */
        Wireframe
    };
}
