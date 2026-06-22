/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace locus::kernel::geometry {

    class LEM;

    /**
     * @brief Facade for topological mutations on a Locus Editable Mesh.
     *
     * TopologyEditor exposes a stable public access point for operations that
     * create, remove, split, collapse, dissolve, flip, or relink editable mesh
     * topology. The implementation is delegated to smaller internal topology
     * modules under editing/topology.
     */
    class TopologyEditor {
    public:
        /**
         * @brief Creates a topology editor bound to a mesh and diff recorder.
         *
         * @param mesh Mesh that receives topology mutations.
         * @param diff Diff that receives accepted change events.
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
         * @brief Removes a face and its boundary loops.
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
         * @brief Marks a face as deleted without deleting its boundary loops.
         *
         * @param face Face to kill.
         * @return True when the face existed and was killed.
         */
        bool kill_face_only(FaceHandle face);

        /**
         * @brief Marks an edge as deleted without deleting its endpoint vertices.
         *
         * @param edge Edge to kill.
         * @return True when the edge existed and was killed.
         */
        bool kill_edge_only(EdgeHandle edge);

        /**
         * @brief Removes a loop from face and radial cycles and marks it as deleted.
         *
         * @param loop Loop to kill.
         * @return True when the loop existed and was killed.
         */
        bool kill_loop(LoopHandle loop);

        /**
         * @brief Splits an edge at its midpoint.
         *
         * @param edge Edge to split.
         * @return Created vertex, or an empty optional on failure.
         */
        std::optional<VertexHandle> split_edge(EdgeHandle edge);

        /**
         * @brief Splits an edge at a parametric point.
         *
         * @param edge Edge to split.
         * @param t Parametric position from vertexA to vertexB, clamped to [0, 1].
         * @return Created vertex, or an empty optional on failure.
         */
        std::optional<VertexHandle> split_edge_at_param(EdgeHandle edge, float t);

        /**
         * @brief Splits a face by connecting two non-adjacent boundary vertices.
         *
         * @param face Face to split.
         * @param vertexA First boundary vertex.
         * @param vertexB Second boundary vertex.
         * @return Created diagonal edge, or an empty optional on failure.
         */
        std::optional<EdgeHandle> split_face(
            FaceHandle face,
            VertexHandle vertexA,
            VertexHandle vertexB);

        /**
         * @brief Collapses an edge by merging its endpoints into one vertex.
         *
         * @param edge Edge to collapse.
         * @return True when the collapse succeeded.
         */
        bool collapse_edge(EdgeHandle edge);

        /**
         * @brief Dissolves an edge while trying to preserve the surrounding region.
         *
         * @param edge Edge to dissolve.
         * @return True when the edge was dissolved or safely removed.
         */
        bool dissolve_edge(EdgeHandle edge);

        /**
         * @brief Merges one vertex into another, even when no edge connects them.
         *
         * @param sourceVertex Vertex that will be removed.
         * @param targetVertex Vertex that will receive the merged topology.
         * @return True when the merge succeeded.
         */
        bool merge_vertices(VertexHandle sourceVertex, VertexHandle targetVertex);

        /**
         * @brief Merges one vertex into another and assigns the final target position.
         *
         * @param sourceVertex Vertex that will be removed.
         * @param targetVertex Vertex that will receive the merged topology.
         * @param position Final object-space position assigned to targetVertex.
         * @return True when the merge succeeded.
         */
        bool merge_vertices_at_position(
            VertexHandle sourceVertex,
            VertexHandle targetVertex,
            const glm::vec3& position);

        /**
         * @brief Merges all active vertices that are closer than a distance threshold.
         *
         * @param distance Maximum distance between vertices to merge.
         * @return Number of successful vertex merges.
         */
        std::size_t merge_vertices_by_distance(float distance);

        /**
         * @brief Merges vertices from a restricted vertex set using a distance threshold.
         *
         * @param vertices Candidate vertices to weld.
         * @param distance Maximum distance between vertices to merge.
         * @return Number of successful vertex merges.
         */
        std::size_t weld_vertices(
            const std::vector<VertexHandle>& vertices,
            float distance);

        /**
         * @brief Dissolves a vertex when it can be removed without invalid topology.
         *
         * @param vertex Vertex to dissolve.
         * @return True when the vertex was dissolved or safely removed.
         */
        bool dissolve_vertex(VertexHandle vertex);

        /**
         * @brief Dissolves a face and removes loose boundary elements left behind.
         *
         * @param face Face to dissolve.
         * @return True when the face existed and was dissolved.
         */
        bool dissolve_face(FaceHandle face);

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
         * @brief Flips a manifold edge shared by two triangular faces.
         *
         * @param edge Edge to flip.
         * @return True when the edge was shared by two triangles and was flipped.
         */
        bool flip_edge(EdgeHandle edge);

        /**
         * @brief Recomputes all active face normals and records normal changes.
         *
         * This method is kept for compatibility with the current LEMEditor facade.
         * New code should prefer GeometryEditor for normal rebuild operations.
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
         * @param loop Loop to detach.
         * @return True when the loop could be detached.
         */
        bool remove_loop_from_radial(LoopHandle loop);

        /**
         * @brief Inserts a loop into an edge radial cycle.
         *
         * @param loop Loop to insert.
         * @param edge Edge that will own the radial cycle.
         * @return True when insertion succeeded.
         */
        bool insert_loop_into_radial(LoopHandle loop, EdgeHandle edge);

        /**
         * @brief Refreshes the entry loop stored by an edge.
         *
         * @param edge Edge whose entry loop may need updating.
         */
        void refresh_edge_entry_loop(EdgeHandle edge);

        /**
         * @brief Refreshes the incident edge stored by a vertex.
         *
         * @param vertex Vertex whose incident edge may need updating.
         */
        void refresh_vertex_incident_edge(VertexHandle vertex);

        /**
         * @brief Reassigns the vertex referenced by a loop.
         *
         * @param loop Loop to update.
         * @param vertex New loop vertex.
         * @return True when the loop was updated.
         */
        bool update_loop_vertex(LoopHandle loop, VertexHandle vertex);

        /**
         * @brief Reassigns the edge referenced by a loop.
         *
         * @param loop Loop to update.
         * @param edge New loop edge.
         * @return True when the loop was relinked.
         */
        bool update_loop_edge(LoopHandle loop, EdgeHandle edge);

        /**
         * @brief Replaces all occurrences of one vertex in a face boundary.
         *
         * @param face Face whose boundary loops will be edited.
         * @param oldVertex Vertex to replace.
         * @param newVertex Replacement vertex.
         * @return True when at least one loop was updated.
         */
        bool replace_vertex_in_face(
            FaceHandle face,
            VertexHandle oldVertex,
            VertexHandle newVertex);

        LEM& mesh_;
        LEMDiff& diff_;
    };

}