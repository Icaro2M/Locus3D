/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <array>
#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    /**
     * @brief Input mode used by BridgeEdgeOp.
     */
    enum class BridgeEdgeMode {
        /**
         * @brief Bridge two explicit ordered vertex cycles.
         */
        VertexCycles,

        /**
         * @brief Bridge two explicit boundary edge cycles.
         */
        EdgeCycles,

        /**
         * @brief Bridge two selected boundary edge cycles.
         */
        SelectedBoundaryEdges
    };

    /**
     * @brief Creates side faces between two compatible boundary cycles.
     *
     * BridgeEdgeOp creates a strip of polygonal faces between two ordered vertex
     * cycles or edge cycles. It delegates all topology creation to LEMEditor.
     */
    class BridgeEdgeOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty bridge operation.
         */
        BridgeEdgeOp() = default;

        /**
         * @brief Creates a bridge operation from two ordered vertex cycles.
         *
         * @param firstCycle First ordered vertex cycle.
         * @param secondCycle Second ordered vertex cycle.
         */
        BridgeEdgeOp(
            std::vector<geometry::VertexHandle> firstCycle,
            std::vector<geometry::VertexHandle> secondCycle);

        /**
         * @brief Creates a bridge operation from two boundary edge cycles.
         *
         * @param firstEdges First boundary edge cycle.
         * @param secondEdges Second boundary edge cycle.
         * @return Configured bridge operation.
         */
        [[nodiscard]] static BridgeEdgeOp edges(
            std::vector<geometry::EdgeHandle> firstEdges,
            std::vector<geometry::EdgeHandle> secondEdges);

        /**
         * @brief Creates a bridge operation that uses selected boundary edges.
         *
         * @return Configured bridge operation.
         */
        [[nodiscard]] static BridgeEdgeOp selected_boundary_edges();

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the bridge input mode.
         *
         * @param mode Input mode.
         */
        void set_mode(BridgeEdgeMode mode);

        /**
         * @brief Returns the current bridge input mode.
         *
         * @return Input mode.
         */
        [[nodiscard]] BridgeEdgeMode mode() const;

        /**
         * @brief Replaces the explicit vertex cycles.
         *
         * @param firstCycle First ordered vertex cycle.
         * @param secondCycle Second ordered vertex cycle.
         */
        void set_vertex_cycles(
            std::vector<geometry::VertexHandle> firstCycle,
            std::vector<geometry::VertexHandle> secondCycle);

        /**
         * @brief Returns the first explicit vertex cycle.
         *
         * @return Read-only vertex cycle.
         */
        [[nodiscard]] const std::vector<geometry::VertexHandle>& first_cycle() const;

        /**
         * @brief Returns the second explicit vertex cycle.
         *
         * @return Read-only vertex cycle.
         */
        [[nodiscard]] const std::vector<geometry::VertexHandle>& second_cycle() const;

        /**
         * @brief Replaces the explicit edge cycles.
         *
         * @param firstEdges First boundary edge cycle.
         * @param secondEdges Second boundary edge cycle.
         */
        void set_edge_cycles(
            std::vector<geometry::EdgeHandle> firstEdges,
            std::vector<geometry::EdgeHandle> secondEdges);

        /**
         * @brief Returns the first explicit edge cycle.
         *
         * @return Read-only edge cycle.
         */
        [[nodiscard]] const std::vector<geometry::EdgeHandle>& first_edges() const;

        /**
         * @brief Returns the second explicit edge cycle.
         *
         * @return Read-only edge cycle.
         */
        [[nodiscard]] const std::vector<geometry::EdgeHandle>& second_edges() const;

        /**
         * @brief Clears all explicit input handles.
         */
        void clear_inputs();

        /**
         * @brief Sets whether the bridge should wrap from the last vertex to the first.
         *
         * @param closed True for closed cycles, false for open chains.
         */
        void set_closed(bool closed);

        /**
         * @brief Checks whether the bridge wraps from the last vertex to the first.
         *
         * @return True for closed cycles.
         */
        [[nodiscard]] bool closed() const;

        /**
         * @brief Sets whether the second cycle should be reversed before bridging.
         *
         * @param flipSecondCycle True to reverse the second cycle.
         */
        void set_flip_second_cycle(bool flipSecondCycle);

        /**
         * @brief Checks whether the second cycle is reversed before bridging.
         *
         * @return True when reversal is enabled.
         */
        [[nodiscard]] bool flip_second_cycle() const;

        /**
         * @brief Sets the cyclic offset applied to the second cycle.
         *
         * @param twistOffset Offset in vertices.
         */
        void set_twist_offset(int twistOffset);

        /**
         * @brief Returns the cyclic offset applied to the second cycle.
         *
         * @return Offset in vertices.
         */
        [[nodiscard]] int twist_offset() const;

    private:
        /**
         * @brief Container for two ordered bridge cycles.
         */
        struct BridgeCycles {
            std::vector<geometry::VertexHandle> first{};
            std::vector<geometry::VertexHandle> second{};
        };

        /**
         * @brief Container for one extracted edge cycle.
         */
        struct ExtractedEdgeCycle {
            std::vector<geometry::VertexHandle> vertices{};
            std::vector<geometry::EdgeHandle> edges{};
        };

        /**
         * @brief Executes the bridge operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects bridge cycles according to the current input mode.
         *
         * @param mesh Mesh used to validate and query topology.
         * @return Ordered bridge cycles.
         */
        [[nodiscard]] BridgeCycles collect_cycles(const geometry::LEM& mesh) const;

        /**
         * @brief Collects selected boundary edge cycles from the mesh.
         *
         * @param mesh Mesh to inspect.
         * @return Exactly two extracted cycles when selection is valid.
         */
        [[nodiscard]] std::vector<ExtractedEdgeCycle> collect_selected_boundary_cycles(
            const geometry::LEM& mesh) const;

        /**
         * @brief Orders a boundary edge cycle into a vertex cycle.
         *
         * @param mesh Mesh used to read edge endpoints.
         * @param edges Edges that should form one closed boundary cycle.
         * @return Extracted ordered cycle, or an empty cycle on failure.
         */
        [[nodiscard]] ExtractedEdgeCycle order_edge_cycle(
            const geometry::LEM& mesh,
            const std::vector<geometry::EdgeHandle>& edges) const;

        /**
         * @brief Extracts one connected edge cycle from an edge set.
         *
         * @param mesh Mesh used to read edge endpoints.
         * @param edges Candidate edge set.
         * @param requireAllEdges True when all provided edges must belong to the extracted cycle.
         * @return Extracted ordered cycle, or an empty cycle on failure.
         */
        [[nodiscard]] ExtractedEdgeCycle extract_one_edge_cycle(
            const geometry::LEM& mesh,
            const std::vector<geometry::EdgeHandle>& edges,
            bool requireAllEdges) const;

        /**
         * @brief Validates bridge cycles before face creation.
         *
         * @param mesh Mesh used to validate handles.
         * @param cycles Candidate bridge cycles.
         * @return True when cycles can be bridged.
         */
        [[nodiscard]] bool validate_cycles(
            const geometry::LEM& mesh,
            const BridgeCycles& cycles) const;

        /**
         * @brief Applies flip and twist settings to the second cycle.
         *
         * @param cycles Mutable bridge cycles.
         */
        void apply_cycle_options(BridgeCycles& cycles) const;

        /**
         * @brief Reorients two open single-edge cycles to avoid crossed bridge quads.
         *
         * @param mesh Mesh used to compare endpoint distances.
         * @param cycles Mutable bridge cycles.
         */
        void orient_open_single_edge_bridge(
            const geometry::LEM& mesh,
            BridgeCycles& cycles) const;

        /**
         * @brief Checks whether a vertex handle already exists in a vector.
         *
         * @param handles Handle vector.
         * @param handle Handle to find.
         * @return True when the handle exists.
         */
        [[nodiscard]] static bool contains_vertex(
            const std::vector<geometry::VertexHandle>& handles,
            geometry::VertexHandle handle);

        /**
         * @brief Checks whether an edge handle already exists in a vector.
         *
         * @param handles Handle vector.
         * @param handle Handle to find.
         * @return True when the handle exists.
         */
        [[nodiscard]] static bool contains_edge(
            const std::vector<geometry::EdgeHandle>& handles,
            geometry::EdgeHandle handle);

        BridgeEdgeMode mode_ = BridgeEdgeMode::VertexCycles;
        std::vector<geometry::VertexHandle> firstCycle_{};
        std::vector<geometry::VertexHandle> secondCycle_{};
        std::vector<geometry::EdgeHandle> firstEdges_{};
        std::vector<geometry::EdgeHandle> secondEdges_{};
        bool closed_ = true;
        bool flipSecondCycle_ = false;
        int twistOffset_ = 0;
    };

}
