/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/DeleteNodeCommand.h"
#include "editor/command/scene/DuplicateNodeCommand.h"
#include "editor/command/scene/ReparentNodeCommand.h"
#include "editor/scene/NodeType.h"

#include <glm/vec3.hpp>

namespace {

[[nodiscard]] bool has_all_flags(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flags)
{
    return (mask & flags) == flags;
}

[[nodiscard]] bool has_scene_render_picking(locus::editor::EditorDirtyFlags mask)
{
    return has_all_flags(
        mask,
        locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking);
}

[[nodiscard]] bool has_delete_flags(locus::editor::EditorDirtyFlags mask)
{
    return has_all_flags(
        mask,
        locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Selection |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking);
}

} // namespace

namespace locus::tests {

TestResult run_node_hierarchy_command_tests()
{
    using namespace kernel::geometry;

    {
        editor::Editor editor;
        editor::CommandDispatcher dispatcher(editor);

        const editor::SceneNodeId parentA = editor.scene().create_empty("Parent A");
        const editor::SceneNodeId parentB = editor.scene().create_empty("Parent B");
        const editor::SceneNodeId child = editor.scene().create_empty("Child");
        const editor::SceneNodeId grandchild = editor.scene().create_empty("Grandchild");

        editor.scene().reparent(child, parentA);
        editor.scene().reparent(grandchild, child);

        editor::ReparentNodeCommand invalidReparent{ editor::SceneNodeId{}, parentB };
        if (invalidReparent.execute(dispatcher.context())) {
            return TestResult::fail("ReparentNodeCommand should reject invalid child ids");
        }

        editor::ReparentNodeCommand missingChild{ editor::SceneNodeId{ 999 }, parentB };
        if (missingChild.execute(dispatcher.context())) {
            return TestResult::fail("ReparentNodeCommand should reject missing children");
        }

        editor::ReparentNodeCommand selfParent{ child, child };
        if (selfParent.execute(dispatcher.context())) {
            return TestResult::fail("ReparentNodeCommand should reject self-parenting");
        }

        editor::ReparentNodeCommand missingParent{ child, editor::SceneNodeId{ 999 } };
        if (missingParent.execute(dispatcher.context())) {
            return TestResult::fail("ReparentNodeCommand should reject missing parents");
        }

        editor::ReparentNodeCommand descendantParent{ child, grandchild };
        if (descendantParent.execute(dispatcher.context())) {
            return TestResult::fail("ReparentNodeCommand should reject descendant parents");
        }

        editor::ReparentNodeCommand reparent{ child, parentB };
        if (reparent.name() != "Reparent Node" ||
            reparent.undo(dispatcher.context()) ||
            reparent.redo(dispatcher.context())) {
            return TestResult::fail("ReparentNodeCommand should expose name and reject undo/redo before execution");
        }

        editor.clear_dirty();
        const editor::CommandResult reparentResult = dispatcher.execute(reparent);
        const editor::SceneNode* childNode = editor.scene().find_node(child);
        if (!reparentResult ||
            !childNode ||
            childNode->parent() != parentB ||
            !has_scene_render_picking(reparentResult.dirtyFlags) ||
            !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
            return TestResult::fail("ReparentNodeCommand should move a child and report scene dirty flags");
        }

        if (!dispatcher.undo(reparent) ||
            editor.scene().find_node(child)->parent() != parentA) {
            return TestResult::fail("ReparentNodeCommand undo should restore the previous parent");
        }

        if (!dispatcher.redo(reparent) ||
            editor.scene().find_node(child)->parent() != parentB) {
            return TestResult::fail("ReparentNodeCommand redo should reapply the new parent");
        }

        editor::ReparentNodeCommand detachToRoot{ child, {} };
        if (!dispatcher.execute(detachToRoot) ||
            editor.scene().find_node(child)->parent().is_valid()) {
            return TestResult::fail("ReparentNodeCommand should support detaching a node to roots");
        }

        editor::ReparentNodeCommand unchangedParent{ child, {} };
        const editor::CommandResult unchangedResult = dispatcher.execute(unchangedParent);
        if (!unchangedResult ||
            unchangedResult.dirtyFlags != editor::EditorDirtyFlags::None) {
            return TestResult::fail("ReparentNodeCommand should be clean when the parent is unchanged");
        }
    }

    {
        editor::Editor editor;
        editor::CommandDispatcher dispatcher(editor);

        const editor::SceneNodeId root = editor.scene().create_empty("Root");
        const editor::SceneNodeId mesh = editor.scene().create_mesh("Mesh Child");
        const editor::SceneNodeId leaf = editor.scene().create_empty("Leaf");
        const editor::SceneNodeId survivor = editor.scene().create_empty("Survivor");

        editor.scene().reparent(mesh, root);
        editor.scene().reparent(leaf, mesh);

        editor::SceneNode* rootNode = editor.scene().find_node(root);
        rootNode->transform().set_position(glm::vec3{ 1.0f, 2.0f, 3.0f });
        rootNode->pivot().offset = glm::vec3{ 4.0f, 5.0f, 6.0f };
        rootNode->pivot().custom = true;
        rootNode->metadata().expanded = false;

        editor.selection().objects().set({ root, survivor }, root);
        editor.selection().objects().set_hovered(mesh);
        editor.selection_controller().set_active_mesh(mesh);
        editor.selection().mesh().add_vertex(VertexHandle{ 1 });

        editor::DeleteNodeCommand invalidDelete{ editor::SceneNodeId{} };
        if (invalidDelete.execute(dispatcher.context())) {
            return TestResult::fail("DeleteNodeCommand should reject invalid node ids");
        }

        editor::DeleteNodeCommand missingDelete{ editor::SceneNodeId{ 999 } };
        if (missingDelete.execute(dispatcher.context())) {
            return TestResult::fail("DeleteNodeCommand should reject missing nodes");
        }

        editor::DeleteNodeCommand deleteRoot{ root };
        if (deleteRoot.name() != "Delete Node" ||
            deleteRoot.undo(dispatcher.context()) ||
            deleteRoot.redo(dispatcher.context())) {
            return TestResult::fail("DeleteNodeCommand should expose name and reject undo/redo before execution");
        }

        editor.clear_dirty();
        const editor::CommandResult deleteResult = dispatcher.execute(deleteRoot);
        if (!deleteResult ||
            editor.scene().find_node(root) ||
            editor.scene().find_node(mesh) ||
            editor.scene().find_node(leaf) ||
            !editor.scene().find_node(survivor) ||
            editor.selection().objects().contains(root) ||
            editor.selection().objects().hovered().is_valid() ||
            editor.selection().mesh().active_mesh().is_valid() ||
            !has_delete_flags(deleteResult.dirtyFlags)) {
            return TestResult::fail("DeleteNodeCommand should delete a subtree and clean deleted selection state");
        }

        const editor::CommandResult deleteUndo = dispatcher.undo(deleteRoot);
        const editor::SceneNode* restoredRoot = editor.scene().find_node(root);
        const editor::SceneNode* restoredMesh = editor.scene().find_node(mesh);
        const editor::SceneNode* restoredLeaf = editor.scene().find_node(leaf);
        if (!deleteUndo ||
            !restoredRoot ||
            !restoredMesh ||
            !restoredLeaf ||
            restoredMesh->parent() != root ||
            restoredLeaf->parent() != mesh ||
            restoredRoot->metadata().name != "Root" ||
            restoredRoot->metadata().expanded ||
            restoredRoot->transform().position().x != 1.0f ||
            restoredRoot->transform().position().y != 2.0f ||
            restoredRoot->transform().position().z != 3.0f ||
            !restoredRoot->pivot().custom ||
            restoredRoot->pivot().offset.x != 4.0f ||
            !has_delete_flags(deleteUndo.dirtyFlags)) {
            return TestResult::fail("DeleteNodeCommand undo should restore the deleted subtree snapshot");
        }

        const editor::CommandResult deleteRedo = dispatcher.redo(deleteRoot);
        if (!deleteRedo ||
            editor.scene().find_node(root) ||
            editor.scene().find_node(mesh) ||
            editor.scene().find_node(leaf)) {
            return TestResult::fail("DeleteNodeCommand redo should delete the restored subtree again");
        }
    }

    {
        editor::Editor editor;
        editor::CommandDispatcher dispatcher(editor);

        const editor::SceneNodeId parent = editor.scene().create_empty("Parent");
        const editor::SceneNodeId root = editor.scene().create_empty("Root");
        const editor::SceneNodeId meshChild = editor.scene().create_mesh("Mesh Child");
        const editor::SceneNodeId leaf = editor.scene().create_empty("Leaf");

        editor.scene().reparent(root, parent);
        editor.scene().reparent(meshChild, root);
        editor.scene().reparent(leaf, meshChild);

        editor::SceneNode* rootNode = editor.scene().find_node(root);
        rootNode->transform().set_position(glm::vec3{ 7.0f, 8.0f, 9.0f });
        rootNode->metadata().locked = true;

        editor::DuplicateNodeCommand invalidDuplicate{ editor::SceneNodeId{} };
        if (invalidDuplicate.execute(dispatcher.context())) {
            return TestResult::fail("DuplicateNodeCommand should reject invalid node ids");
        }

        editor::DuplicateNodeCommand missingDuplicate{ editor::SceneNodeId{ 999 } };
        if (missingDuplicate.execute(dispatcher.context())) {
            return TestResult::fail("DuplicateNodeCommand should reject missing nodes");
        }

        editor::DuplicateNodeCommand duplicateRoot{ root };
        if (duplicateRoot.name() != "Duplicate Node" ||
            duplicateRoot.duplicated_node().is_valid() ||
            duplicateRoot.undo(dispatcher.context()) ||
            duplicateRoot.redo(dispatcher.context())) {
            return TestResult::fail("DuplicateNodeCommand should expose name/id and reject undo/redo before execution");
        }

        editor.clear_dirty();
        const editor::CommandResult duplicateResult = dispatcher.execute(duplicateRoot);
        const editor::SceneNodeId duplicateId = duplicateRoot.duplicated_node();
        const editor::SceneNode* duplicateNode = editor.scene().find_node(duplicateId);
        if (!duplicateResult ||
            !duplicateId.is_valid() ||
            !duplicateNode ||
            duplicateNode->parent() != parent ||
            duplicateNode->metadata().name != "Root Copy" ||
            !duplicateNode->metadata().locked ||
            duplicateNode->transform().position().x != 7.0f ||
            duplicateNode->children().size() != 1 ||
            !has_all_flags(
                duplicateResult.dirtyFlags,
                editor::EditorDirtyFlags::Scene |
                    editor::EditorDirtyFlags::Mesh |
                    editor::EditorDirtyFlags::Render |
                    editor::EditorDirtyFlags::Picking) ||
            !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
            return TestResult::fail("DuplicateNodeCommand should duplicate the root subtree beside the source");
        }

        const editor::SceneNodeId duplicateMeshId = duplicateNode->children()[0];
        const editor::SceneNode* duplicateMesh = editor.scene().find_node(duplicateMeshId);
        if (!duplicateMesh ||
            duplicateMesh->type() != editor::NodeType::Mesh ||
            duplicateMesh->metadata().name != "Mesh Child" ||
            duplicateMesh->parent() != duplicateId ||
            duplicateMesh->children().size() != 1 ||
            !editor.scene().find_mesh(duplicateMeshId)) {
            return TestResult::fail("DuplicateNodeCommand should duplicate child mesh nodes");
        }

        const editor::SceneNodeId duplicateLeafId = duplicateMesh->children()[0];
        const editor::SceneNode* duplicateLeaf = editor.scene().find_node(duplicateLeafId);
        if (!duplicateLeaf ||
            duplicateLeaf->metadata().name != "Leaf" ||
            duplicateLeaf->parent() != duplicateMeshId) {
            return TestResult::fail("DuplicateNodeCommand should duplicate deeper descendants");
        }

        const editor::CommandResult duplicateUndo = dispatcher.undo(duplicateRoot);
        if (!duplicateUndo ||
            editor.scene().find_node(duplicateId) ||
            editor.scene().find_node(duplicateMeshId) ||
            editor.scene().find_node(duplicateLeafId) ||
            !has_all_flags(
                duplicateUndo.dirtyFlags,
                editor::EditorDirtyFlags::Scene |
                    editor::EditorDirtyFlags::Selection |
                    editor::EditorDirtyFlags::Mesh |
                    editor::EditorDirtyFlags::Render |
                    editor::EditorDirtyFlags::Picking)) {
            return TestResult::fail("DuplicateNodeCommand undo should remove the duplicated subtree");
        }

        const editor::CommandResult duplicateRedo = dispatcher.redo(duplicateRoot);
        const editor::SceneNode* redoneDuplicate = editor.scene().find_node(duplicateId);
        if (!duplicateRedo ||
            !redoneDuplicate ||
            redoneDuplicate->children().empty() ||
            !editor.scene().find_node(duplicateMeshId) ||
            !editor.scene().find_node(duplicateLeafId)) {
            return TestResult::fail("DuplicateNodeCommand redo should restore the same duplicated ids");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
