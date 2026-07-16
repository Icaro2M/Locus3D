/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/mesh/core/MeshDragOperationTool.h"

#include <glm/vec2.hpp>

#include <memory>
#include <string_view>

namespace locus::editor {

    /**
     * @brief Configuration used by the interactive face extrusion tool.
     */
    struct ExtrudeFaceToolOptions {
        /**
         * @brief Extrusion distance produced by one viewport pixel.
         *
         * This value is multiplied by ToolPointerData::visualScale when that
         * scale is positive.
         */
        float distancePerPixel = 0.01f;

        /**
         * @brief Minimum absolute distance considered a visible extrusion.
         *
         * Distances below this threshold produce an empty preview and do not
         * create a history entry.
         */
        float distanceEpsilon = 0.000001f;

        /**
         * @brief True when the original source faces should remain.
         */
        bool keepSourceFace = false;

        /**
         * @brief True when downward dragging produces positive extrusion.
         *
         * By default, upward viewport dragging produces positive distance.
         */
        bool invertDragDirection = false;

        /**
         * @brief True when the committed operation validates the resulting mesh.
         */
        bool validateAfterExecute = true;

        /**
         * @brief True when the operation rebuilds affected normals.
         */
        bool rebuildNormals = true;

        /**
         * @brief True when the operation may create non-manifold topology.
         */
        bool allowNonManifold = true;
    };

    /**
     * @brief Interactive tool that extrudes selected mesh faces.
     *
     * The tool captures the selected face handles when pointer interaction
     * begins. Pointer movement changes a signed extrusion distance, kernel
     * previews are generated non-destructively, and confirmation executes one
     * undoable ApplyMeshOperationCommand.
     */
    class ExtrudeFaceTool final : public MeshDragOperationTool {
    public:
        /**
         * @brief Stable registry identifier used by the extrusion tool.
         */
        static constexpr std::string_view Id =
            "mesh.face.extrude";

        /**
         * @brief Creates a face extrusion tool.
         */
        ExtrudeFaceTool();

        /**
         * @brief Creates a face extrusion tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit ExtrudeFaceTool(
            ExtrudeFaceToolOptions options);

        /**
         * @brief Returns the current extrusion options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const ExtrudeFaceToolOptions& options() const;

        /**
         * @brief Changes the extrusion options.
         *
         * Options cannot be changed while an interaction is active.
         *
         * @param options New tool options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const ExtrudeFaceToolOptions& options);

        /**
         * @brief Returns the current signed extrusion distance.
         *
         * @return Distance along each target face normal.
         */
        [[nodiscard]]
        float distance() const;

    protected:
        /**
         * @brief Captures initial pointer and scaling data.
         */
        ToolResult begin_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override;

        /**
         * @brief Updates extrusion distance from pointer movement.
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
         * @brief Executes the final extrusion through command history.
         */
        ToolResult commit_mesh_operation(
            ToolContext& context,
            const MeshToolTarget& target) override;

        /**
         * @brief Clears temporary extrusion parameters.
         */
        void clear_mesh_operation() override;

    private:
        /**
         * @brief Builds the tool descriptor.
         *
         * @return Static extrusion tool metadata.
         */
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

        /**
         * @brief Calculates distance from the current pointer position.
         *
         * @param event Current pointer event.
         * @return Signed extrusion distance.
         */
        [[nodiscard]]
        float calculate_distance(
            const ToolEvent& event) const;

        /**
         * @brief Checks whether the current distance produces geometry.
         *
         * @return True when the distance exceeds the configured epsilon.
         */
        [[nodiscard]]
        bool has_effective_distance() const;

        ExtrudeFaceToolOptions options_{};

        glm::vec2 startPosition_{ 0.0f };

        float interactionVisualScale_ = 1.0f;
        float distance_ = 0.0f;
    };

} // namespace locus::editor