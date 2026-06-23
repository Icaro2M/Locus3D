/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    /**
     * @brief Source used by CreaseOp to choose affected edges.
     */
    enum class CreaseTarget {
        /**
         * @brief Apply crease to all active edges when no explicit edge list is set.
         */
        Edges,

        /**
         * @brief Apply crease only to selected active edges when no explicit edge list is set.
         */
        SelectedEdges
    };

    /**
     * @brief Sets crease strength on editable mesh edges.
     */
    class CreaseOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty crease operation.
         */
        CreaseOp() = default;

        /**
         * @brief Creates an operation that affects collected edges.
         *
         * @param crease Crease strength clamped by the editor to the range [0, 1].
         */
        explicit CreaseOp(float crease);

        /**
         * @brief Creates an operation with an explicit edge list.
         *
         * @param edges Edges to modify.
         * @param crease Crease strength clamped by the editor to the range [0, 1].
         */
        CreaseOp(std::vector<geometry::EdgeHandle> edges, float crease);

        /**
         * @brief Creates an operation configured to affect selected edges.
         *
         * @param crease Crease strength clamped by the editor to the range [0, 1].
         * @return Crease operation.
         */
        [[nodiscard]] static CreaseOp selected(float crease);

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the crease strength.
         *
         * @param crease Crease strength.
         */
        void set_crease(float crease);

        /**
         * @brief Returns the crease strength.
         *
         * @return Current crease strength.
         */
        [[nodiscard]] float crease() const;

        /**
         * @brief Sets how target edges are collected when no explicit list exists.
         *
         * @param target Target collection mode.
         */
        void set_target(CreaseTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] CreaseTarget target() const;

        /**
         * @brief Replaces the explicit edge target list.
         *
         * @param edges Edges to modify.
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

    private:
        /**
         * @brief Executes the crease operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects valid edges affected by this operation.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Target edge list.
         */
        [[nodiscard]] std::vector<geometry::EdgeHandle> collect_edges(
            const geometry::LEM& mesh) const;

        std::vector<geometry::EdgeHandle> edges_{};
        float crease_ = 1.0f;
        CreaseTarget target_ = CreaseTarget::Edges;
    };

}