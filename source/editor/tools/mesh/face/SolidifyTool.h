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
     * @brief Configuration used by the interactive face solidify tool.
     */
    struct SolidifyToolOptions {
        /**
         * @brief Fallback thickness produced by one viewport pixel.
         */
        float thicknessPerPixel = 0.01f;

        /**
         * @brief Minimum absolute thickness considered a visible solidify.
         */
        float thicknessEpsilon = 0.000001f;

        /**
         * @brief True when the original source faces should remain.
         */
        bool keepSourceFaces = true;

        /**
         * @brief True when duplicated cap faces should be created.
         */
        bool createCaps = true;

        /**
         * @brief True when side faces should be created on region boundaries.
         */
        bool createRims = true;

        /**
         * @brief True when generated cap winding should be reversed.
         */
        bool flipCaps = false;

        /**
         * @brief True when generated rim winding should be reversed.
         */
        bool flipRims = false;

        /**
         * @brief True when the fallback screen drag direction is inverted.
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
     * @brief Interactive tool that gives selected mesh faces thickness.
     */
    class SolidifyTool final : public MeshDragOperationTool {
    public:
        /**
         * @brief Stable registry identifier used by the solidify tool.
         */
        static constexpr std::string_view Id =
            "mesh.face.solidify";

        /**
         * @brief Creates a face solidify tool with default options.
         */
        SolidifyTool();

        /**
         * @brief Creates a face solidify tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit SolidifyTool(
            SolidifyToolOptions options);

        /**
         * @brief Returns the current solidify options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const SolidifyToolOptions& options() const;

        /**
         * @brief Changes the solidify options.
         *
         * @param options New options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const SolidifyToolOptions& options);

        /**
         * @brief Returns the current signed solidify thickness.
         *
         * @return Offset thickness along target face normals.
         */
        [[nodiscard]]
        float thickness() const;

        /**
         * @brief Builds the static tool descriptor.
         *
         * @return Solidify tool descriptor.
         */
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

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
        static SolidifyToolOptions sanitize_options(
            SolidifyToolOptions options);

        [[nodiscard]]
        float calculate_thickness(
            const ToolEvent& event) const;

        [[nodiscard]]
        bool initialize_axis_drag(
            const ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target);

        [[nodiscard]]
        bool has_effective_thickness() const;

        SolidifyToolOptions options_{};

        glm::vec2 startPosition_{ 0.0f };

        glm::vec3 solidifyAxisWorld_{ 0.0f, 1.0f, 0.0f };
        glm::vec3 axisOriginWorld_{ 0.0f };
        glm::vec3 startAxisPointWorld_{ 0.0f };
        glm::vec2 fallbackScreenAxis_{ 0.0f, -1.0f };

        float interactionVisualScale_ = 1.0f;
        float worldDistanceToLocalThickness_ = 1.0f;
        bool axisDragReady_ = false;
        bool keepSourceFacesForOperation_ = true;
        float thickness_ = 0.0f;
    };

} // namespace locus::editor
