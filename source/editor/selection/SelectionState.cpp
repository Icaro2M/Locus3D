/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/selection/SelectionState.h"

namespace locus::editor {

    ObjectSelection& SelectionState::objects()
    {
        mark_dirty();
        return objects_;
    }

    const ObjectSelection& SelectionState::objects() const
    {
        return objects_;
    }

    MeshSelection& SelectionState::mesh()
    {
        mark_dirty();
        return mesh_;
    }

    const MeshSelection& SelectionState::mesh() const
    {
        return mesh_;
    }

    SelectionGranularity SelectionState::granularity() const
    {
        return granularity_;
    }

    void SelectionState::set_granularity(SelectionGranularity granularity)
    {
        if (granularity_ == granularity) {
            return;
        }

        granularity_ = granularity;
        scope_ = is_mesh_granularity(granularity_)
            ? SelectionScope::ActiveMesh
            : SelectionScope::Scene;

        mark_dirty();
    }

    SelectionScope SelectionState::scope() const
    {
        return scope_;
    }

    void SelectionState::set_scope(SelectionScope scope)
    {
        if (scope_ == scope) {
            return;
        }

        scope_ = scope;

        if (scope_ == SelectionScope::Scene) {
            granularity_ = SelectionGranularity::Object;
        }

        mark_dirty();
    }

    void SelectionState::clear()
    {
        objects_.clear();
        mesh_.clear();
        granularity_ = SelectionGranularity::Object;
        scope_ = SelectionScope::Scene;
        mark_dirty();
    }

    bool SelectionState::is_dirty() const
    {
        return dirty_;
    }

    void SelectionState::mark_dirty()
    {
        dirty_ = true;
    }

    void SelectionState::clear_dirty()
    {
        dirty_ = false;
    }

}