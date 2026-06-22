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
     * @brief Target collection mode used by SubdivideOp.
     */
    enum class SubdivideTarget {
        /**
         * @brief Subdivide all active edges, or the explicit edge list when provided.
         */
        Edges,

        /**
         * @brief Subdivide selected active edges, or the explicit edge list when provided.
         */
        SelectedEdges,

        /**
         * @brief Subdivide all active faces, or the explicit face list when provided.
         */
        Faces,

        /**
         * @brief Subdivide selected active faces, or the explicit face list when provided.
         */
        SelectedFaces
    };

    /**
     * @brief Subdivides editable mesh topology using the LEM editor facade.
     */
    class SubdivideOp final : public IOperation {
    public:
        /**
         * @brief Creates an edge subdivision operation.
         */
        SubdivideOp() = default;

        /**
         * @brief Creates an operation that subdivides one edge.
         *
         * @param edge Edge to subdivide.
         */
        explicit SubdivideOp(geometry::EdgeHandle edge);

        /**
         * @brief Creates an operation that subdivides explicit edges.
         *
         * @param edges Edges to subdivide.
         */
        explicit SubdivideOp(std::vector<geometry::EdgeHandle> edges);

        /**
         * @brief Creates a face subdivision operation.
         *
         * @param faces Faces to subdivide.
         * @return Subdivide operation configured for explicit faces.
         */
        [[nodiscard]] static SubdivideOp faces(std::vector<geometry::FaceHandle> faces);

        /**
         * @brief Creates a face subdivision operation.
         *
         * @param face Face to subdivide.
         * @return Subdivide operation configured for one face.
         */
        [[nodiscard]] static SubdivideOp face(geometry::FaceHandle face);

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
        void set_target(SubdivideTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] SubdivideTarget target() const;

        /**
         * @brief Replaces the explicit edge list.
         *
         * @param edges Edges to subdivide.
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
         * @brief Replaces the explicit face list.
         *
         * @param faces Faces to subdivide.
         */
        void set_faces(std::vector<geometry::FaceHandle> faces);

        /**
         * @brief Returns the explicit face list.
         *
         * @return Read-only face list.
         */
        [[nodiscard]] const std::vector<geometry::FaceHandle>& faces() const;

        /**
         * @brief Clears the explicit face list.
         */
        void clear_faces();

    private:
        /**
         * @brief Executes the subdivision operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Executes edge subdivision.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_edges(OperationContext& context) const;

        /**
         * @brief Executes face subdivision.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_faces(OperationContext& context) const;

        /**
         * @brief Collects valid edge targets.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Edge targets.
         */
        [[nodiscard]] std::vector<geometry::EdgeHandle> collect_edges(
            const geometry::LEM& mesh) const;

        /**
         * @brief Collects valid face targets.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Face targets.
         */
        [[nodiscard]] std::vector<geometry::FaceHandle> collect_faces(
            const geometry::LEM& mesh) const;

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

        SubdivideTarget target_ = SubdivideTarget::Edges;
        std::vector<geometry::EdgeHandle> edges_{};
        std::vector<geometry::FaceHandle> faces_{};
    };

}