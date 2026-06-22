/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/editing/AttributeEditor.h"
#include "kernel/geometry/mesh/editing/GeometryEditor.h"
#include "kernel/geometry/mesh/editing/TopologyEditor.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Facade for editing a LEM while recording a change diff.
     *
     * LEMEditor exposes a stable editing API while delegating concrete mutation
     * responsibilities to specialized editors for topology, geometry, and
     * attributes.
     */
    class LEMEditor {
    public:
        /**
         * @brief Creates an editor facade bound to an editable mesh.
         *
         * @param mesh Mesh that receives all mutations.
         */
        explicit LEMEditor(LEM& mesh);

        LEMEditor(const LEMEditor&) = delete;
        LEMEditor& operator=(const LEMEditor&) = delete;

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
         * @brief Returns the topology editor.
         *
         * @return Mutable topology editor reference.
         */
        [[nodiscard]] TopologyEditor& topology();

        /**
         * @brief Returns the topology editor.
         *
         * @return Read-only topology editor reference.
         */
        [[nodiscard]] const TopologyEditor& topology() const;

        /**
         * @brief Returns the geometry editor.
         *
         * @return Mutable geometry editor reference.
         */
        [[nodiscard]] GeometryEditor& geometry();

        /**
         * @brief Returns the geometry editor.
         *
         * @return Read-only geometry editor reference.
         */
        [[nodiscard]] const GeometryEditor& geometry() const;

        /**
         * @brief Returns the attribute editor.
         *
         * @return Mutable attribute editor reference.
         */
        [[nodiscard]] AttributeEditor& attributes();

        /**
         * @brief Returns the attribute editor.
         *
         * @return Read-only attribute editor reference.
         */
        [[nodiscard]] const AttributeEditor& attributes() const;

        /**
         * @brief Returns the accumulated edit diff.
         *
         * @return Read-only diff reference.
         */
        [[nodiscard]] const LEMDiff& diff() const;

        /**
         * @brief Moves the accumulated diff out of the editor and clears it.
         *
         * @return Diff recorded since the last clear or take.
         */
        [[nodiscard]] LEMDiff take_diff();

        /**
         * @brief Clears the accumulated diff without changing the mesh.
         */
        void clear_diff();

        /**
         * @brief Adds a loose vertex and records the created element.
         *
         * @param position Vertex position in object space.
         * @return Handle referencing the created vertex.
         */
        VertexHandle add_vertex(const glm::vec3& position);

        /**
         * @brief Finds or creates an edge and records topology changes when inserted.
         *
         * @param vertexA First endpoint vertex.
         * @param vertexB Second endpoint vertex.
         * @return Handle referencing the existing or created edge.
         */
        EdgeHandle find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB);

        /**
         * @brief Adds a polygonal face and records all new topology elements.
         *
         * @param vertices Ordered face vertices.
         * @return Handle referencing the created face, or an invalid handle on failure.
         */
        FaceHandle add_face(const std::vector<VertexHandle>& vertices);

        /**
         * @brief Removes a face and its boundary loops.
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
         * @brief Dissolves an edge while trying to preserve the surrounding region.
         *
         * @param edge Edge to dissolve.
         * @return True when the edge was dissolved or safely removed.
         */
        bool dissolve_edge(EdgeHandle edge);

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
         * @brief Reverses the winding of a face.
         *
         * @param face Face to flip.
         * @return True when the face existed and was flipped.
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
         * @brief Sets a vertex position and updates adjacent face normals.
         *
         * @param handle Vertex to modify.
         * @param position New object-space position.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_vertex_position(VertexHandle handle, const glm::vec3& position);

        /**
         * @brief Linearly interpolates a vertex position toward a target.
         *
         * @param handle Vertex to modify.
         * @param target Target object-space position.
         * @param t Interpolation factor clamped to the range [0, 1].
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_vertex_position_lerp(VertexHandle handle, const glm::vec3& target, float t);

        /**
         * @brief Translates a single vertex by an object-space offset.
         *
         * @param handle Vertex to translate.
         * @param offset Translation offset.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool translate_vertex(VertexHandle handle, const glm::vec3& offset);

        /**
         * @brief Translates multiple vertices by the same object-space offset.
         *
         * @param vertices Vertices to translate.
         * @param offset Translation offset.
         * @return Number of vertices accepted by the edit.
         */
        std::size_t translate_vertices(const std::vector<VertexHandle>& vertices, const glm::vec3& offset);

        /**
         * @brief Applies a transform matrix to multiple vertices.
         *
         * @param vertices Vertices to transform.
         * @param transform Transform matrix applied to each position.
         * @return Number of vertices accepted by the edit.
         */
        std::size_t transform_vertices(const std::vector<VertexHandle>& vertices, const glm::mat4& transform);

        /**
         * @brief Moves a vertex along the averaged normal of adjacent faces.
         *
         * @param handle Vertex to move.
         * @param distance Signed movement distance.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool offset_vertex_along_normal(VertexHandle handle, float distance);

        /**
         * @brief Recomputes all active face normals and records normal changes.
         */
        void rebuild_face_normals();

        /**
         * @brief Recomputes normals for faces adjacent to a vertex.
         *
         * @param vertex Vertex whose adjacent faces will be updated.
         */
        void rebuild_normals_around_vertex(VertexHandle vertex);

        /**
         * @brief Recomputes a single face normal.
         *
         * @param face Face whose normal will be updated.
         * @return True when the face exists and was updated.
         */
        bool rebuild_normals_around_face(FaceHandle face);

        /**
         * @brief Changes a vertex selection flag.
         *
         * @param handle Vertex to modify.
         * @param selected New selection state.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_selected(VertexHandle handle, bool selected);

        /**
         * @brief Changes an edge selection flag.
         *
         * @param handle Edge to modify.
         * @param selected New selection state.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_selected(EdgeHandle handle, bool selected);

        /**
         * @brief Changes a face selection flag.
         *
         * @param handle Face to modify.
         * @param selected New selection state.
         * @return True when the face exists and the edit was accepted.
         */
        bool set_selected(FaceHandle handle, bool selected);

        /**
         * @brief Clears selection flags on all active vertices, edges, and faces.
         */
        void clear_selection();

        /**
         * @brief Changes a vertex visibility flag.
         *
         * @param handle Vertex to modify.
         * @param hidden New hidden state.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_hidden(VertexHandle handle, bool hidden);

        /**
         * @brief Changes an edge visibility flag.
         *
         * @param handle Edge to modify.
         * @param hidden New hidden state.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_hidden(EdgeHandle handle, bool hidden);

        /**
         * @brief Changes a face visibility flag.
         *
         * @param handle Face to modify.
         * @param hidden New hidden state.
         * @return True when the face exists and the edit was accepted.
         */
        bool set_hidden(FaceHandle handle, bool hidden);

        /**
         * @brief Clears hidden flags on all active vertices, edges, and faces.
         */
        void clear_visibility();

        /**
         * @brief Changes whether adjacent faces should be smoothed across an edge.
         *
         * @param handle Edge to modify.
         * @param smooth New smoothing state.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_smooth(EdgeHandle handle, bool smooth);

        /**
         * @brief Changes crease strength on an edge.
         *
         * @param handle Edge to modify.
         * @param crease New crease strength clamped to the range [0, 1].
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_crease(EdgeHandle handle, float crease);

        /**
         * @brief Changes a vertex internal tag.
         *
         * @param handle Vertex to modify.
         * @param tag New tag value.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_tag(VertexHandle handle, std::uint32_t tag);

        /**
         * @brief Changes an edge internal tag.
         *
         * @param handle Edge to modify.
         * @param tag New tag value.
         * @return True when the edge exists and the edit was accepted.
         */
        bool set_tag(EdgeHandle handle, std::uint32_t tag);

        /**
         * @brief Changes a face internal tag.
         *
         * @param handle Face to modify.
         * @param tag New tag value.
         * @return True when the face exists and the edit was accepted.
         */
        bool set_tag(FaceHandle handle, std::uint32_t tag);

        /**
         * @brief Clears internal tags on all active vertices, edges, and faces.
         */
        void clear_tags();

        /**
         * @brief Clears the mesh and records a whole-mesh clear event.
         */
        void clear();

    private:
        LEM& mesh_;
        LEMDiff diff_{};
        TopologyEditor topology_;
        GeometryEditor geometry_;
        AttributeEditor attributes_;
    };

}