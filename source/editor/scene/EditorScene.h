/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/EmptyNode.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneTree.h"

namespace locus::editor {

    /**
     * @brief High-level editable scene owned by the editor layer.
     */
    class EditorScene {
    public:
        /**
         * @brief Creates an empty scene.
         */
        EditorScene() = default;

        EditorScene(const EditorScene&) = delete;
        EditorScene& operator=(const EditorScene&) = delete;
        EditorScene(EditorScene&&) noexcept = default;
        EditorScene& operator=(EditorScene&&) noexcept = default;

        /**
         * @brief Creates an empty transform node.
         *
         * @param name Human-readable node name.
         * @return Created node identifier.
         */
        SceneNodeId create_empty(const std::string& name);

        /**
         * @brief Creates an editable mesh node.
         *
         * @param name Human-readable node name.
         * @return Created node identifier.
         */
        SceneNodeId create_mesh(const std::string& name);

        /**
         * @brief Removes a node and all descendants.
         *
         * @param id Node to remove.
         * @return True when a node was removed.
         */
        bool remove_node(SceneNodeId id);

        /**
         * @brief Reparents a node.
         *
         * @param child Node to move.
         * @param parent New parent, or invalid for root.
         * @return True when the hierarchy change succeeded.
         */
        bool reparent(SceneNodeId child, SceneNodeId parent);

        /**
         * @brief Returns mutable access to the scene tree.
         *
         * @return Mutable tree reference.
         */
        [[nodiscard]] SceneTree& tree();

        /**
         * @brief Returns read-only access to the scene tree.
         *
         * @return Read-only tree reference.
         */
        [[nodiscard]] const SceneTree& tree() const;

        /**
         * @brief Finds a mutable scene node.
         *
         * @param id Node identifier.
         * @return Node pointer, or null when not found.
         */
        [[nodiscard]] SceneNode* find_node(SceneNodeId id);

        /**
         * @brief Finds a read-only scene node.
         *
         * @param id Node identifier.
         * @return Node pointer, or null when not found.
         */
        [[nodiscard]] const SceneNode* find_node(SceneNodeId id) const;

        /**
         * @brief Finds a mutable mesh node.
         *
         * @param id Node identifier.
         * @return Mesh node pointer, or null when not found or type mismatched.
         */
        [[nodiscard]] MeshNode* find_mesh(SceneNodeId id);

        /**
         * @brief Finds a read-only mesh node.
         *
         * @param id Node identifier.
         * @return Mesh node pointer, or null when not found or type mismatched.
         */
        [[nodiscard]] const MeshNode* find_mesh(SceneNodeId id) const;

        /**
         * @brief Removes every node from the scene.
         */
        void clear();

    private:
        SceneTree tree_{};
    };

}
