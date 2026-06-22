/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    /**
     * @brief Strategy used by MergeVerticesOp to choose how vertices are merged.
     */
    enum class MergeVerticesMode {
        /**
         * @brief Merge one source vertex into one target vertex.
         */
        Pair,

        /**
         * @brief Merge one source vertex into one target vertex and move the result to a custom position.
         */
        PairAtPosition,

        /**
         * @brief Merge every compatible vertex pair in the mesh using a distance threshold.
         */
        Distance,

        /**
         * @brief Merge only vertices from an explicit vertex list using a distance threshold.
         */
        VertexSetDistance
    };

    /**
     * @brief Merges editable mesh vertices through the LEM editor facade.
     */
    class MergeVerticesOp final : public IOperation {
    public:
        MergeVerticesOp() = default;

        /**
         * @brief Creates a pair merge operation.
         *
         * @param sourceVertex Vertex that will be merged into the target.
         * @param targetVertex Vertex that receives the merged connectivity.
         */
        MergeVerticesOp(
            geometry::VertexHandle sourceVertex,
            geometry::VertexHandle targetVertex);

        /**
         * @brief Creates a pair merge operation with an explicit final position.
         *
         * @param sourceVertex Vertex that will be merged into the target.
         * @param targetVertex Vertex that receives the merged connectivity.
         * @param position Final position assigned to the merged vertex.
         */
        MergeVerticesOp(
            geometry::VertexHandle sourceVertex,
            geometry::VertexHandle targetVertex,
            const glm::vec3& position);

        /**
         * @brief Creates a whole-mesh distance merge operation.
         *
         * @param distance Maximum distance between vertices that may be merged.
         */
        explicit MergeVerticesOp(float distance);

        /**
         * @brief Creates a vertex-set distance merge operation.
         *
         * @param vertices Candidate vertices.
         * @param distance Maximum distance between vertices that may be merged.
         */
        MergeVerticesOp(
            std::vector<geometry::VertexHandle> vertices,
            float distance);

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the operation mode.
         *
         * @param mode Merge mode.
         */
        void set_mode(MergeVerticesMode mode);

        /**
         * @brief Returns the current operation mode.
         *
         * @return Merge mode.
         */
        [[nodiscard]] MergeVerticesMode mode() const;

        /**
         * @brief Sets the source and target vertices used by pair merge modes.
         *
         * @param sourceVertex Vertex that will be merged into the target.
         * @param targetVertex Vertex that receives the merged connectivity.
         */
        void set_pair(
            geometry::VertexHandle sourceVertex,
            geometry::VertexHandle targetVertex);

        /**
         * @brief Sets the source and target vertices and switches to PairAtPosition mode.
         *
         * @param sourceVertex Vertex that will be merged into the target.
         * @param targetVertex Vertex that receives the merged connectivity.
         * @param position Final position assigned to the merged vertex.
         */
        void set_pair_at_position(
            geometry::VertexHandle sourceVertex,
            geometry::VertexHandle targetVertex,
            const glm::vec3& position);

        /**
         * @brief Returns the source vertex.
         *
         * @return Source vertex handle.
         */
        [[nodiscard]] geometry::VertexHandle source_vertex() const;

        /**
         * @brief Returns the target vertex.
         *
         * @return Target vertex handle.
         */
        [[nodiscard]] geometry::VertexHandle target_vertex() const;

        /**
         * @brief Sets the final merged position used by PairAtPosition mode.
         *
         * @param position Final merged position.
         */
        void set_position(const glm::vec3& position);

        /**
         * @brief Returns the final merged position used by PairAtPosition mode.
         *
         * @return Final merged position.
         */
        [[nodiscard]] const glm::vec3& position() const;

        /**
         * @brief Sets the distance threshold used by distance merge modes.
         *
         * @param distance Maximum merge distance.
         */
        void set_distance(float distance);

        /**
         * @brief Returns the distance threshold used by distance merge modes.
         *
         * @return Maximum merge distance.
         */
        [[nodiscard]] float distance() const;

        /**
         * @brief Replaces the explicit vertex candidate list.
         *
         * @param vertices Candidate vertices.
         */
        void set_vertices(std::vector<geometry::VertexHandle> vertices);

        /**
         * @brief Returns the explicit vertex candidate list.
         *
         * @return Read-only vertex list.
         */
        [[nodiscard]] const std::vector<geometry::VertexHandle>& vertices() const;

        /**
         * @brief Clears the explicit vertex candidate list.
         */
        void clear_vertices();

    private:
        /**
         * @brief Executes the merge vertices operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Executes a pair merge operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_pair(OperationContext& context) const;

        /**
         * @brief Executes a whole-mesh distance merge operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_distance(OperationContext& context) const;

        /**
         * @brief Executes an explicit vertex-set distance merge operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_vertex_set_distance(OperationContext& context) const;

        /**
         * @brief Collects valid distinct vertices from the explicit vertex list.
         *
         * @param mesh Mesh used to validate handles.
         * @return Valid distinct vertex handles.
         */
        [[nodiscard]] std::vector<geometry::VertexHandle> collect_valid_vertices(
            const geometry::LEM& mesh) const;

        MergeVerticesMode mode_ = MergeVerticesMode::Pair;
        geometry::VertexHandle sourceVertex_{};
        geometry::VertexHandle targetVertex_{};
        glm::vec3 position_{ 0.0f };
        float distance_ = 0.0f;
        std::vector<geometry::VertexHandle> vertices_{};
    };

}