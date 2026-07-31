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
     * @brief Configuration used by the interactive shrink/fatten tool.
     */
    struct ShrinkFattenToolOptions {
        /**
         * @brief Fallback signed distance produced by one viewport pixel.
         */
        float distancePerPixel = 0.01f;

        /**
         * @brief Minimum absolute distance considered a visible operation.
         */
        float distanceEpsilon = 0.000001f;

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
     * @brief Interactive tool that moves selected vertices along their normals.
     */
    class ShrinkFattenTool final : public MeshDragOperationTool {
    public:
        /**
         * @brief Stable registry identifier used by the shrink/fatten tool.
         */
        static constexpr std::string_view Id =
            "mesh.vertex.shrink_fatten";

        /**
         * @brief Creates a shrink/fatten tool with default options.
         */
        ShrinkFattenTool();

        /**
         * @brief Creates a shrink/fatten tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit ShrinkFattenTool(
            ShrinkFattenToolOptions options);

        /**
         * @brief Returns the current shrink/fatten options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const ShrinkFattenToolOptions& options() const;

        /**
         * @brief Changes the shrink/fatten options.
         *
         * @param options New options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const ShrinkFattenToolOptions& options);

        /**
         * @brief Returns the current signed shrink/fatten distance.
         *
         * @return Signed distance passed to ShrinkFattenOp.
         */
        [[nodiscard]]
        float distance() const;

        /**
         * @brief Builds the static tool descriptor.
         *
         * @return Shrink/fatten tool descriptor.
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
        static ShrinkFattenToolOptions sanitize_options(
            ShrinkFattenToolOptions options);

        [[nodiscard]]
        float calculate_distance(
            const ToolEvent& event) const;

        [[nodiscard]]
        bool initialize_normal_drag(
            const ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target);

        [[nodiscard]]
        bool has_effective_distance() const;

        ShrinkFattenToolOptions options_{};

        glm::vec2 startPosition_{ 0.0f };

        glm::vec3 normalAxisWorld_{ 0.0f, 1.0f, 0.0f };
        glm::vec3 axisOriginWorld_{ 0.0f };
        glm::vec3 startAxisPointWorld_{ 0.0f };
        glm::vec2 fallbackScreenAxis_{ 0.0f, -1.0f };

        float interactionVisualScale_ = 1.0f;
        float worldDistanceToLocalDistance_ = 1.0f;
        bool axisDragReady_ = false;
        float distance_ = 0.0f;
    };

} // namespace locus::editor
