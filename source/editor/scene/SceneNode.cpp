/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/scene/SceneNode.h"

#include <algorithm>
#include <utility>

namespace locus::editor {

    SceneNode::SceneNode(SceneNodeId id, NodeType type, std::string name)
        : id_(id)
        , type_(type)
    {
        metadata_.name = std::move(name);
    }

    SceneNodeId SceneNode::id() const
    {
        return id_;
    }

    SceneNodeId SceneNode::parent() const
    {
        return parent_;
    }

    NodeType SceneNode::type() const
    {
        return type_;
    }

    NodeTransform& SceneNode::transform()
    {
        return transform_;
    }

    const NodeTransform& SceneNode::transform() const
    {
        return transform_;
    }

    NodePivot& SceneNode::pivot()
    {
        return pivot_;
    }

    const NodePivot& SceneNode::pivot() const
    {
        return pivot_;
    }

    NodeMetadata& SceneNode::metadata()
    {
        mark_dirty(EditorDirtyFlags::Scene);
        return metadata_;
    }

    const NodeMetadata& SceneNode::metadata() const
    {
        return metadata_;
    }

    const std::vector<SceneNodeId>& SceneNode::children() const
    {
        return children_;
    }

    bool SceneNode::has_parent() const
    {
        return parent_.is_valid();
    }

    bool SceneNode::is_visible() const
    {
        return metadata_.visible;
    }

    bool SceneNode::is_selectable() const
    {
        return metadata_.visible && metadata_.selectable && !metadata_.locked;
    }

    void SceneNode::mark_dirty(EditorDirtyFlags flags)
    {
        dirtyFlags_ |= flags;
    }

    void SceneNode::clear_dirty(EditorDirtyFlags flags)
    {
        dirtyFlags_ = static_cast<EditorDirtyFlags>(
            static_cast<std::uint32_t>(dirtyFlags_) &
            ~static_cast<std::uint32_t>(flags));
    }

    EditorDirtyFlags SceneNode::dirty_flags() const
    {
        return dirtyFlags_;
    }

    void SceneNode::set_parent(SceneNodeId parent)
    {
        parent_ = parent;
        mark_dirty(EditorDirtyFlags::Scene);
    }

    void SceneNode::add_child(SceneNodeId child)
    {
        if (child.is_invalid()) {
            return;
        }

        const auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            return;
        }

        children_.push_back(child);
        mark_dirty(EditorDirtyFlags::Scene);
    }

    void SceneNode::remove_child(SceneNodeId child)
    {
        const auto it = std::remove(children_.begin(), children_.end(), child);
        if (it == children_.end()) {
            return;
        }

        children_.erase(it, children_.end());
        mark_dirty(EditorDirtyFlags::Scene);
    }

}