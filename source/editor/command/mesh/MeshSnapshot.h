/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/selection/MeshSelectionSnapshot.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/mesh/LEM.h"

namespace locus::editor {

    /**
     * @brief Snapshot of an editable mesh node and its component selection state.
     *
     * Mesh commands use this snapshot for robust undo and redo of topology-heavy
     * edits. Instead of trying to reverse low-level diffs, the command layer stores
     * the complete LEM state before and after the operation.
     */
    class MeshSnapshot {
    public:
        /**
         * @brief Captures the mesh and current editor mesh selection.
         *
         * @param node Mesh node to capture.
         * @param selection Selection state to capture.
         */
        void capture(const MeshNode& node, const SelectionState& selection)
        {
            mesh_ = node.mesh();
            selection_.capture(selection);
            hasSnapshot_ = true;
        }

        /**
         * @brief Restores the captured mesh and component selection.
         *
         * @param node Mesh node to restore.
         * @param selection Selection state to restore.
         */
        void restore(MeshNode& node, SelectionState& selection) const
        {
            if (!hasSnapshot_) {
                return;
            }

            node.mesh() = mesh_;
            node.bump_mesh_revision();
            selection_.restore(selection);

            node.mark_dirty(
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Selection |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        /**
         * @brief Returns the captured mesh.
         *
         * @return Read-only captured mesh.
         */
        [[nodiscard]] const kernel::geometry::LEM& mesh() const
        {
            return mesh_;
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
        kernel::geometry::LEM mesh_{};
        MeshSelectionSnapshot selection_{};
        bool hasSnapshot_ = false;
    };

} // namespace locus::editor
