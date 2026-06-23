/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <glm/glm.hpp>

#include <string_view>
#include <vector>
#include <utility>

namespace locus::kernel::modeling {

    /**
     * @brief Input mode used by FillHoleOp.
     */
    enum class FillHoleMode {
        /**
         * @brief Fill a hole from an explicit ordered vertex cycle.
         */
        VertexCycle,

        /**
         * @brief Fill a hole from an explicit unordered boundary edge cycle.
         */
        EdgeCycle,

        /**
         * @brief Fill a hole from selected boundary edges.
         */
        SelectedBoundaryEdges
    };

    /**
     * @brief Creates one polygonal face from a boundary cycle.
     *
     * FillHoleOp does not try to repair arbitrary topology. It expects either an
     * ordered vertex cycle or a closed boundary edge cycle and delegates the final
     * face creation to LEMEditor.
     */
    class FillHoleOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty fill-hole operation.
         */
        FillHoleOp() = default;

        /**
         * @brief Creates an operation from an ordered vertex cycle.
         *
         * @param vertices Ordered boundary vertices.
         */
        explicit FillHoleOp(std::vector<geometry::VertexHandle> vertices);

        /**
         * @brief Creates an operation from an unordered boundary edge cycle.
         *
         * @param edges Boundary edges forming exactly one closed cycle.
         * @return Configured fill-hole operation.
         */
        [[nodiscard]] static FillHoleOp edges(std::vector<geometry::EdgeHandle> edges);

        /**
         * @brief Creates an operation that uses selected boundary edges.
         *
         * @return Configured fill-hole operation.
         */
        [[nodiscard]] static FillHoleOp selected_boundary_edges();

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the fill input mode.
         *
         * @param mode Input mode.
         */
        void set_mode(FillHoleMode mode);

        /**
         * @brief Returns the current fill input mode.
         *
         * @return Input mode.
         */
        [[nodiscard]] FillHoleMode mode() const;

        /**
         * @brief Replaces the ordered vertex cycle.
         *
         * @param vertices Ordered boundary vertices.
         */
        void set_vertices(std::vector<geometry::VertexHandle> vertices);

        /**
         * @brief Returns the explicit vertex cycle.
         *
         * @return Read-only vertex list.
         */
        [[nodiscard]] const std::vector<geometry::VertexHandle>& vertices() const;

        /**
         * @brief Clears the explicit vertex cycle.
         */
        void clear_vertices();

        /**
         * @brief Replaces the edge cycle.
         *
         * @param edges Boundary edges.
         */
        void set_edges(std::vector<geometry::EdgeHandle> edges);

        /**
         * @brief Returns the explicit edge cycle.
         *
         * @return Read-only edge list.
         */
        [[nodiscard]] const std::vector<geometry::EdgeHandle>& edges() const;

        /**
         * @brief Clears the explicit edge cycle.
         */
        void clear_edges();

        /**
         * @brief Sets whether the generated face winding should be reversed.
         *
         * @param flipWinding True to reverse the final vertex cycle.
         */
        void set_flip_winding(bool flipWinding);

        /**
         * @brief Checks whether the generated face winding will be reversed.
         *
         * @return True when winding reversal is enabled.
         */
        [[nodiscard]] bool flip_winding() const;

    private:
        /**
         * @brief Executes the fill-hole operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects the vertex cycle according to the current input mode.
         *
         * @param mesh Mesh used to validate and query topology.
         * @return Ordered vertex cycle, or an empty vector on failure.
         */
        [[nodiscard]] std::vector<geometry::VertexHandle> collect_vertex_cycle(
            const geometry::LEM& mesh) const;

        /**
         * @brief Collects selected boundary edges from the mesh.
         *
         * @param mesh Mesh to inspect.
         * @return Selected boundary edges.
         */
        [[nodiscard]] std::vector<geometry::EdgeHandle> collect_selected_boundary_edges(
            const geometry::LEM& mesh) const;

        /**
         * @brief Orders an unordered edge cycle into a vertex cycle.
         *
         * @param mesh Mesh used to read edge endpoints.
         * @param edges Edges that should form one closed cycle.
         * @return Ordered vertex cycle, or an empty vector when the edges are not a cycle.
         */
        [[nodiscard]] std::vector<geometry::VertexHandle> order_edge_cycle(
            const geometry::LEM& mesh,
            const std::vector<geometry::EdgeHandle>& edges) const;

        /**
         * @brief Checks whether the vertex cycle is valid.
         *
         * @param mesh Mesh used to validate handles and geometry.
         * @param vertices Candidate ordered vertex cycle.
         * @return True when the cycle can be filled.
         */
        [[nodiscard]] bool validate_vertex_cycle(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices) const;

        /**
         * @brief Checks whether filling the cycle would violate manifold constraints.
         *
         * @param context Operation context containing topology policy.
         * @param mesh Mesh used to inspect existing edges.
         * @param vertices Ordered vertex cycle.
         * @return True when the operation is allowed by the current topology policy.
         */
        [[nodiscard]] bool validate_topology_policy(
            const OperationContext& context,
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices) const;

        /**
         * @brief Computes a coarse polygon normal magnitude test.
         *
         * @param mesh Mesh used to read vertex positions.
         * @param vertices Ordered vertex cycle.
         * @return True when the polygon has usable area.
         */
        [[nodiscard]] bool has_non_degenerate_area(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices) const;

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

        FillHoleMode mode_ = FillHoleMode::VertexCycle;
        std::vector<geometry::VertexHandle> vertices_{};
        std::vector<geometry::EdgeHandle> edges_{};
        bool flipWinding_ = false;
    };

}