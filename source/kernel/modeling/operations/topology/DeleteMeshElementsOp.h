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
     * @brief Mesh component granularity deleted by DeleteMeshElementsOp.
     */
    enum class DeleteMeshElementMode {
        Vertices,
        Edges,
        Faces
    };

    /**
     * @brief Deletes editable mesh components with explicit topological policy.
     *
     * Delete Face removes selected faces and loops while keeping boundary edges
     * and vertices alive. Delete Edge removes incident faces first, then removes
     * the selected edges. Delete Vertex removes incident faces, incident edges,
     * and finally the selected vertices.
     */
    class DeleteMeshElementsOp final : public IOperation {
    public:
        [[nodiscard]] static DeleteMeshElementsOp vertices(
            std::vector<geometry::VertexHandle> vertices);

        [[nodiscard]] static DeleteMeshElementsOp edges(
            std::vector<geometry::EdgeHandle> edges);

        [[nodiscard]] static DeleteMeshElementsOp faces(
            std::vector<geometry::FaceHandle> faces);

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] DeleteMeshElementMode mode() const;

        [[nodiscard]] const std::vector<geometry::VertexHandle>&
            vertices() const;

        [[nodiscard]] const std::vector<geometry::EdgeHandle>&
            edges() const;

        [[nodiscard]] const std::vector<geometry::FaceHandle>&
            faces() const;

    private:
        explicit DeleteMeshElementsOp(DeleteMeshElementMode mode);

        [[nodiscard]] OperationResult execute_impl(
            OperationContext& context) override;

        [[nodiscard]] OperationResult execute_vertices(
            OperationContext& context) const;

        [[nodiscard]] OperationResult execute_edges(
            OperationContext& context) const;

        [[nodiscard]] OperationResult execute_faces(
            OperationContext& context) const;

        DeleteMeshElementMode mode_ = DeleteMeshElementMode::Faces;
        std::vector<geometry::VertexHandle> vertices_{};
        std::vector<geometry::EdgeHandle> edges_{};
        std::vector<geometry::FaceHandle> faces_{};
    };

} // namespace locus::kernel::modeling
