/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Visual metrics used by editable topology overlays.
     */
    struct TopologyOverlayStyle
    {
        /**
         * @brief Color used for normal editable mesh edges.
         */
        ColorRGBA wireframeColor{ 0.10f, 0.11f, 0.12f, 1.0f };

        /**
         * @brief Width used for normal editable mesh edges.
         */
        float wireframeWidthPixels = 1.35f;

        /**
         * @brief Color used for the hovered editable mesh edge.
         */
        ColorRGBA hoveredEdgeColor{ 1.0f, 0.85f, 0.20f, 1.0f };

        /**
         * @brief Width used for the hovered editable mesh edge.
         */
        float hoveredEdgeWidthPixels = 2.6f;

        /**
         * @brief Color used for selected editable mesh edges.
         */
        ColorRGBA selectedEdgeColor{ 1.0f, 0.55f, 0.05f, 1.0f };

        /**
         * @brief Width used for selected editable mesh edges.
         */
        float selectedEdgeWidthPixels = 4.0f;
    };

    /**
     * @brief Default color palette for viewport drawing and overlays.
     */
    struct ViewportPalette
    {
        /**
         * @brief Viewport clear/background color.
         */
        ColorRGBA background{ 0.08f, 0.08f, 0.09f, 1.0f };

        /**
         * @brief Color used for major grid lines.
         */
        ColorRGBA gridMajor{ 0.38f, 0.38f, 0.40f, 1.0f };

        /**
         * @brief Color used for minor grid lines.
         */
        ColorRGBA gridMinor{ 0.20f, 0.20f, 0.22f, 1.0f };

        /**
         * @brief Default object surface color.
         */
        ColorRGBA objectDefault{ 0.85f, 0.85f, 0.88f, 1.0f };

        /**
         * @brief Object color used for selected state.
         */
        ColorRGBA objectSelected{ 1.0f, 0.68f, 0.18f, 1.0f };

        /**
         * @brief Object color used for hover state.
         */
        ColorRGBA objectHovered{ 0.45f, 0.72f, 1.0f, 1.0f };

        /**
         * @brief Wireframe line color.
         */
        ColorRGBA wireframe{ 0.02f, 0.02f, 0.02f, 1.0f };

        /**
         * @brief Editable topology overlay colors and widths.
         */
        TopologyOverlayStyle topology{};

        /**
         * @brief Selection outline color.
         */
        ColorRGBA outline{ 1.0f, 0.72f, 0.20f, 1.0f };

        /**
         * @brief X-axis color.
         */
        ColorRGBA axisX{ 0.90f, 0.20f, 0.20f, 1.0f };

        /**
         * @brief Y-axis color.
         */
        ColorRGBA axisY{ 0.20f, 0.80f, 0.25f, 1.0f };

        /**
         * @brief Z-axis color.
         */
        ColorRGBA axisZ{ 0.25f, 0.45f, 1.0f, 1.0f };
    };
}
