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

        /**
         * @brief Fill color used for normal editable mesh vertices.
         */
        ColorRGBA vertexColor{ 0.95f, 0.96f, 0.98f, 1.0f };

        /**
         * @brief Radius used for normal editable mesh vertices.
         */
        float vertexRadiusPixels = 4.5f;

        /**
         * @brief Fill color used for the hovered editable mesh vertex.
         */
        ColorRGBA hoveredVertexColor{ 1.0f, 0.85f, 0.20f, 1.0f };

        /**
         * @brief Radius used for the hovered editable mesh vertex.
         */
        float hoveredVertexRadiusPixels = 6.0f;

        /**
         * @brief Fill color used for selected editable mesh vertices.
         */
        ColorRGBA selectedVertexColor{ 1.0f, 0.55f, 0.05f, 1.0f };

        /**
         * @brief Radius used for selected editable mesh vertices.
         */
        float selectedVertexRadiusPixels = 7.0f;

        /**
         * @brief Border color used for editable mesh vertices.
         */
        ColorRGBA vertexBorderColor{ 0.02f, 0.02f, 0.02f, 1.0f };

        /**
         * @brief Border width used for editable mesh vertices.
         */
        float vertexBorderWidthPixels = 1.25f;

        /**
         * @brief Translucent color used for the hovered editable mesh face.
         */
        ColorRGBA hoveredFaceColor{ 0.36f, 0.70f, 1.0f, 0.24f };

        /**
         * @brief Translucent color used for selected editable mesh faces.
         */
        ColorRGBA selectedFaceColor{ 0.16f, 0.48f, 1.0f, 0.38f };
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
         * @brief Diagnostic color for front-facing surface fragments.
         */
        ColorRGBA frontFaceOrientationColor{ 0.22f, 0.48f, 0.86f, 1.0f };

        /**
         * @brief Diagnostic color for back-facing surface fragments.
         */
        ColorRGBA backFaceOrientationColor{ 0.95f, 0.08f, 0.08f, 1.0f };

        /**
         * @brief Object color used for selected state.
         */
        ColorRGBA objectSelected{ 1.0f, 0.68f, 0.18f, 1.0f };

        /**
         * @brief Object color used for hover state.
         */
        ColorRGBA objectHovered{ 0.45f, 0.72f, 1.0f, 1.0f };

        /**
         * @brief Screen-space outline color used for hovered objects.
         */
        ColorRGBA hoveredObjectOutlineColor{ 1.0f, 0.85f, 0.20f, 0.70f };

        /**
         * @brief Screen-space outline width used for hovered objects.
         */
        float hoveredObjectOutlineWidthPixels = 2.0f;

        /**
         * @brief Screen-space outline color used for selected objects.
         */
        ColorRGBA selectedObjectOutlineColor{ 1.0f, 0.55f, 0.05f, 0.95f };

        /**
         * @brief Screen-space outline width used for selected objects.
         */
        float selectedObjectOutlineWidthPixels = 2.5f;

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
