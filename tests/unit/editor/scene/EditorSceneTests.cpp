/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorSceneTestSuite.h"

#include "editor/Editor.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/NodeType.h"

namespace locus::tests {

TestResult run_editor_scene_tests()
{
    editor::EditorScene scene;

    const editor::SceneNodeId empty = scene.create_empty("Root");
    const editor::SceneNodeId mesh = scene.create_mesh("Mesh");

    if (!empty.is_valid() || !mesh.is_valid()) {
        return TestResult::fail("EditorScene should create valid node ids");
    }

    const editor::SceneNode* emptyNode = scene.find_node(empty);
    const editor::SceneNode* meshNode = scene.find_node(mesh);

    if (!emptyNode || !meshNode) {
        return TestResult::fail("EditorScene should find created nodes");
    }

    if (emptyNode->type() != editor::NodeType::Empty ||
        meshNode->type() != editor::NodeType::Mesh) {
        return TestResult::fail("EditorScene should create nodes with expected types");
    }

    if (scene.find_mesh(empty) != nullptr || scene.find_mesh(mesh) == nullptr) {
        return TestResult::fail("find_mesh should only return mesh nodes");
    }

    if (!scene.reparent(mesh, empty)) {
        return TestResult::fail("EditorScene should expose scene reparenting");
    }

    if (!scene.tree().is_ancestor(empty, mesh)) {
        return TestResult::fail("EditorScene tree should reflect reparented hierarchy");
    }

    if (!scene.remove_node(empty)) {
        return TestResult::fail("EditorScene should remove existing nodes");
    }

    if (scene.find_node(empty) || scene.find_node(mesh)) {
        return TestResult::fail("removing a parent should remove descendants");
    }

    scene.create_empty("Temporary");
    scene.clear();
    if (!scene.tree().empty()) {
        return TestResult::fail("EditorScene clear should empty the tree");
    }

    editor::Editor editor;
    editor.clear_dirty();
    if (editor.dirty_flags() != editor::EditorDirtyFlags::None) {
        return TestResult::fail("Editor clear_dirty should clear all flags");
    }

    (void)editor.scene();
    if (!editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
        return TestResult::fail("mutable Editor::scene access should mark the editor scene-dirty");
    }

    editor.clear_dirty();
    editor.set_mode(editor::EditorMode::Mesh);
    if (editor.mode() != editor::EditorMode::Mesh ||
        !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Selection)) {
        return TestResult::fail("set_mode should update mode and mark selection dirty");
    }

    return TestResult::pass();
}

} // namespace locus::tests
