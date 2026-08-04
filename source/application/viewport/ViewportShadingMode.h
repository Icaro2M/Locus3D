/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::application {

    /**
     * @brief Primary visual shading mode for an editor viewport.
     */
    enum class ViewportShadingMode {
        Solid,
        Wireframe
    };

    /**
     * @brief Render-pass policy derived from the viewport shading mode.
     */
    struct ViewportShadingFrameConfig {
        bool surfaceColorPass = true;
        bool surfaceDepthPrepass = false;
        bool topologyVisibleEdges = true;
        bool topologyOccludedEdges = false;
        bool topologySurfaceOverlays = true;
    };

    /**
     * @brief Independent visual display options for one editor viewport.
     */
    struct ViewportDisplaySettings {
        ViewportShadingMode shadingMode = ViewportShadingMode::Solid;
        bool showFaceOrientation = false;
    };

    /**
     * @brief Returns the alternate shading mode for two-state toggles.
     */
    [[nodiscard]] constexpr ViewportShadingMode toggle_viewport_shading_mode(
        ViewportShadingMode mode) noexcept
    {
        return mode == ViewportShadingMode::Solid
            ? ViewportShadingMode::Wireframe
            : ViewportShadingMode::Solid;
    }

    /**
     * @brief Builds the generic frame policy for a shading mode.
     */
    [[nodiscard]] constexpr ViewportShadingFrameConfig
    viewport_shading_frame_config(ViewportShadingMode mode) noexcept
    {
        switch (mode) {
        case ViewportShadingMode::Wireframe:
            return {
                false,
                true,
                true,
                false,
                false
            };

        case ViewportShadingMode::Solid:
            break;
        }

        return {};
    }

    /**
     * @brief Builds the generic frame policy for combined display settings.
     */
    [[nodiscard]] constexpr ViewportShadingFrameConfig
    viewport_shading_frame_config(
        const ViewportDisplaySettings& settings) noexcept
    {
        ViewportShadingFrameConfig config =
            viewport_shading_frame_config(settings.shadingMode);

        if (settings.showFaceOrientation) {
            config.surfaceColorPass = true;
            config.surfaceDepthPrepass = false;
            config.topologyVisibleEdges = true;
            config.topologySurfaceOverlays = true;
        }

        return config;
    }

} // namespace locus::application
