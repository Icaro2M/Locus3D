/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEM.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Low-level editor for LEM topology mutations.
     *
     * TopologyEditor owns editing operations that create, remove, relink, or
     * rebuild topological mesh structure. It records all accepted topology
     * changes into the shared LEMDiff owned by the parent LEMEditor facade.
     */
    class TopologyEditor {
    public:
        /**
         * @brief Creates a topology editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives topology mutations.
         * @param diff Diff that receives change events.
         */
        TopologyEditor(LEM& mesh, LEMDiff& diff);

        /**
         * @brief Returns the edited mesh.
         *
         * @return Mutable mesh reference.
         */
        [[nodiscard]] LEM& mesh();

        /**
         * @brief Returns the edited mesh.
         *
         * @return Read-only mesh reference.
         */
        [[nodiscard]] const LEM& mesh() const;

        /**
         * @brief Adds a loose vertex and records the created element.
         *
         * @param position Vertex position in object space.
         * @return Handle referencing the created vertex.
         */
        VertexHandle add_vertex(const glm::vec3& position);

        /**
         * @brief Finds or creates a non-directional edge and records insertions.
         *
         * @param vertexA First endpoint vertex.
         * @param vertexB Second endpoint vertex.
         * @return Handle referencing the existing or created edge.
         */
        EdgeHandle find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB);

        /**
         * @brief Adds a polygonal face and records all created topology elements.
         *
         * @param vertices Ordered face vertices.
         * @return Handle referencing the created face, or an invalid handle on failure.
         */
        FaceHandle add_face(const std::vector<VertexHandle>& vertices);

        /**
         * @brief Removes a face and its boundary loops from radial cycles.
         *
         * Edges and vertices are kept alive. Loose cleanup can be performed
         * explicitly through remove_edge_if_loose() and remove_vertex_if_loose().
         *
         * @param face Face to remove.
         * @return True when the face existed and was removed.
         */
        bool remove_face(FaceHandle face);

        /**
         * @brief Removes an edge only when no active loop references it.
         *
         * @param edge Edge to remove.
         * @return True when the edge existed and was loose.
         */
        bool remove_edge_if_loose(EdgeHandle edge);

        /**
         * @brief Removes a vertex only when no active edge or loop references it.
         *
         * @param vertex Vertex to remove.
         * @return True when the vertex existed and was loose.
         */
        bool remove_vertex_if_loose(VertexHandle vertex);

        /**
         * @brief Reverses the winding of a face while preserving radial links.
         *
         * @param face Face whose boundary orientation will be reversed.
         * @return True when the face existed and could be flipped.
         */
        bool flip_face(FaceHandle face);

        /**
         * @brief Reverses the winding of every active face.
         *
         * @return Number of faces successfully flipped.
         */
        std::size_t flip_all_faces();

        /**
         * @brief Recomputes all active face normals and records normal changes.
         */
        void rebuild_face_normals();

        /**
         * @brief Clears the whole mesh and records a mesh clear event.
         */
        void clear();

    private:
        /**
         * @brief Removes a loop from its edge radial cycle.
         *
         * @param loopHandle Loop to detach.
         * @return True when the loop could be detached.
         */
        bool remove_loop_from_radial(LoopHandle loopHandle);

        /**
         * @brief Inserts a loop into an edge radial cycle.
         *
         * @param loopHandle Loop to insert.
         * @param edgeHandle Edge that will own the radial cycle.
         * @return True when insertion succeeded.
         */
        bool insert_loop_into_radial(LoopHandle loopHandle, EdgeHandle edgeHandle);

        /**
         * @brief Refreshes the entry loop stored by an edge.
         *
         * @param edgeHandle Edge whose entry loop may need updating.
         */
        void refresh_edge_entry_loop(EdgeHandle edgeHandle);

        /**
         * @brief Refreshes the incident edge stored by a vertex.
         *
         * @param vertexHandle Vertex whose incident edge may need updating.
         */
        void refresh_vertex_incident_edge(VertexHandle vertexHandle);

        LEM& mesh_;
        LEMDiff& diff_;
    };

}