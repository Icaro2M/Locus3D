/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/selection/SelectionState.h"

namespace locus::editor {

    /**
     * @brief Serializable snapshot of object selection state.
     */
    struct ObjectSelectionSnapshot {
        /**
         * @brief Selected scene objects.
         */
        std::vector<SceneNodeId> selected;

        /**
         * @brief Active scene object.
         */
        SceneNodeId active{};

        /**
         * @brief Hovered scene object.
         */
        SceneNodeId hovered{};
    };

    /**
     * @brief Serializable snapshot of mesh component selection state.
     */
    struct MeshSelectionSnapshot {
        /**
         * @brief Active mesh scene node.
         */
        SceneNodeId activeMesh{};

        /**
         * @brief Selected vertices.
         */
        std::vector<kernel::geometry::VertexHandle> vertices;

        /**
         * @brief Selected edges.
         */
        std::vector<kernel::geometry::EdgeHandle> edges;

        /**
         * @brief Selected loops.
         */
        std::vector<kernel::geometry::LoopHandle> loops;

        /**
         * @brief Selected faces.
         */
        std::vector<kernel::geometry::FaceHandle> faces;

        /**
         * @brief Hovered vertex.
         */
        kernel::geometry::VertexHandle hoveredVertex{};

        /**
         * @brief Hovered edge.
         */
        kernel::geometry::EdgeHandle hoveredEdge{};

        /**
         * @brief Hovered loop.
         */
        kernel::geometry::LoopHandle hoveredLoop{};

        /**
         * @brief Hovered face.
         */
        kernel::geometry::FaceHandle hoveredFace{};
    };

    /**
     * @brief Serializable snapshot of the complete editor selection state.
     */
    struct SelectionSnapshot {
        /**
         * @brief Current selection granularity.
         */
        SelectionGranularity granularity = SelectionGranularity::Object;

        /**
         * @brief Current selection scope.
         */
        SelectionScope scope = SelectionScope::Scene;

        /**
         * @brief Object selection snapshot.
         */
        ObjectSelectionSnapshot objects{};

        /**
         * @brief Mesh selection snapshot.
         */
        MeshSelectionSnapshot mesh{};
    };

    /**
     * @brief Creates snapshots from selection state.
     */
    class SelectionSerializer {
    public:
        /**
         * @brief Captures the current selection state.
         *
         * @param state Selection state to capture.
         * @return Captured snapshot.
         */
        [[nodiscard]] static SelectionSnapshot capture(const SelectionState& state)
        {
            SelectionSnapshot snapshot;
            snapshot.granularity = state.granularity();
            snapshot.scope = state.scope();

            snapshot.objects.selected = state.objects().selected();
            snapshot.objects.active = state.objects().active();
            snapshot.objects.hovered = state.objects().hovered();

            snapshot.mesh.activeMesh = state.mesh().active_mesh();
            snapshot.mesh.vertices = state.mesh().vertices().items();
            snapshot.mesh.edges = state.mesh().edges().items();
            snapshot.mesh.loops = state.mesh().loops().items();
            snapshot.mesh.faces = state.mesh().faces().items();
            snapshot.mesh.hoveredVertex = state.mesh().hovered_vertex();
            snapshot.mesh.hoveredEdge = state.mesh().hovered_edge();
            snapshot.mesh.hoveredLoop = state.mesh().hovered_loop();
            snapshot.mesh.hoveredFace = state.mesh().hovered_face();

            return snapshot;
        }

        /**
         * @brief Restores a selection snapshot.
         *
         * @param snapshot Snapshot to restore.
         * @param state Selection state to mutate.
         */
        static void restore(const SelectionSnapshot& snapshot, SelectionState& state)
        {
            state.clear();

            state.objects().set(snapshot.objects.selected, snapshot.objects.active);
            state.objects().set_hovered(snapshot.objects.hovered);

            state.mesh().set_active_mesh(snapshot.mesh.activeMesh);

            for (kernel::geometry::VertexHandle handle : snapshot.mesh.vertices) {
                state.mesh().add_vertex(handle);
            }

            for (kernel::geometry::EdgeHandle handle : snapshot.mesh.edges) {
                state.mesh().add_edge(handle);
            }

            for (kernel::geometry::LoopHandle handle : snapshot.mesh.loops) {
                state.mesh().add_loop(handle);
            }

            for (kernel::geometry::FaceHandle handle : snapshot.mesh.faces) {
                state.mesh().add_face(handle);
            }

            state.mesh().set_hovered_vertex(snapshot.mesh.hoveredVertex);
            state.mesh().set_hovered_edge(snapshot.mesh.hoveredEdge);
            state.mesh().set_hovered_loop(snapshot.mesh.hoveredLoop);
            state.mesh().set_hovered_face(snapshot.mesh.hoveredFace);

            state.set_granularity(snapshot.granularity);
            state.set_scope(snapshot.scope);
            state.mark_dirty();
        }
    };

}