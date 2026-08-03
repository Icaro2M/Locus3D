/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "graphics/appearance/ViewportPalette.h"
#include "graphics/primitives/PointMarker.h"
#include "graphics/primitives/ScreenSpaceLine.h"
#include "graphics/primitives/SurfaceOverlay.h"

#include <cstddef>
#include <string>

namespace locus::editor {

    class EditorScene;
    class SelectionState;

    /**
     * @brief Options used when building editable topology line overlays.
     */
    struct TopologyOverlayOptions {
        /**
         * @brief Excludes hidden editable mesh edges.
         */
        bool skipHiddenComponents = true;

        /**
         * @brief Visual colors and pixel widths for topology states.
         */
        graphics::TopologyOverlayStyle style{};
    };

    /**
     * @brief Diagnostics produced while building topology overlay lines.
     */
    struct TopologyOverlayResult {
        SceneNodeId nodeId{};
        std::size_t visitedEdgeCount = 0;
        std::size_t wireframeEdgeCount = 0;
        std::size_t hoveredEdgeCount = 0;
        std::size_t selectedEdgeCount = 0;
        std::size_t visitedVertexCount = 0;
        std::size_t normalVertexCount = 0;
        std::size_t hoveredVertexCount = 0;
        std::size_t selectedVertexCount = 0;
        std::size_t visitedFaceCount = 0;
        std::size_t hoveredFaceCount = 0;
        std::size_t selectedFaceCount = 0;
        std::size_t surfaceTriangleCount = 0;
        std::size_t invalidHandleCount = 0;
        std::string message;

        /**
         * @brief Checks whether any line segment was emitted.
         *
         * @return True when at least one segment exists.
         */
        [[nodiscard]] bool has_geometry() const noexcept
        {
            return wireframeEdgeCount + hoveredEdgeCount + selectedEdgeCount +
                normalVertexCount + hoveredVertexCount + selectedVertexCount +
                hoveredFaceCount + selectedFaceCount + surfaceTriangleCount > 0;
        }
    };

    /**
     * @brief Surface overlay batches for editable face states.
     */
    struct TopologySurfaceOverlayBatches {
        /**
         * @brief Hovered faces not already selected.
         */
        graphics::SurfaceOverlayBatch hovered;

        /**
         * @brief Selected faces.
         */
        graphics::SurfaceOverlayBatch selected;

        /**
         * @brief Checks whether any face surface overlay exists.
         *
         * @return True when at least one batch contains indexed triangles.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return hovered.empty() && selected.empty();
        }
    };

    /**
     * @brief Converts editor mesh topology state into generic graphics line segments.
     */
    class TopologyOverlayAdapter {
    public:
        /**
         * @brief Builds a world-space line batch for the active editable mesh.
         *
         * @param scene Editor scene containing the active mesh node.
         * @param selection Editor selection and hover state.
         * @param options Overlay conversion options.
         * @param result Optional diagnostic output.
         * @return Generic graphics line batch.
         */
        [[nodiscard]] static graphics::ScreenSpaceLineBatch build_active_mesh_lines(
            const EditorScene& scene,
            const SelectionState& selection,
            const TopologyOverlayOptions& options = {},
            TopologyOverlayResult* result = nullptr);

        /**
         * @brief Builds a world-space line batch for every visible editable mesh.
         *
         * @param scene Editor scene containing mesh nodes.
         * @param selection Editor selection and hover state.
         * @param options Overlay conversion options.
         * @param result Optional diagnostic output.
         * @return Generic graphics line batch.
         */
        [[nodiscard]] static graphics::ScreenSpaceLineBatch build_visible_mesh_lines(
            const EditorScene& scene,
            const SelectionState& selection,
            const TopologyOverlayOptions& options = {},
            TopologyOverlayResult* result = nullptr);

        /**
         * @brief Builds a world-space point marker batch for the active editable mesh.
         *
         * @param scene Editor scene containing the active mesh node.
         * @param selection Editor selection and hover state.
         * @param options Overlay conversion options.
         * @param result Optional diagnostic output.
         * @return Generic graphics point marker batch.
         */
        [[nodiscard]] static graphics::PointMarkerBatch build_active_mesh_vertex_markers(
            const EditorScene& scene,
            const SelectionState& selection,
            const TopologyOverlayOptions& options = {},
            TopologyOverlayResult* result = nullptr);

        /**
         * @brief Builds local-space translucent surface batches for active editable mesh faces.
         *
         * @param scene Editor scene containing the active mesh node.
         * @param selection Editor selection and hover state.
         * @param options Overlay conversion options.
         * @param result Optional diagnostic output.
         * @return Generic graphics surface overlay batches.
         */
        [[nodiscard]] static TopologySurfaceOverlayBatches build_active_mesh_face_surfaces(
            const EditorScene& scene,
            const SelectionState& selection,
            const TopologyOverlayOptions& options = {},
            TopologyOverlayResult* result = nullptr);
    };

} // namespace locus::editor
