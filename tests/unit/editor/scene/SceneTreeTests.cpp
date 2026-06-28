/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorSceneTestSuite.h"

#include "editor/scene/EmptyNode.h"
#include "editor/scene/SceneTree.h"

namespace locus::tests {

TestResult run_scene_tree_tests()
{
    editor::SceneTree tree;

    const editor::SceneNodeId root = tree.create_node<editor::EmptyNode>("Root");
    const editor::SceneNodeId child = tree.create_node<editor::EmptyNode>("Child");
    const editor::SceneNodeId grandchild = tree.create_node<editor::EmptyNode>("Grandchild");

    if (!root.is_valid() || !child.is_valid() || !grandchild.is_valid()) {
        return TestResult::fail("created node ids should be valid");
    }

    if (tree.size() != 3 || tree.roots().size() != 3) {
        return TestResult::fail("created nodes should start as roots");
    }

    if (!tree.reparent(child, root) || !tree.reparent(grandchild, child)) {
        return TestResult::fail("reparent should build a valid hierarchy");
    }

    const editor::SceneNode* rootNode = tree.find_node(root);
    const editor::SceneNode* childNode = tree.find_node(child);
    const editor::SceneNode* grandchildNode = tree.find_node(grandchild);

    if (!rootNode || !childNode || !grandchildNode) {
        return TestResult::fail("find_node should return stored nodes");
    }

    if (tree.roots().size() != 1 || tree.roots()[0] != root) {
        return TestResult::fail("reparented children should leave the root list");
    }

    if (childNode->parent() != root || grandchildNode->parent() != child) {
        return TestResult::fail("reparent should update parent ids");
    }

    if (!tree.is_ancestor(root, grandchild)) {
        return TestResult::fail("is_ancestor should detect indirect ancestors");
    }

    if (tree.reparent(root, grandchild)) {
        return TestResult::fail("reparent should reject cycles");
    }

    if (tree.reparent(root, root)) {
        return TestResult::fail("reparent should reject self-parenting");
    }

    if (!tree.reparent(child, {})) {
        return TestResult::fail("reparent to an invalid parent should make a root");
    }

    if (childNode->parent().is_valid() || tree.roots().size() != 2) {
        return TestResult::fail("reparent to invalid parent should detach to roots");
    }

    if (!tree.remove_node(child)) {
        return TestResult::fail("remove_node should remove existing nodes");
    }

    if (tree.contains(child) || tree.contains(grandchild)) {
        return TestResult::fail("remove_node should remove descendants");
    }

    tree.clear();
    if (!tree.empty()) {
        return TestResult::fail("clear should remove all nodes");
    }

    const editor::SceneNodeId resetId = tree.create_node<editor::EmptyNode>("After clear");
    if (resetId.value != 1u) {
        return TestResult::fail("clear should reset scene node id allocation");
    }

    return TestResult::pass();
}

} // namespace locus::tests
