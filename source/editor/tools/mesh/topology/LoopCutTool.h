/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/mesh/core/MeshDragOperationTool.h"

#include <glm/vec2.hpp>

#include <cstddef>
#include <memory>
#include <string_view>

namespace locus::editor {

    /**
     * @brief Configuration used by the interactive loop cut tool.
     */
    struct LoopCutToolOptions {
        /**
         * @brief Factor change produced by one viewport pixel.
         *
         * The value is multiplied by ToolPointerData::visualScale when that
         * scale is positive.
         */
        float factorPerPixel = 0.005f;

        /**
         * @brief Minimum factor change considered relevant.
         */
        float factorEpsilon = 0.000001f;

        /**
         * @brief Minimum permitted single-cut factor.
         */
        float minimumFactor = 0.0001f;

        /**
         * @brief Maximum permitted single-cut factor.
         */
        float maximumFactor = 0.9999f;

        /**
         * @brief Initial position used by single-cut mode.
         */
        float initialFactor = 0.5f;

        /**
         * @brief Number of cuts inserted on every target edge.
         *
         * Values below one are sanitized to one.
         */
        std::size_t cuts = 1;

        /**
         * @brief True when cuts use uniform spacing.
         *
         * Multiple cuts are always uniformly spaced by the current kernel
         * operation. For one cut, false allows interactive positioning.
         */
        bool evenSpacing = false;

        /**
         * @brief True when dragging left increases the cut factor.
         *
         * By default, dragging right increases the factor.
         */
        bool invertDragDirection = false;

        /**
         * @brief True when the committed operation validates the resulting mesh.
         */
        bool validateAfterExecute = true;

        /**
         * @brief True when affected normals should be rebuilt.
         */
        bool rebuildNormals = true;

        /**
         * @brief True when non-manifold topology is permitted.
         */
        bool allowNonManifold = true;
    };

    /**
     * @brief Interactively cuts selected mesh edges.
     *
     * LoopCutTool operates on the explicit edge selection captured when the
     * interaction begins. It does not currently discover an edge ring by
     * itself; target-ring discovery belongs to a future kernel query or an
     * expanded LoopCutOp contract.
     */
    class LoopCutTool final : public MeshDragOperationTool {
    public:
        /**
         * @brief Stable registry identifier used by the loop cut tool.
         */
        static constexpr std::string_view Id =
            "mesh.topology.loop_cut";

        /**
         * @brief Creates a loop cut tool with default options.
         */
        LoopCutTool();

        /**
         * @brief Creates a loop cut tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit LoopCutTool(
            LoopCutToolOptions options);

        /**
         * @brief Returns the current loop cut options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const LoopCutToolOptions& options() const;

        /**
         * @brief Changes the loop cut options.
         *
         * Options cannot be changed during an active interaction.
         *
         * @param options New options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const LoopCutToolOptions& options);

        /**
         * @brief Returns the current single-cut factor.
         *
         * @return Parametric position from edge vertexA to vertexB.
         */
        [[nodiscard]]
        float factor() const;

        /**
         * @brief Returns the configured cut count.
         *
         * @return Number of cuts per target edge.
         */
        [[nodiscard]]
        std::size_t cuts() const;

    protected:
        ToolResult begin_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override;

        ToolResult update_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override;

        [[nodiscard]]
        std::unique_ptr<kernel::modeling::IOperation>
            build_preview_operation(
                const ToolContext& context,
                const MeshToolTarget& target) const override;

        ToolResult commit_mesh_operation(
            ToolContext& context,
            const MeshToolTarget& target) override;

        void clear_mesh_operation() override;

    private:
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

        [[nodiscard]]
        static LoopCutToolOptions sanitize_options(
            LoopCutToolOptions options);

        [[nodiscard]]
        float calculate_factor(
            const ToolEvent& event) const;

        /**
         * @brief Checks whether pointer movement controls the cut position.
         *
         * @return True for a single cut with even spacing disabled.
         */
        [[nodiscard]]
        bool uses_interactive_factor() const;

        LoopCutToolOptions options_{};

        glm::vec2 startPosition_{ 0.0f };

        float interactionVisualScale_ = 1.0f;
        float factor_ = 0.5f;
    };

} // namespace locus::editor