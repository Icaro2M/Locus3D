/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/manufacturing/ManufacturingDisplaySettings.h"
#include "graphics/primitives/PointMarker.h"
#include "graphics/primitives/ScreenSpaceLine.h"
#include "graphics/primitives/SurfaceOverlay.h"

#include <cstddef>
#include <string>

namespace locus::editor {

    class EditorScene;
    class ManufacturingSync;

    /**
     * @brief Generic graphics batches used to render manufacturing diagnostics.
     */
    struct ManufacturingOverlayBatches {
        graphics::SurfaceOverlayBatch surfaces;
        graphics::ScreenSpaceLineBatch lines;
        graphics::PointMarkerBatch markers;

        /**
         * @brief Checks whether every batch is empty.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return surfaces.empty() && lines.empty() && markers.empty();
        }
    };

    /**
     * @brief Diagnostics produced by ManufacturingRenderAdapter.
     */
    struct ManufacturingRenderResult {
        std::size_t nodeCount = 0;
        std::size_t issueCount = 0;
        std::size_t surfaceTriangleCount = 0;
        std::size_t lineCount = 0;
        std::size_t markerCount = 0;
        std::size_t skippedIssueCount = 0;
        std::size_t invalidHandleCount = 0;
        std::string message;
    };

    /**
     * @brief Converts manufacturing reports to generic graphics overlay batches.
     */
    class ManufacturingRenderAdapter {
    public:
        /**
         * @brief Builds generic overlay batches from synchronized manufacturing state.
         */
        [[nodiscard]] static ManufacturingOverlayBatches build_overlays(
            const EditorScene& scene,
            const ManufacturingSync& manufacturing,
            ManufacturingRenderResult* result = nullptr);
    };

} // namespace locus::editor
