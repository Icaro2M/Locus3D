/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/interaction/ModalTool.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Configuration used by the edge bridge tool.
     */
    struct BridgeEdgeToolOptions {
        /**
         * @brief True when the bridge should wrap cycles as closed loops.
         *
         * The editor integration currently captures two explicit boundary edges,
         * so open bridging is the default.
         */
        bool closed = false;

        /**
         * @brief True when the second edge cycle should be reversed.
         */
        bool flipSecondCycle = false;

        /**
         * @brief Cyclic offset applied to the second cycle.
         */
        int twistOffset = 0;

        /**
         * @brief True when committed topology should be validated.
         */
        bool validateAfterExecute = true;

        /**
         * @brief True when affected normals should be rebuilt.
         */
        bool rebuildNormals = true;

        /**
         * @brief True when non-manifold topology is permitted by the operation
         * context.
         */
        bool allowNonManifold = true;
    };

    /**
     * @brief Instant tool that bridges two selected boundary edges.
     *
     * BridgeEdgeTool delegates topology compatibility, orientation, face
     * creation, and validation to BridgeEdgeOp. Activating the tool commits one
     * undoable mesh operation when exactly two edge components are selected.
     */
    class BridgeEdgeTool final : public ModalTool {
    public:
        /**
         * @brief Stable registry identifier used by the bridge edge tool.
         */
        static constexpr std::string_view Id =
            "mesh.edge.bridge";

        /**
         * @brief Creates a bridge tool with default options.
         */
        BridgeEdgeTool();

        /**
         * @brief Creates a bridge tool with custom options.
         *
         * @param options Initial tool options.
         */
        explicit BridgeEdgeTool(
            BridgeEdgeToolOptions options);

        /**
         * @brief Returns the current bridge options.
         *
         * @return Read-only options.
         */
        [[nodiscard]]
        const BridgeEdgeToolOptions& options() const;

        /**
         * @brief Changes bridge options.
         *
         * @param options New options.
         * @return True when the options were accepted.
         */
        bool set_options(
            const BridgeEdgeToolOptions& options);

        /**
         * @brief Builds the static tool descriptor.
         *
         * @return Bridge edge tool descriptor.
         */
        [[nodiscard]]
        static ToolDescriptor make_descriptor();

    protected:
        [[nodiscard]]
        bool can_activate_tool(
            const ToolContext& context) const override;

        ToolResult on_activate(
            ToolContext& context) override;

        ToolResult on_event(
            ToolContext& context,
            const ToolEvent& event) override;

        ToolResult on_confirm(
            ToolContext& context) override;

        ToolResult on_cancel(
            ToolContext& context,
            ToolCancelReason reason) override;

    private:
        [[nodiscard]]
        ToolResult commit_bridge(
            ToolContext& context,
            const MeshToolTarget& target) const;

        BridgeEdgeToolOptions options_{};
    };

} // namespace locus::editor
