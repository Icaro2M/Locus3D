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
     * @brief Configuration used by the interactive edge bevel tool.
     */
    struct BevelToolOptions {
        /**
         * @brief Object-space bevel width produced by one viewport pixel.
         *
         * The value is multiplied by ToolPointerData::visualScale when that
         * scale is positive.
         */
        float widthPerPixel = 0.01f;

        /**
         * @brief Minimum width considered an effective bevel.
         *
         * Smaller widths produce an empty preview and no history entry.
         */
        float widthEpsilon = 0.000001f;

        /**
         * @brief Optional maximum interactive bevel width.
         *
         * A value of zero disables the explicit tool-side maximum. The kernel
         * may still limit individual corner cuts according to local edge length.
         */
        float maximumWidth = 0.0f;

        /**
         * @brief True when dragging left increases bevel width.
         *
         * By default, dragging right increases the width.
         */
        bool invertDragDirection = false;

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
     * @brief Interactive tool that bevels selected mesh edges.
     *
     * The tool captures selected edge handles when interaction begins, converts
     * horizontal pointer displacement into a positive object-space width,
     * generates non-destructive previews, and commits the final topology change
     * through command history.
     */
    class BevelTool final : public MeshDragOperationTool {
    public:
        /**
         * @brief Stable registry identifier used by the bevel tool.
         */
        static constexpr std::string_view Id =
            "mesh.edge.bevel";

        /**
         * @brief Creates an edge bevel tool with default options.
         */
        BevelTool();

        /**
         * @brief Creates an edge bevel tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit BevelTool(
            BevelToolOptions options);

        /**
         * @brief Returns the current bevel options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const BevelToolOptions& options() const;

        /**
         * @brief Changes the bevel options.
         *
         * Options cannot be changed during an active interaction.
         *
         * @param options New options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const BevelToolOptions& options);

        /**
         * @brief Returns the current bevel width.
         *
         * @return Positive object-space bevel width.
         */
        [[nodiscard]]
        float width() const;

        /**
         * @brief Builds the static tool descriptor.
         *
         * @return Bevel tool descriptor.
         */
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

    protected:
        /**
         * @brief Captures initial pointer and visual-scale data.
         */
        ToolResult begin_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override;

        /**
         * @brief Updates bevel width from pointer movement.
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
         * @brief Commits the bevel through command history.
         */
        ToolResult commit_mesh_operation(
            ToolContext& context,
            const MeshToolTarget& target) override;

        /**
         * @brief Clears temporary bevel parameters.
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
        static BevelToolOptions sanitize_options(
            BevelToolOptions options);

        /**
         * @brief Calculates width from horizontal pointer displacement.
         *
         * @param event Current pointer event.
         * @return Positive, optionally limited bevel width.
         */
        [[nodiscard]]
        float calculate_width(
            const ToolEvent& event) const;

        /**
         * @brief Initializes the ray-to-bevel-width drag mapping.
         */
        [[nodiscard]]
        bool initialize_width_drag(
            const ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target);

        /**
         * @brief Checks whether the current width produces a change.
         *
         * @return True when width exceeds the configured epsilon.
         */
        [[nodiscard]]
        bool has_effective_width() const;

        BevelToolOptions options_{};

        glm::vec2 startPosition_{ 0.0f };

        glm::vec3 widthAxisWorld_{ 1.0f, 0.0f, 0.0f };
        glm::vec3 axisOriginWorld_{ 0.0f };
        glm::vec3 startAxisPointWorld_{ 0.0f };
        glm::vec2 fallbackScreenAxis_{ 1.0f, 0.0f };

        float interactionVisualScale_ = 1.0f;
        float worldDistanceToLocalWidth_ = 1.0f;
        bool axisDragReady_ = false;
        float width_ = 0.0f;
    };

} // namespace locus::editor
