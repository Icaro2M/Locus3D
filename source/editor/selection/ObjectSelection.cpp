/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/selection/ObjectSelection.h"

namespace locus::editor {

    void ObjectSelection::set(SceneNodeId id)
    {
        selected_.clear();

        if (id.is_valid()) {
            selected_.add(id);
        }

        active_ = id;
    }

    void ObjectSelection::set(const std::vector<SceneNodeId>& ids, SceneNodeId active)
    {
        selected_.set(ids);

        if (active.is_valid() && selected_.contains(active)) {
            active_ = active;
            return;
        }

        if (!selected_.empty()) {
            active_ = selected_.items().back();
            return;
        }

        active_ = {};
    }

    bool ObjectSelection::add(SceneNodeId id)
    {
        if (id.is_invalid()) {
            return false;
        }

        const bool added = selected_.add(id);
        if (active_.is_invalid()) {
            active_ = id;
        }

        return added;
    }

    bool ObjectSelection::remove(SceneNodeId id)
    {
        const bool removed = selected_.remove(id);
        if (!removed) {
            return false;
        }

        if (active_ == id) {
            active_ = selected_.empty() ? SceneNodeId{} : selected_.items().back();
        }

        if (hovered_ == id) {
            hovered_ = {};
        }

        return true;
    }

    bool ObjectSelection::toggle(SceneNodeId id)
    {
        if (id.is_invalid()) {
            return false;
        }

        const bool selected = selected_.toggle(id);

        if (selected) {
            active_ = id;
        }
        else if (active_ == id) {
            active_ = selected_.empty() ? SceneNodeId{} : selected_.items().back();
        }

        return selected;
    }

    bool ObjectSelection::contains(SceneNodeId id) const
    {
        return selected_.contains(id);
    }

    void ObjectSelection::clear()
    {
        selected_.clear();
        active_ = {};
        hovered_ = {};
    }

    void ObjectSelection::clear_hovered()
    {
        hovered_ = {};
    }

    const std::vector<SceneNodeId>& ObjectSelection::selected() const
    {
        return selected_.items();
    }

    const SelectionSet<SceneNodeId>& ObjectSelection::set() const
    {
        return selected_;
    }

    SceneNodeId ObjectSelection::active() const
    {
        return active_;
    }

    void ObjectSelection::set_active(SceneNodeId id)
    {
        if (id.is_valid() && !selected_.contains(id)) {
            selected_.add(id);
        }

        active_ = id;
    }

    SceneNodeId ObjectSelection::hovered() const
    {
        return hovered_;
    }

    void ObjectSelection::set_hovered(SceneNodeId id)
    {
        hovered_ = id;
    }

    std::size_t ObjectSelection::size() const
    {
        return selected_.size();
    }

    bool ObjectSelection::empty() const
    {
        return selected_.empty();
    }

}