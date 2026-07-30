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
     * @brief Configuration used by the interactive face inset tool.
     */
    struct InsetFaceToolOptions {
        /**
         * @brief Inset factor produced by one viewport pixel.
         *
         * The value is multiplied by ToolPointerData::visualScale when that
         * scale is positive.
         */
        float factorPerPixel = 0.005f;

        /**
         * @brief Minimum factor considered an effective inset.
         *
         * Smaller factors produce an empty preview and do not create history.
         */
        float factorEpsilon = 0.000001f;

        /**
         * @brief Largest factor produced by pointer interaction.
         *
         * InsetFaceOp requires a factor strictly smaller than one.
         */
        float maximumFactor = 0.95f;

        /**
         * @brief True when downward dragging increases the inset factor.
         *
         * By default, upward viewport dragging increases the factor.
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
         * @brief True when the operation may create non-manifold topology.
         */
        bool allowNonManifold = true;
    };

    /**
     * @brief Interactive tool that insets selected mesh faces.
     *
     * The tool captures selected faces when interaction begins, converts pointer
     * displacement into an interpolation factor, builds non-destructive kernel
     * previews, and commits the final result through command history.
     */
    class InsetFaceTool final : public MeshDragOperationTool {
    public:
        /**
         * @brief Stable registry identifier used by the inset tool.
         */
        static constexpr std::string_view Id =
            "mesh.face.inset";

        /**
         * @brief Creates a face inset tool with default options.
         */
        InsetFaceTool();

        /**
         * @brief Creates a face inset tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit InsetFaceTool(
            InsetFaceToolOptions options);

        /**
         * @brief Returns the current inset options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const InsetFaceToolOptions& options() const;

        /**
         * @brief Changes the inset options.
         *
         * Options cannot be changed during an active interaction.
         *
         * @param options New options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const InsetFaceToolOptions& options);

        /**
         * @brief Returns the current inset factor.
         *
         * @return Interpolation factor toward each face center.
         */
        [[nodiscard]]
        float factor() const;

        /**
         * @brief Builds the static tool descriptor.
         *
         * @return Inset tool descriptor.
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
         * @brief Updates the inset factor from pointer movement.
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
         * @brief Commits the inset through command history.
         */
        ToolResult commit_mesh_operation(
            ToolContext& context,
            const MeshToolTarget& target) override;

        /**
         * @brief Clears temporary inset parameters.
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
        static InsetFaceToolOptions sanitize_options(
            InsetFaceToolOptions options);

        /**
         * @brief Calculates the inset factor from pointer displacement.
         *
         * @param event Current pointer event.
         * @return Clamped inset factor.
         */
        [[nodiscard]]
        float calculate_factor(
            const ToolEvent& event) const;

        /**
         * @brief Initializes the ray-to-face-plane drag mapping.
         */
        [[nodiscard]]
        bool initialize_plane_drag(
            const ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target);

        /**
         * @brief Checks whether the current factor produces geometry.
         *
         * @return True when the factor is inside the effective inset range.
         */
        [[nodiscard]]
        bool has_effective_factor() const;

        InsetFaceToolOptions options_{};

        glm::vec2 startPosition_{ 0.0f };

        glm::vec3 planeOriginWorld_{ 0.0f };
        glm::vec3 planeNormalWorld_{ 0.0f, 0.0f, 1.0f };
        glm::vec3 startPlanePointWorld_{ 0.0f };
        glm::vec3 inwardDragDirectionWorld_{ 1.0f, 0.0f, 0.0f };
        glm::vec2 fallbackScreenDirection_{ 1.0f, 0.0f };

        float interactionVisualScale_ = 1.0f;
        float worldDistanceToFactor_ = 1.0f;
        bool planeDragReady_ = false;
        float factor_ = 0.0f;
    };

} // namespace locus::editor
