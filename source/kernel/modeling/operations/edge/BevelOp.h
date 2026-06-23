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
     * @brief Target collection mode used by BevelOp.
     */
    enum class BevelTarget {
        /**
         * @brief Bevel explicit edges when provided, otherwise all active edges.
         */
        Edges,

        /**
         * @brief Bevel explicit edges when provided, otherwise selected edges.
         */
        SelectedEdges,

        /**
         * @brief Bevel explicit vertices when provided, otherwise selected vertices.
         */
        SelectedVertices
    };

    /**
     * @brief Creates single-segment chamfers on editable mesh faces.
     *
     * BevelOp is a conservative topology operation that bevels face corners affected
     * by target edges or vertices. It rebuilds affected faces through the LEM editor
     * facade and creates one chamfer face per affected corner.
     */
    class BevelOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty bevel operation.
         */
        BevelOp() = default;

        /**
         * @brief Creates an operation that bevels one edge.
         *
         * @param edge Edge whose endpoints should be beveled in adjacent faces.
         * @param width Chamfer width in object space.
         */
        BevelOp(geometry::EdgeHandle edge, float width);

        /**
         * @brief Creates an operation that bevels explicit edges.
         *
         * @param edges Edges whose endpoints should be beveled in adjacent faces.
         * @param width Chamfer width in object space.
         */
        BevelOp(std::vector<geometry::EdgeHandle> edges, float width);

        /**
         * @brief Creates an operation that bevels explicit vertices.
         *
         * @param vertices Vertices to bevel in adjacent faces.
         * @param width Chamfer width in object space.
         */
        BevelOp(std::vector<geometry::VertexHandle> vertices, float width);

        /**
         * @brief Creates an operation configured to bevel selected edges.
         *
         * @param width Chamfer width in object space.
         * @return Configured bevel operation.
         */
        [[nodiscard]] static BevelOp selected_edges(float width);

        /**
         * @brief Creates an operation configured to bevel selected vertices.
         *
         * @param width Chamfer width in object space.
         * @return Configured bevel operation.
         */
        [[nodiscard]] static BevelOp selected_vertices(float width);

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
        void set_target(BevelTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] BevelTarget target() const;

        /**
         * @brief Sets the bevel width.
         *
         * @param width Chamfer width in object space.
         */
        void set_width(float width);

        /**
         * @brief Returns the bevel width.
         *
         * @return Bevel width.
         */
        [[nodiscard]] float width() const;

        /**
         * @brief Replaces the explicit edge target list.
         *
         * @param edges Edges to bevel.
         */
        void set_edges(std::vector<geometry::EdgeHandle> edges);

        /**
         * @brief Returns the explicit edge target list.
         *
         * @return Read-only edge target list.
         */
        [[nodiscard]] const std::vector<geometry::EdgeHandle>& edges() const;

        /**
         * @brief Clears the explicit edge target list.
         */
        void clear_edges();

        /**
         * @brief Replaces the explicit vertex target list.
         *
         * @param vertices Vertices to bevel.
         */
        void set_vertices(std::vector<geometry::VertexHandle> vertices);

        /**
         * @brief Returns the explicit vertex target list.
         *
         * @return Read-only vertex target list.
         */
        [[nodiscard]] const std::vector<geometry::VertexHandle>& vertices() const;

        /**
         * @brief Clears the explicit vertex target list.
         */
        void clear_vertices();

    private:
        /**
         * @brief Stores the two replacement vertices created for one beveled corner.
         */
        struct CornerCut {
            geometry::VertexHandle previousSide{};
            geometry::VertexHandle nextSide{};
        };

        /**
         * @brief Executes the bevel operation.
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
         * @brief Collects vertex targets.
         *
         * @param mesh Mesh used to validate and query handles.
         * @param targetEdges Already collected edge targets.
         * @return Valid vertex targets.
         */
        [[nodiscard]] std::vector<geometry::VertexHandle> collect_vertices(
            const geometry::LEM& mesh,
            const std::vector<geometry::EdgeHandle>& targetEdges) const;

        /**
         * @brief Collects faces affected by the vertex targets.
         *
         * @param mesh Mesh used to query topology.
         * @param targetVertices Vertices selected for beveling.
         * @return Affected face handles.
         */
        [[nodiscard]] std::vector<geometry::FaceHandle> collect_faces(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& targetVertices) const;

        /**
         * @brief Bevels one face.
         *
         * @param mesh Mesh being edited.
         * @param editor Editor facade used to mutate the mesh.
         * @param face Face to rebuild.
         * @param targetVertices Vertices selected for beveling.
         * @return True when the face was rebuilt.
         */
        [[nodiscard]] bool bevel_face(
            geometry::LEM& mesh,
            geometry::LEMEditor& editor,
            geometry::FaceHandle face,
            const std::vector<geometry::VertexHandle>& targetVertices) const;

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

        BevelTarget target_ = BevelTarget::SelectedEdges;
        std::vector<geometry::EdgeHandle> edges_{};
        std::vector<geometry::VertexHandle> vertices_{};
        float width_ = 0.1f;
    };

}