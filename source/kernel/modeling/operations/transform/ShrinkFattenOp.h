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
     * @brief Source used by ShrinkFattenOp to choose affected vertices.
     */
    enum class ShrinkFattenTarget {
        /**
         * @brief Move all active vertices when no explicit vertex list is set.
         */
        Vertices,

        /**
         * @brief Move only selected active vertices when no explicit vertex list is set.
         */
        SelectedVertices
    };

    /**
     * @brief Moves vertices along their averaged adjacent face normals.
     */
    class ShrinkFattenOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty shrink/fatten operation.
         */
        ShrinkFattenOp() = default;

        /**
         * @brief Creates an operation that affects collected vertices.
         *
         * @param distance Signed movement distance along averaged vertex normals.
         */
        explicit ShrinkFattenOp(float distance);

        /**
         * @brief Creates an operation with an explicit vertex list.
         *
         * @param vertices Vertices to move.
         * @param distance Signed movement distance along averaged vertex normals.
         */
        ShrinkFattenOp(std::vector<geometry::VertexHandle> vertices, float distance);

        /**
         * @brief Creates an operation configured to affect selected vertices.
         *
         * @param distance Signed movement distance along averaged vertex normals.
         * @return Shrink/fatten operation.
         */
        [[nodiscard]] static ShrinkFattenOp selected(float distance);

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the signed movement distance.
         *
         * @param distance Signed movement distance.
         */
        void set_distance(float distance);

        /**
         * @brief Returns the signed movement distance.
         *
         * @return Current movement distance.
         */
        [[nodiscard]] float distance() const;

        /**
         * @brief Sets how target vertices are collected when no explicit list exists.
         *
         * @param target Target collection mode.
         */
        void set_target(ShrinkFattenTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] ShrinkFattenTarget target() const;

        /**
         * @brief Replaces the explicit vertex target list.
         *
         * @param vertices Vertices to move.
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
         * @brief Executes the shrink/fatten operation.
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
        float distance_ = 0.0f;
        ShrinkFattenTarget target_ = ShrinkFattenTarget::Vertices;
    };

}