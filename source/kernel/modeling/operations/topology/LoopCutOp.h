/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace locus::kernel::geometry {

    class LEM;
    class LEMEditor;

}

namespace locus::kernel::modeling {

    /**
     * @brief Target collection mode used by LoopCutOp.
     */
    enum class LoopCutTarget {
        /**
         * @brief Cut the explicit edge list when provided, otherwise all active edges.
         */
        Edges,

        /**
         * @brief Cut the explicit edge list when provided, otherwise selected edges.
         */
        SelectedEdges
    };

    /**
     * @brief Inserts edge cuts and connects them across affected faces.
     *
     * LoopCutOp splits each target edge at one or more parametric positions and
     * then attempts to connect the newly created vertices inside faces that
     * received matching cut vertices. It is intentionally independent from editor
     * interaction tools such as picking, snapping, or preview state.
     */
    class LoopCutOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty loop cut operation.
         */
        LoopCutOp() = default;

        /**
         * @brief Creates an operation that cuts one edge.
         *
         * @param edge Edge to cut.
         */
        explicit LoopCutOp(geometry::EdgeHandle edge);

        /**
         * @brief Creates an operation that cuts explicit edges.
         *
         * @param edges Edges to cut.
         */
        explicit LoopCutOp(std::vector<geometry::EdgeHandle> edges);

        /**
         * @brief Creates an operation that cuts selected edges.
         *
         * @return Configured loop cut operation.
         */
        [[nodiscard]] static LoopCutOp selected_edges();

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
        void set_target(LoopCutTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] LoopCutTarget target() const;

        /**
         * @brief Replaces the explicit edge list.
         *
         * @param edges Edges to cut.
         */
        void set_edges(std::vector<geometry::EdgeHandle> edges);

        /**
         * @brief Returns the explicit edge list.
         *
         * @return Read-only edge list.
         */
        [[nodiscard]] const std::vector<geometry::EdgeHandle>& edges() const;

        /**
         * @brief Clears the explicit edge list.
         */
        void clear_edges();

        /**
         * @brief Sets the number of cuts inserted on each target edge.
         *
         * @param cuts Number of cuts. Values below one are clamped to one.
         */
        void set_cuts(std::size_t cuts);

        /**
         * @brief Returns the number of cuts inserted on each target edge.
         *
         * @return Cut count.
         */
        [[nodiscard]] std::size_t cuts() const;

        /**
         * @brief Sets the parametric cut position for single-cut mode.
         *
         * @param factor Parametric factor from edge vertexA to vertexB.
         */
        void set_factor(float factor);

        /**
         * @brief Returns the single-cut parametric factor.
         *
         * @return Cut factor.
         */
        [[nodiscard]] float factor() const;

        /**
         * @brief Sets whether multiple cuts are spaced uniformly.
         *
         * @param useEvenSpacing True to use uniform spacing.
         */
        void set_even_spacing(bool useEvenSpacing);

        /**
         * @brief Checks whether multiple cuts are spaced uniformly.
         *
         * @return True when even spacing is enabled.
         */
        [[nodiscard]] bool even_spacing() const;

    private:
        /**
         * @brief Stores newly created vertices for one original edge.
         */
        struct EdgeCut {
            geometry::EdgeHandle originalEdge{};
            std::vector<geometry::VertexHandle> vertices{};
        };

        /**
         * @brief Executes the loop cut operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects valid edge targets.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Edge targets.
         */
        [[nodiscard]] std::vector<geometry::EdgeHandle> collect_edges(
            const geometry::LEM& mesh) const;

        /**
         * @brief Splits one edge according to the current cut settings.
         *
         * @param editor Editor used to mutate topology.
         * @param edge Edge to split.
         * @return Created cut vertices.
         */
        [[nodiscard]] EdgeCut cut_edge(
            geometry::LEMEditor& editor,
            geometry::EdgeHandle edge) const;

        /**
         * @brief Connects newly created cut vertices across active faces.
         *
         * @param editor Editor used to mutate topology.
         * @param edgeCuts Newly created vertices grouped by source edge.
         * @return Number of accepted face splits.
         */
        [[nodiscard]] std::size_t connect_edge_cuts(
            geometry::LEMEditor& editor,
            const std::vector<EdgeCut>& edgeCuts) const;

        /**
         * @brief Tries to connect two cut vertices by splitting a shared face.
         *
         * @param editor Editor used to mutate topology.
         * @param vertexA First cut vertex.
         * @param vertexB Second cut vertex.
         * @return True when a split was accepted.
         */
        [[nodiscard]] bool connect_cut_pair(
            geometry::LEMEditor& editor,
            geometry::VertexHandle vertexA,
            geometry::VertexHandle vertexB) const;

        /**
         * @brief Checks whether the second cut sequence should be paired reversed.
         *
         * @param mesh Mesh used to query vertex positions.
         * @param first First edge cut sequence.
         * @param second Second edge cut sequence.
         * @return True when reversed pairing is spatially shorter.
         */
        [[nodiscard]] bool should_reverse_cut_order(
            const geometry::LEM& mesh,
            const EdgeCut& first,
            const EdgeCut& second) const;

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

        LoopCutTarget target_ = LoopCutTarget::SelectedEdges;
        std::vector<geometry::EdgeHandle> edges_{};
        std::size_t cuts_ = 1;
        float factor_ = 0.5f;
        bool evenSpacing_ = true;
    };

}
