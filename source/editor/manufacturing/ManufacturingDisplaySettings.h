/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssueType.h"

namespace locus::editor {

    /**
     * @brief Generic overlay visibility used by manufacturing diagnostics.
     */
    enum class ManufacturingOverlayVisibility {
        VisibleOnly,
        XRay
    };

    /**
     * @brief Visual style assigned by the editor to one manufacturing issue type.
     */
    struct ManufacturingVisualStyle {
        graphics::ColorRGBA color{ 1.0f, 0.1f, 0.1f, 1.0f };
        float surfaceAlpha = 0.34f;
        float lineAlpha = 1.0f;
        float markerAlpha = 1.0f;
        float lineWidthPixels = 4.0f;
        float markerRadiusPixels = 7.0f;
        float markerBorderWidthPixels = 1.25f;
        int priority = 0;
    };

    /**
     * @brief Per-category visibility for manufacturing diagnostics.
     */
    struct ManufacturingIssueVisibility {
        bool looseEdges = true;
        bool openBoundaries = true;
        bool nonManifoldEdges = true;
        bool normalProblems = true;
        bool invertedOrientation = true;
        bool disconnectedIslands = true;
        bool degenerateGeometry = true;
        bool selfIntersections = true;
        bool minimumFeatureSize = true;
        bool thinWalls = true;
        bool overhangs = true;
        bool supportRequired = false;
    };

    /**
     * @brief Editor-side display policy for manufacturing analysis overlays.
     */
    struct ManufacturingDisplaySettings {
        bool enabled = false;
        ManufacturingOverlayVisibility visibility =
            ManufacturingOverlayVisibility::XRay;
        kernel::manufacturing::IssueSeverity minimumSeverity =
            kernel::manufacturing::IssueSeverity::Info;
        ManufacturingIssueVisibility issueVisibility{};
        graphics::ColorRGBA markerBorderColor{ 0.02f, 0.02f, 0.02f, 1.0f };

        /**
         * @brief Returns whether an issue type is currently displayable.
         */
        [[nodiscard]] bool is_type_visible(
            kernel::manufacturing::PrintIssueType type) const noexcept;

        /**
         * @brief Returns whether an issue severity passes the display filter.
         */
        [[nodiscard]] bool is_severity_visible(
            kernel::manufacturing::IssueSeverity severity) const noexcept;

        /**
         * @brief Returns the editor-defined style for an issue type.
         */
        [[nodiscard]] ManufacturingVisualStyle style_for(
            kernel::manufacturing::PrintIssueType type) const noexcept;
    };

} // namespace locus::editor
