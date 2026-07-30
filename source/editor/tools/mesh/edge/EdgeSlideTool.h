/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/mesh/core/MeshDragOperationTool.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <string_view>

namespace locus::editor {

    /**
     * @brief Configuration used by the interactive edge slide tool.
     */
    struct EdgeSlideToolOptions {
        /**
         * @brief Object-space slide distance produced by one viewport pixel.
         *
         * The value is multiplied by ToolPointerData::visualScale when that
         * scale is positive.
         */
        float distancePerPixel = 0.01f;

        /**
         * @brief Minimum absolute distance considered an effective slide.
         *
         * Smaller distances produce an empty preview and do not create history.
         */
        float distanceEpsilon = 0.000001f;

        /**
         * @brief True when dragging left produces a positive slide distance.
         *
         * By default, dragging right produces a positive distance.
         */
        bool invertDragDirection = false;

        /**
         * @brief True when target edges are excluded from slide-rail discovery.
         */
        bool excludeTargetEdgesFromRails = true;

        /**
         * @brief True when the committed operation validates the resulting mesh.
         */
        bool validateAfterExecute = true;

        /**
         * @brief True when affected face normals should be rebuilt.
         */
        bool rebuildNormals = true;

        /**
         * @brief True when non-manifold geometry is permitted by the operation
         * context.
         */
        bool allowNonManifold = true;
    };

    /**
     * @brief Interactive tool that slides selected edges along adjacent rails.
     *
     * The tool captures the selected edges when pointer interaction begins,
     * converts horizontal pointer displacement into a signed object-space
     * distance, generates non-destructive previews, and commits the final
     * result through command history.
     */
    class EdgeSlideTool final : public MeshDragOperationTool {
    public:
        /**
         * @brief Stable registry identifier used by the edge slide tool.
         */
        static constexpr std::string_view Id =
            "mesh.edge.slide";

        /**
         * @brief Creates an edge slide tool with default options.
         */
        EdgeSlideTool();

        /**
         * @brief Creates an edge slide tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit EdgeSlideTool(
            EdgeSlideToolOptions options);

        /**
         * @brief Returns the current edge slide options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const EdgeSlideToolOptions& options() const;

        /**
         * @brief Changes the edge slide options.
         *
         * Options cannot be changed during an active interaction.
         *
         * @param options New options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const EdgeSlideToolOptions& options);

        /**
         * @brief Returns the current signed slide distance.
         *
         * @return Object-space slide distance.
         */
        [[nodiscard]]
        float distance() const;

        /**
         * @brief Builds the static tool descriptor.
         *
         * @return Edge slide tool descriptor.
         */
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

    protected:
        /**
         * @brief Captures initial pointer and scaling data.
         */
        ToolResult begin_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override;

        /**
         * @brief Updates slide distance from pointer movement.
         */
        ToolResult update_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override;

        /**
         * @brief Creates the kernel operation used for ghost preview.
         */
        [[nodiscard]]
        std::unique_ptr<kernel::modeling::IOperation>
            build_preview_operation(
                const ToolContext& context,
                const MeshToolTarget& target) const override;

        /**
         * @brief Commits the edge slide through command history.
         */
        ToolResult commit_mesh_operation(
            ToolContext& context,
            const MeshToolTarget& target) override;

        /**
         * @brief Clears temporary edge slide parameters.
         */
        void clear_mesh_operation() override;

    private:
        /**
         * @brief Sanitizes configuration values.
         *
         * @param options Options to sanitize.
         * @return Sanitized options.
         */
        [[nodiscard]]
        static EdgeSlideToolOptions sanitize_options(
            EdgeSlideToolOptions options);

        /**
         * @brief Calculates distance from the current pointer ray.
         *
         * @param event Current pointer event.
         * @return Signed object-space distance.
         */
        [[nodiscard]]
        float calculate_distance(
            const ToolEvent& event) const;

        /**
         * @brief Initializes the ray-to-slide-rail drag mapping.
         */
        [[nodiscard]]
        bool initialize_slide_drag(
            const ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target);

        /**
         * @brief Checks whether the current distance produces a change.
         *
         * @return True when the distance exceeds the configured epsilon.
         */
        [[nodiscard]]
        bool has_effective_distance() const;

        EdgeSlideToolOptions options_{};

        glm::vec2 startPosition_{ 0.0f };

        glm::vec3 slideAxisWorld_{ 1.0f, 0.0f, 0.0f };
        glm::vec3 axisOriginWorld_{ 0.0f };
        glm::vec3 startAxisPointWorld_{ 0.0f };
        glm::vec2 fallbackScreenAxis_{ 1.0f, 0.0f };

        float interactionVisualScale_ = 1.0f;
        float worldDistanceToLocalDistance_ = 1.0f;
        bool axisDragReady_ = false;
        float distance_ = 0.0f;
    };

} // namespace locus::editor
