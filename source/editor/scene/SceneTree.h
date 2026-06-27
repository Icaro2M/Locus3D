/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNode.h"
#include "editor/scene/SceneNodeId.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace locus::editor {

    /**
     * @brief Owns editor scene nodes and manages hierarchy relationships.
     */
    class SceneTree {
    public:
        /**
         * @brief Creates an empty scene tree.
         */
        SceneTree() = default;

        SceneTree(const SceneTree&) = delete;
        SceneTree& operator=(const SceneTree&) = delete;

        /**
         * @brief Creates a node of a specific derived type.
         *
         * @tparam NodeT Concrete node type.
         * @tparam Args Constructor argument types after id.
         * @param args Constructor arguments after id.
         * @return Created node identifier.
         */
        template <typename NodeT, typename... Args>
        SceneNodeId create_node(Args&&... args)
        {
            const SceneNodeId id = allocate_id();
            auto node = std::make_unique<NodeT>(id, std::forward<Args>(args)...);
            insert_node(std::move(node));
            return id;
        }

        /**
         * @brief Inserts an already constructed node.
         *
         * @param node Node to insert.
         * @return Identifier of the inserted node, or invalid on failure.
         */
        SceneNodeId insert_node(std::unique_ptr<SceneNode> node);

        /**
         * @brief Removes a node and all of its descendants.
         *
         * @param id Node to remove.
         * @return True when a node was removed.
         */
        bool remove_node(SceneNodeId id);

        /**
         * @brief Changes the parent of a node.
         *
         * @param child Node to reparent.
         * @param parent New parent, or invalid to make the node a root.
         * @return True when the hierarchy change succeeded.
         */
        bool reparent(SceneNodeId child, SceneNodeId parent);

        /**
         * @brief Finds a mutable node by identifier.
         *
         * @param id Node identifier.
         * @return Node pointer, or null when not found.
         */
        [[nodiscard]] SceneNode* find_node(SceneNodeId id);

        /**
         * @brief Finds a read-only node by identifier.
         *
         * @param id Node identifier.
         * @return Node pointer, or null when not found.
         */
        [[nodiscard]] const SceneNode* find_node(SceneNodeId id) const;

        /**
         * @brief Finds a mutable node of a specific concrete type.
         *
         * @tparam NodeT Expected node type.
         * @param id Node identifier.
         * @return Node pointer, or null when not found or type mismatched.
         */
        template <typename NodeT>
        [[nodiscard]] NodeT* find_node_as(SceneNodeId id)
        {
            return dynamic_cast<NodeT*>(find_node(id));
        }

        /**
         * @brief Finds a read-only node of a specific concrete type.
         *
         * @tparam NodeT Expected node type.
         * @param id Node identifier.
         * @return Node pointer, or null when not found or type mismatched.
         */
        template <typename NodeT>
        [[nodiscard]] const NodeT* find_node_as(SceneNodeId id) const
        {
            return dynamic_cast<const NodeT*>(find_node(id));
        }

        /**
         * @brief Checks whether the tree contains a node.
         *
         * @param id Node identifier.
         * @return True when the node exists.
         */
        [[nodiscard]] bool contains(SceneNodeId id) const;

        /**
         * @brief Checks whether a node is an ancestor of another node.
         *
         * @param ancestor Potential ancestor.
         * @param node Node to test.
         * @return True when ancestor is found in the parent chain of node.
         */
        [[nodiscard]] bool is_ancestor(SceneNodeId ancestor, SceneNodeId node) const;

        /**
         * @brief Returns all root nodes.
         *
         * @return Ordered root identifier list.
         */
        [[nodiscard]] const std::vector<SceneNodeId>& roots() const;

        /**
         * @brief Returns all stored node identifiers.
         *
         * @return Node identifier list.
         */
        [[nodiscard]] std::vector<SceneNodeId> node_ids() const;

        /**
         * @brief Returns the number of stored nodes.
         *
         * @return Node count.
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Checks whether no nodes are stored.
         *
         * @return True when the tree is empty.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Removes all nodes from the tree.
         */
        void clear();

    private:
        SceneNodeId allocate_id();
        void remove_from_roots(SceneNodeId id);
        void add_root(SceneNodeId id);
        void collect_subtree(SceneNodeId id, std::vector<SceneNodeId>& out) const;

        std::unordered_map<SceneNodeId, std::unique_ptr<SceneNode>> nodes_{};
        std::vector<SceneNodeId> roots_{};
        SceneNodeIdValue nextId_ = 1;
    };

}