/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNode.h"
#include "kernel/geometry/mesh/LEM.h"

#include <cstdint>

namespace locus::editor {

    /**
     * @brief Scene node containing an editable Locus Editable Mesh.
     */
    class MeshNode final : public SceneNode {
    public:
        /**
         * @brief Creates a mesh scene node.
         *
         * @param id Stable node identifier.
         * @param name Human-readable node name.
         */
        MeshNode(SceneNodeId id, const std::string& name)
            : SceneNode(id, NodeType::Mesh, name)
        {
        }

        /**
         * @brief Returns the editable mesh owned by this node.
         *
         * @return Mutable mesh reference.
         */
        [[nodiscard]] kernel::geometry::LEM& mesh()
        {
            mark_dirty(EditorDirtyFlags::Mesh);
            return mesh_;
        }

        /**
         * @brief Returns the editable mesh owned by this node.
         *
         * @return Read-only mesh reference.
         */
        [[nodiscard]] const kernel::geometry::LEM& mesh() const
        {
            return mesh_;
        }

        /**
         * @brief Returns the revision used by render caches for this mesh.
         *
         * @return Monotonic mesh revision.
         */
        [[nodiscard]] std::uint64_t mesh_revision() const noexcept
        {
            return meshRevision_;
        }

        /**
         * @brief Marks the mesh payload as definitively modified.
         */
        void bump_mesh_revision() noexcept
        {
            ++meshRevision_;
        }

    private:
        kernel::geometry::LEM mesh_{};
        std::uint64_t meshRevision_ = 1;
    };

}
