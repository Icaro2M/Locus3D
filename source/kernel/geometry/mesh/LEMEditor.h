/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/editing/AttributeEditor.h"
#include "kernel/geometry/mesh/editing/GeometryEditor.h"
#include "kernel/geometry/mesh/editing/TopologyEditor.h"
#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEM.h"

#include <glm/glm.hpp>

#include <cstddef>
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
         * @brief Sets a vertex position and updates adjacent face normals.
         *
         * @param handle Vertex to modify.
         * @param position New object-space position.
         * @return True when the vertex exists and the edit was accepted.
         */
        bool set_vertex_position(VertexHandle handle, const glm::vec3& position);

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
         * @brief Recomputes all active face normals and records normal changes.
         */
        void rebuild_face_normals();

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