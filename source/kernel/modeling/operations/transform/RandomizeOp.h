/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    /**
     * @brief Source used by RandomizeOp to choose affected vertices.
     */
    enum class RandomizeTarget {
        /**
         * @brief Randomize all active vertices when no explicit vertex list is set.
         */
        Vertices,

        /**
         * @brief Randomize only selected active vertices when no explicit vertex list is set.
         */
        SelectedVertices
    };

    /**
     * @brief Applies bounded pseudo-random offsets to editable mesh vertices.
     */
    class RandomizeOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty randomize operation.
         */
        RandomizeOp() = default;

        /**
         * @brief Creates an operation that affects collected vertices.
         *
         * @param strength Maximum absolute random offset on each axis.
         */
        explicit RandomizeOp(float strength);

        /**
         * @brief Creates an operation with an explicit vertex list.
         *
         * @param vertices Vertices to randomize.
         * @param strength Maximum absolute random offset on each axis.
         */
        RandomizeOp(std::vector<geometry::VertexHandle> vertices, float strength);

        /**
         * @brief Creates an operation configured to affect selected vertices.
         *
         * @param strength Maximum absolute random offset on each axis.
         * @return Randomize operation.
         */
        [[nodiscard]] static RandomizeOp selected(float strength);

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the maximum absolute random offset on each axis.
         *
         * @param strength Random offset strength.
         */
        void set_strength(float strength);

        /**
         * @brief Returns the random offset strength.
         *
         * @return Current strength.
         */
        [[nodiscard]] float strength() const;

        /**
         * @brief Sets the deterministic pseudo-random seed.
         *
         * @param seed Seed value.
         */
        void set_seed(std::uint32_t seed);

        /**
         * @brief Returns the deterministic pseudo-random seed.
         *
         * @return Current seed.
         */
        [[nodiscard]] std::uint32_t seed() const;

        /**
         * @brief Sets how target vertices are collected when no explicit list exists.
         *
         * @param target Target collection mode.
         */
        void set_target(RandomizeTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] RandomizeTarget target() const;

        /**
         * @brief Replaces the explicit vertex target list.
         *
         * @param vertices Vertices to randomize.
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

    private:
        /**
         * @brief Executes the randomize operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects valid vertices affected by this operation.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Target vertex list.
         */
        [[nodiscard]] std::vector<geometry::VertexHandle> collect_vertices(
            const geometry::LEM& mesh) const;

        std::vector<geometry::VertexHandle> vertices_{};
        float strength_ = 0.0f;
        std::uint32_t seed_ = 1337u;
        RandomizeTarget target_ = RandomizeTarget::Vertices;
    };

}