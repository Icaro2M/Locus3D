/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <algorithm>
#include <cstddef>
#include <glm/vec3.hpp>
#include <string_view>
#include <vector>

namespace locus::kernel::geometry {

    class LEM;

}

namespace locus::kernel::modeling {

    /**
     * @brief Target collection mode used by EdgeSlideOp.
     */
    enum class EdgeSlideTarget {
        /**
         * @brief Slide explicit edges when provided, otherwise all active edges.
         */
        Edges,

        /**
         * @brief Slide explicit edges when provided, otherwise selected edges.
         */
        SelectedEdges,

        /**
         * @brief Slide explicit vertices when provided, otherwise selected vertices.
         */
        SelectedVertices
    };

    /**
     * @brief Moves vertices along adjacent edge directions without changing topology.
     *
     * EdgeSlideOp is a geometric modeling operation. It does not create, delete, or
     * relink mesh elements. Tool interaction layers are expected to resolve picking,
     * snapping, preview state, and edge-loop expansion before configuring this op.
     */
    class EdgeSlideOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty edge slide operation.
         */
        EdgeSlideOp() = default;

        /**
         * @brief Creates an operation that slides one edge.
         *
         * @param edge Edge whose endpoints will slide.
         * @param distance Signed slide distance.
         */
        EdgeSlideOp(geometry::EdgeHandle edge, float distance);

        /**
         * @brief Creates an operation that slides explicit edges.
         *
         * @param edges Edges whose endpoints will slide.
         * @param distance Signed slide distance.
         */
        EdgeSlideOp(std::vector<geometry::EdgeHandle> edges, float distance);

        /**
         * @brief Creates an operation that slides explicit vertices.
         *
         * @param vertices Vertices to slide.
         * @param distance Signed slide distance.
         */
        EdgeSlideOp(std::vector<geometry::VertexHandle> vertices, float distance);

        /**
         * @brief Creates an operation configured to slide selected edges.
         *
         * @param distance Signed slide distance.
         * @return Configured edge slide operation.
         */
        [[nodiscard]] static EdgeSlideOp selected_edges(float distance);

        /**
         * @brief Creates an operation configured to slide selected vertices.
         *
         * @param distance Signed slide distance.
         * @return Configured edge slide operation.
         */
        [[nodiscard]] static EdgeSlideOp selected_vertices(float distance);

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the target collection mode.
         *
         * @param target Target mode.
         */
        void set_target(EdgeSlideTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] EdgeSlideTarget target() const;

        /**
         * @brief Sets the signed slide distance.
         *
         * @param distance Signed object-space distance.
         */
        void set_distance(float distance);

        /**
         * @brief Returns the signed slide distance.
         *
         * @return Slide distance.
         */
        [[nodiscard]] float distance() const;

        /**
         * @brief Replaces the explicit edge target list.
         *
         * @param edges Edges whose endpoints will slide.
         */
        void set_edges(std::vector<geometry::EdgeHandle> edges);

        /**
         * @brief Returns the explicit edge target list.
         *
         * @return Read-only edge list.
         */
        [[nodiscard]] const std::vector<geometry::EdgeHandle>& edges() const;

        /**
         * @brief Clears the explicit edge target list.
         */
        void clear_edges();

        /**
         * @brief Replaces the explicit vertex target list.
         *
         * @param vertices Vertices to slide.
         */
        void set_vertices(std::vector<geometry::VertexHandle> vertices);

        /**
         * @brief Returns the explicit vertex target list.
         *
         * @return Read-only vertex list.
         */
        [[nodiscard]] const std::vector<geometry::VertexHandle>& vertices() const;

        /**
         * @brief Clears the explicit vertex target list.
         */
        void clear_vertices();

        /**
         * @brief Sets whether selected target edges should be ignored as slide rails.
         *
         * @param excludeTargetEdges True to avoid sliding along the selected edge itself.
         */
        void set_exclude_target_edges_from_rails(bool excludeTargetEdges);

        /**
         * @brief Checks whether selected target edges are ignored as slide rails.
         *
         * @return True when target edges are excluded from rail calculation.
         */
        [[nodiscard]] bool exclude_target_edges_from_rails() const;

    private:
        /**
         * @brief Executes the edge slide operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects edge targets.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Valid edge targets.
         */
        [[nodiscard]] std::vector<geometry::EdgeHandle> collect_edges(
            const geometry::LEM& mesh) const;

        /**
         * @brief Collects vertex targets from operation configuration.
         *
         * @param mesh Mesh used to validate and query handles.
         * @param targetEdges Already collected edge targets.
         * @return Valid vertex targets.
         */
        [[nodiscard]] std::vector<geometry::VertexHandle> collect_vertices(
            const geometry::LEM& mesh,
            const std::vector<geometry::EdgeHandle>& targetEdges) const;

        /**
         * @brief Computes a normalized slide direction for one vertex.
         *
         * @param mesh Mesh used for topology and position queries.
         * @param vertex Vertex to slide.
         * @param targetEdges Edge targets that may be excluded from rail calculation.
         * @return Direction vector. Zero means no valid slide rail was found.
         */
        [[nodiscard]] glm::vec3 slide_direction(
            const geometry::LEM& mesh,
            geometry::VertexHandle vertex,
            const std::vector<geometry::EdgeHandle>& targetEdges) const;

        /**
         * @brief Checks whether a handle already exists in a vector.
         *
         * @param handles Handle vector.
         * @param handle Handle to find.
         * @return True when the handle exists.
         */
        template <typename Handle>
        [[nodiscard]] static bool contains(
            const std::vector<Handle>& handles,
            Handle handle)
        {
            return std::find(handles.begin(), handles.end(), handle) != handles.end();
        }

        EdgeSlideTarget target_ = EdgeSlideTarget::SelectedEdges;
        std::vector<geometry::EdgeHandle> edges_{};
        std::vector<geometry::VertexHandle> vertices_{};
        float distance_ = 0.0f;
        bool excludeTargetEdgesFromRails_ = true;
    };

}