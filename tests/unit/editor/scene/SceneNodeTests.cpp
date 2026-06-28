/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorSceneTestSuite.h"

#include "editor/scene/EmptyNode.h"
#include "editor/scene/NodeType.h"

namespace locus::tests {

TestResult run_scene_node_tests()
{
    editor::EmptyNode node{ editor::SceneNodeId{ 7 }, "Group" };

    if (node.id() != editor::SceneNodeId{ 7 }) {
        return TestResult::fail("node should keep its construction id");
    }

    if (node.type() != editor::NodeType::Empty) {
        return TestResult::fail("empty node should report empty node type");
    }

    if (node.metadata().name != "Group") {
        return TestResult::fail("node metadata should keep its construction name");
    }

    if (node.has_parent()) {
        return TestResult::fail("newly constructed node should not have a parent");
    }

    if (!node.is_visible() || !node.is_selectable()) {
        return TestResult::fail("default metadata should make nodes selectable");
    }

    node.metadata().locked = true;
    if (node.is_selectable()) {
        return TestResult::fail("locked nodes should not be selectable");
    }

    node.metadata().locked = false;
    node.metadata().visible = false;
    if (node.is_visible() || node.is_selectable()) {
        return TestResult::fail("hidden nodes should not be selectable");
    }

    node.clear_dirty();
    if (node.dirty_flags() != editor::EditorDirtyFlags::None) {
        return TestResult::fail("clear_dirty should clear all flags by default");
    }

    node.metadata().expanded = false;
    if (!editor::has_flag(node.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
        return TestResult::fail("mutable metadata access should mark the node scene-dirty");
    }

    return TestResult::pass();
}

} // namespace locus::tests
