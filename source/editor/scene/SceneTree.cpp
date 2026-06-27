/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/scene/SceneTree.h"

#include <algorithm>

namespace locus::editor {

    SceneNodeId SceneTree::insert_node(std::unique_ptr<SceneNode> node)
    {
        if (!node || node->id().is_invalid()) {
            return {};
        }

        const SceneNodeId id = node->id();
        if (contains(id)) {
            return {};
        }

        node->set_parent({});
        nodes_.emplace(id, std::move(node));
        add_root(id);
        return id;
    }

    bool SceneTree::remove_node(SceneNodeId id)
    {
        SceneNode* node = find_node(id);
        if (!node) {
            return false;
        }

        std::vector<SceneNodeId> removedNodes;
        collect_subtree(id, removedNodes);

        const SceneNodeId parent = node->parent();
        if (SceneNode* parentNode = find_node(parent)) {
            parentNode->remove_child(id);
        }

        remove_from_roots(id);

        for (SceneNodeId removedId : removedNodes) {
            nodes_.erase(removedId);
            remove_from_roots(removedId);
        }

        return true;
    }

    bool SceneTree::reparent(SceneNodeId child, SceneNodeId parent)
    {
        if (child.is_invalid()) {
            return false;
        }

        SceneNode* childNode = find_node(child);
        if (!childNode) {
            return false;
        }

        if (parent.is_valid()) {
            SceneNode* parentNode = find_node(parent);
            if (!parentNode || parent == child || is_ancestor(child, parent)) {
                return false;
            }
        }

        const SceneNodeId oldParent = childNode->parent();
        if (SceneNode* oldParentNode = find_node(oldParent)) {
            oldParentNode->remove_child(child);
        }

        if (parent.is_valid()) {
            remove_from_roots(child);
            find_node(parent)->add_child(child);
        }
        else {
            add_root(child);
        }

        childNode->set_parent(parent);
        return true;
    }

    SceneNode* SceneTree::find_node(SceneNodeId id)
    {
        const auto it = nodes_.find(id);
        if (it == nodes_.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    const SceneNode* SceneTree::find_node(SceneNodeId id) const
    {
        const auto it = nodes_.find(id);
        if (it == nodes_.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    bool SceneTree::contains(SceneNodeId id) const
    {
        return nodes_.find(id) != nodes_.end();
    }

    bool SceneTree::is_ancestor(SceneNodeId ancestor, SceneNodeId node) const
    {
        const SceneNode* current = find_node(node);
        while (current && current->parent().is_valid()) {
            if (current->parent() == ancestor) {
                return true;
            }

            current = find_node(current->parent());
        }

        return false;
    }

    const std::vector<SceneNodeId>& SceneTree::roots() const
    {
        return roots_;
    }

    std::vector<SceneNodeId> SceneTree::node_ids() const
    {
        std::vector<SceneNodeId> ids;
        ids.reserve(nodes_.size());

        for (const auto& entry : nodes_) {
            ids.push_back(entry.first);
        }

        return ids;
    }

    std::size_t SceneTree::size() const
    {
        return nodes_.size();
    }

    bool SceneTree::empty() const
    {
        return nodes_.empty();
    }

    void SceneTree::clear()
    {
        nodes_.clear();
        roots_.clear();
        nextId_ = 1;
    }

    SceneNodeId SceneTree::allocate_id()
    {
        return SceneNodeId{ nextId_++ };
    }

    void SceneTree::remove_from_roots(SceneNodeId id)
    {
        const auto it = std::remove(roots_.begin(), roots_.end(), id);
        roots_.erase(it, roots_.end());
    }

    void SceneTree::add_root(SceneNodeId id)
    {
        if (id.is_invalid()) {
            return;
        }

        const auto it = std::find(roots_.begin(), roots_.end(), id);
        if (it != roots_.end()) {
            return;
        }

        roots_.push_back(id);
    }

    void SceneTree::collect_subtree(SceneNodeId id, std::vector<SceneNodeId>& out) const
    {
        const SceneNode* node = find_node(id);
        if (!node) {
            return;
        }

        out.push_back(id);

        for (SceneNodeId child : node->children()) {
            collect_subtree(child, out);
        }
    }

}