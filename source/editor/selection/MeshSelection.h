/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionSet.h"
#include "kernel/geometry/mesh/LEMHandles.h"

namespace locus::editor {

    /**
     * @brief Stores component selection state for one active editable mesh.
     */
    class MeshSelection {
    public:
        /**
         * @brief Changes the active mesh object.
         *
         * Changing the active mesh clears all component selections.
         *
         * @param id Mesh scene node identifier.
         */
        void set_active_mesh(SceneNodeId id);

        /**
         * @brief Returns the active mesh object.
         *
         * @return Mesh scene node identifier, or invalid when none is active.
         */
        [[nodiscard]] SceneNodeId active_mesh() const;

        /**
         * @brief Clears active mesh and all selected and hovered components.
         */
        void clear();

        /**
         * @brief Clears all selected and hovered components but keeps the active mesh.
         */
        void clear_components();

        /**
         * @brief Selects a single vertex.
         *
         * @param handle Vertex handle.
         */
        void set_vertex(kernel::geometry::VertexHandle handle);

        /**
         * @brief Selects a single edge.
         *
         * @param handle Edge handle.
         */
        void set_edge(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Selects a single loop.
         *
         * @param handle Loop handle.
         */
        void set_loop(kernel::geometry::LoopHandle handle);

        /**
         * @brief Selects a single face.
         *
         * @param handle Face handle.
         */
        void set_face(kernel::geometry::FaceHandle handle);

        /**
         * @brief Adds a vertex to the selection.
         *
         * @param handle Vertex handle.
         * @return True when added.
         */
        bool add_vertex(kernel::geometry::VertexHandle handle);

        /**
         * @brief Adds an edge to the selection.
         *
         * @param handle Edge handle.
         * @return True when added.
         */
        bool add_edge(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Adds a loop to the selection.
         *
         * @param handle Loop handle.
         * @return True when added.
         */
        bool add_loop(kernel::geometry::LoopHandle handle);

        /**
         * @brief Adds a face to the selection.
         *
         * @param handle Face handle.
         * @return True when added.
         */
        bool add_face(kernel::geometry::FaceHandle handle);

        /**
         * @brief Removes a vertex from the selection.
         *
         * @param handle Vertex handle.
         * @return True when removed.
         */
        bool remove_vertex(kernel::geometry::VertexHandle handle);

        /**
         * @brief Removes an edge from the selection.
         *
         * @param handle Edge handle.
         * @return True when removed.
         */
        bool remove_edge(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Removes a loop from the selection.
         *
         * @param handle Loop handle.
         * @return True when removed.
         */
        bool remove_loop(kernel::geometry::LoopHandle handle);

        /**
         * @brief Removes a face from the selection.
         *
         * @param handle Face handle.
         * @return True when removed.
         */
        bool remove_face(kernel::geometry::FaceHandle handle);

        /**
         * @brief Toggles a vertex in the selection.
         *
         * @param handle Vertex handle.
         * @return True when selected after the operation.
         */
        bool toggle_vertex(kernel::geometry::VertexHandle handle);

        /**
         * @brief Toggles an edge in the selection.
         *
         * @param handle Edge handle.
         * @return True when selected after the operation.
         */
        bool toggle_edge(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Toggles a loop in the selection.
         *
         * @param handle Loop handle.
         * @return True when selected after the operation.
         */
        bool toggle_loop(kernel::geometry::LoopHandle handle);

        /**
         * @brief Toggles a face in the selection.
         *
         * @param handle Face handle.
         * @return True when selected after the operation.
         */
        bool toggle_face(kernel::geometry::FaceHandle handle);

        /**
         * @brief Returns selected vertices.
         *
         * @return Selected vertex set.
         */
        [[nodiscard]] const SelectionSet<kernel::geometry::VertexHandle>& vertices() const;

        /**
         * @brief Returns selected edges.
         *
         * @return Selected edge set.
         */
        [[nodiscard]] const SelectionSet<kernel::geometry::EdgeHandle>& edges() const;

        /**
         * @brief Returns selected loops.
         *
         * @return Selected loop set.
         */
        [[nodiscard]] const SelectionSet<kernel::geometry::LoopHandle>& loops() const;

        /**
         * @brief Returns selected faces.
         *
         * @return Selected face set.
         */
        [[nodiscard]] const SelectionSet<kernel::geometry::FaceHandle>& faces() const;

        /**
         * @brief Returns the hovered vertex.
         *
         * @return Hovered vertex handle.
         */
        [[nodiscard]] kernel::geometry::VertexHandle hovered_vertex() const;

        /**
         * @brief Returns the hovered edge.
         *
         * @return Hovered edge handle.
         */
        [[nodiscard]] kernel::geometry::EdgeHandle hovered_edge() const;

        /**
         * @brief Returns the hovered loop.
         *
         * @return Hovered loop handle.
         */
        [[nodiscard]] kernel::geometry::LoopHandle hovered_loop() const;

        /**
         * @brief Returns the hovered face.
         *
         * @return Hovered face handle.
         */
        [[nodiscard]] kernel::geometry::FaceHandle hovered_face() const;

        /**
         * @brief Changes the hovered vertex.
         *
         * @param handle Hovered vertex handle.
         */
        void set_hovered_vertex(kernel::geometry::VertexHandle handle);

        /**
         * @brief Changes the hovered edge.
         *
         * @param handle Hovered edge handle.
         */
        void set_hovered_edge(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Changes the hovered loop.
         *
         * @param handle Hovered loop handle.
         */
        void set_hovered_loop(kernel::geometry::LoopHandle handle);

        /**
         * @brief Changes the hovered face.
         *
         * @param handle Hovered face handle.
         */
        void set_hovered_face(kernel::geometry::FaceHandle handle);

        /**
         * @brief Clears all hovered components.
         */
        void clear_hovered();

        /**
         * @brief Checks whether no mesh component is selected.
         *
         * @return True when all component selections are empty.
         */
        [[nodiscard]] bool empty() const;

    private:
        SceneNodeId activeMesh_{};

        SelectionSet<kernel::geometry::VertexHandle> vertices_{};
        SelectionSet<kernel::geometry::EdgeHandle> edges_{};
        SelectionSet<kernel::geometry::LoopHandle> loops_{};
        SelectionSet<kernel::geometry::FaceHandle> faces_{};

        kernel::geometry::VertexHandle hoveredVertex_{};
        kernel::geometry::EdgeHandle hoveredEdge_{};
        kernel::geometry::LoopHandle hoveredLoop_{};
        kernel::geometry::FaceHandle hoveredFace_{};
    };

}