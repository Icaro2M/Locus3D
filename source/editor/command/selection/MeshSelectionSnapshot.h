/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/selection/MeshSelection.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <vector>

namespace locus::editor {

    /**
     * @brief Lightweight snapshot of mesh component selection state.
     */
    class MeshSelectionSnapshot {
    public:
        /**
         * @brief Captures mesh component selection state.
         *
         * @param selection Selection state to capture.
         */
        void capture(const SelectionState& selection)
        {
            const MeshSelection& mesh = selection.mesh();

            activeMesh_ = mesh.active_mesh();

            vertices_ = mesh.vertices().items();
            edges_ = mesh.edges().items();
            loops_ = mesh.loops().items();
            faces_ = mesh.faces().items();

            hoveredVertex_ = mesh.hovered_vertex();
            hoveredEdge_ = mesh.hovered_edge();
            hoveredLoop_ = mesh.hovered_loop();
            hoveredFace_ = mesh.hovered_face();

            granularity_ = selection.granularity();
            scope_ = selection.scope();

            hasSnapshot_ = true;
        }

        /**
         * @brief Restores mesh component selection state.
         *
         * @param selection Selection state to restore.
         */
        void restore(SelectionState& selection) const
        {
            if (!hasSnapshot_) {
                return;
            }

            MeshSelection& mesh = selection.mesh();

            mesh.clear_components();
            mesh.set_active_mesh(activeMesh_);

            for (kernel::geometry::VertexHandle handle : vertices_) {
                mesh.add_vertex(handle);
            }

            for (kernel::geometry::EdgeHandle handle : edges_) {
                mesh.add_edge(handle);
            }

            for (kernel::geometry::LoopHandle handle : loops_) {
                mesh.add_loop(handle);
            }

            for (kernel::geometry::FaceHandle handle : faces_) {
                mesh.add_face(handle);
            }

            mesh.set_hovered_vertex(hoveredVertex_);
            mesh.set_hovered_edge(hoveredEdge_);
            mesh.set_hovered_loop(hoveredLoop_);
            mesh.set_hovered_face(hoveredFace_);

            selection.set_granularity(granularity_);
            selection.set_scope(scope_);
            selection.mark_dirty();
        }

        /**
         * @brief Checks whether this snapshot contains captured data.
         *
         * @return True when captured.
         */
        [[nodiscard]] bool is_valid() const
        {
            return hasSnapshot_;
        }

    private:
        SceneNodeId activeMesh_{};

        std::vector<kernel::geometry::VertexHandle> vertices_{};
        std::vector<kernel::geometry::EdgeHandle> edges_{};
        std::vector<kernel::geometry::LoopHandle> loops_{};
        std::vector<kernel::geometry::FaceHandle> faces_{};

        kernel::geometry::VertexHandle hoveredVertex_{};
        kernel::geometry::EdgeHandle hoveredEdge_{};
        kernel::geometry::LoopHandle hoveredLoop_{};
        kernel::geometry::FaceHandle hoveredFace_{};

        SelectionGranularity granularity_ = SelectionGranularity::Object;
        SelectionScope scope_ = SelectionScope::Scene;

        bool hasSnapshot_ = false;
    };

} // namespace locus::editor