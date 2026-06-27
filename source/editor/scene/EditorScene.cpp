/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/scene/EditorScene.h"

namespace locus::editor {

    SceneNodeId EditorScene::create_empty(const std::string& name)
    {
        return tree_.create_node<EmptyNode>(name);
    }

    SceneNodeId EditorScene::create_mesh(const std::string& name)
    {
        return tree_.create_node<MeshNode>(name);
    }

    bool EditorScene::remove_node(SceneNodeId id)
    {
        return tree_.remove_node(id);
    }

    bool EditorScene::reparent(SceneNodeId child, SceneNodeId parent)
    {
        return tree_.reparent(child, parent);
    }

    SceneTree& EditorScene::tree()
    {
        return tree_;
    }

    const SceneTree& EditorScene::tree() const
    {
        return tree_;
    }

    SceneNode* EditorScene::find_node(SceneNodeId id)
    {
        return tree_.find_node(id);
    }

    const SceneNode* EditorScene::find_node(SceneNodeId id) const
    {
        return tree_.find_node(id);
    }

    MeshNode* EditorScene::find_mesh(SceneNodeId id)
    {
        return tree_.find_node_as<MeshNode>(id);
    }

    const MeshNode* EditorScene::find_mesh(SceneNodeId id) const
    {
        return tree_.find_node_as<MeshNode>(id);
    }

    void EditorScene::clear()
    {
        tree_.clear();
    }

}