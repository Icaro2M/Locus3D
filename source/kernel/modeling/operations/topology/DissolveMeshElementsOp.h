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
     * @brief Mesh component granularity dissolved by DissolveMeshElementsOp.
     */
    enum class DissolveMeshElementMode {
        Vertices,
        Edges,
        Faces
    };

    /**
     * @brief Dissolves selected editable mesh components when topology can be preserved.
     *
     * Edge dissolve merges two manifold adjacent faces into one polygonal face.
     * Vertex dissolve removes loose chain vertices with valence up to two.
     * Face dissolve is intentionally rejected until a surface-preserving region
     * policy exists; it must not silently behave like Delete Face.
     */
    class DissolveMeshElementsOp final : public IOperation {
    public:
        [[nodiscard]] static DissolveMeshElementsOp vertices(
            std::vector<geometry::VertexHandle> vertices);

        [[nodiscard]] static DissolveMeshElementsOp edges(
            std::vector<geometry::EdgeHandle> edges);

        [[nodiscard]] static DissolveMeshElementsOp faces(
            std::vector<geometry::FaceHandle> faces);

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] DissolveMeshElementMode mode() const;

        [[nodiscard]] const std::vector<geometry::VertexHandle>&
            vertices() const;

        [[nodiscard]] const std::vector<geometry::EdgeHandle>&
            edges() const;

        [[nodiscard]] const std::vector<geometry::FaceHandle>&
            faces() const;

    private:
        explicit DissolveMeshElementsOp(DissolveMeshElementMode mode);

        [[nodiscard]] OperationResult execute_impl(
            OperationContext& context) override;

        [[nodiscard]] OperationResult execute_vertices(
            OperationContext& context) const;

        [[nodiscard]] OperationResult execute_edges(
            OperationContext& context) const;

        [[nodiscard]] OperationResult execute_faces(
            OperationContext& context) const;

        DissolveMeshElementMode mode_ = DissolveMeshElementMode::Edges;
        std::vector<geometry::VertexHandle> vertices_{};
        std::vector<geometry::EdgeHandle> edges_{};
        std::vector<geometry::FaceHandle> faces_{};
    };

} // namespace locus::kernel::modeling
