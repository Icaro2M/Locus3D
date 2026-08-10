/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/DeleteNodesCommand.h"
#include "editor/command/scene/PasteNodesCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/io/SceneFragmentSerializer.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionSerializer.h"

#include <glm/vec3.hpp>

#include <memory>
#include <string>
#include <vector>

namespace {

[[nodiscard]] locus::editor::SceneNodeId create_mesh_node(
    locus::editor::Editor& editor,
    const char* name)
{
    using namespace locus::kernel::geometry;

    const locus::editor::SceneNodeId id =
        editor.scene().create_mesh(name);
    auto* node = editor.scene().find_mesh(id);
    if (!node) {
        return {};
    }

    LEM& mesh = node->mesh();
    const VertexHandle a = mesh.add_vertex({ 0.0f, 0.0f, 0.0f });
    const VertexHandle b = mesh.add_vertex({ 1.0f, 0.0f, 0.0f });
    const VertexHandle c = mesh.add_vertex({ 0.0f, 1.0f, 0.0f });
    (void)mesh.add_face({ a, b, c });
    node->bump_mesh_revision();

    return id;
}

[[nodiscard]] bool paste_fragment(
    locus::editor::Editor& editor,
    locus::editor::HistoryStack& history,
    const locus::editor::SceneFragment& fragment)
{
    locus::editor::CommandDispatcher dispatcher(editor);
    locus::editor::CommandResult result =
        history.execute(
            dispatcher,
            std::make_unique<locus::editor::PasteNodesCommand>(
                fragment));
    return result.success;
}

} // namespace

namespace locus::tests {

TestResult run_clipboard_command_tests()
{
    {
        editor::Editor editor;
        editor::HistoryStack history;

        const editor::SceneNodeId cube =
            create_mesh_node(editor, "Cube");
        editor.scene().find_node(cube)->transform().set_position(
            { 2.0f, 3.0f, 4.0f });
        editor.selection_controller().select_object(cube);
        editor.clear_dirty();

        const editor::Editor& readOnlyEditor = editor;
        const std::size_t nodeCountBefore =
            readOnlyEditor.scene().tree().size();
        const std::size_t undoSizeBefore = history.undo_size();
        const editor::SelectionSnapshot selectionBefore =
            editor::SelectionSerializer::capture(
                readOnlyEditor.selection());

        editor::SceneFragmentResult fragment =
            editor::capture_scene_fragment(
                readOnlyEditor.scene(),
                readOnlyEditor.selection().objects().selected());

        if (!fragment.success) {
            return TestResult::fail("Copy should capture selected object");
        }

        if (readOnlyEditor.scene().tree().size() != nodeCountBefore
            || history.undo_size() != undoSizeBefore
            || editor.dirty_flags() != editor::EditorDirtyFlags::None
            || readOnlyEditor.selection().objects().selected()
                != selectionBefore.objects.selected) {
            return TestResult::fail("Copy should not mutate scene, history, dirty flags, or selection");
        }

        if (!paste_fragment(editor, history, fragment.fragment)) {
            return TestResult::fail("Paste should execute captured fragment");
        }

        if (editor.scene().tree().size() != nodeCountBefore + 1u
            || history.undo_size() != undoSizeBefore + 1u) {
            return TestResult::fail("Paste should add one node as one history entry");
        }

        const editor::SceneNodeId pasted =
            editor.selection().objects().active();
        if (pasted == cube || pasted.is_invalid()) {
            return TestResult::fail("Paste should select a newly allocated node id");
        }

        const auto* originalMesh = editor.scene().find_mesh(cube);
        const auto* pastedMesh = editor.scene().find_mesh(pasted);
        if (!originalMesh || !pastedMesh
            || originalMesh->mesh().vertex_count()
                != pastedMesh->mesh().vertex_count()
            || pastedMesh->transform().position()
                != originalMesh->transform().position()) {
            return TestResult::fail("Paste should preserve mesh and transform on a distinct node");
        }
    }

    {
        editor::Editor editor;
        editor::HistoryStack history;
        editor::CommandDispatcher dispatcher(editor);

        const editor::SceneNodeId cube =
            create_mesh_node(editor, "Cube");
        editor.selection_controller().select_object(cube);

        const editor::Editor& readOnlyEditor = editor;
        editor::SceneFragmentResult fragment =
            editor::capture_scene_fragment(
                readOnlyEditor.scene(),
                readOnlyEditor.selection().objects().selected());

        editor::DeleteNodesCommand deleteCube({ cube });
        if (!history.execute(
                dispatcher,
                std::make_unique<editor::DeleteNodesCommand>(
                    std::vector<editor::SceneNodeId>{ cube })).success) {
            return TestResult::fail("Delete setup should succeed");
        }

        if (!paste_fragment(editor, history, fragment.fragment)) {
            return TestResult::fail("Detached paste should survive deleting original");
        }

        if (editor.scene().tree().size() != 1u) {
            return TestResult::fail("Detached paste should recreate the copied node");
        }
    }

    {
        editor::Editor editor;
        editor::HistoryStack history;

        const editor::SceneNodeId parent =
            editor.scene().create_empty("Parent");
        const editor::SceneNodeId childA =
            create_mesh_node(editor, "Child A");
        const editor::SceneNodeId childB =
            create_mesh_node(editor, "Child B");
        editor.scene().reparent(childA, parent);
        editor.scene().reparent(childB, parent);
        editor.selection().objects().set({ parent, childA });

        const editor::Editor& readOnlyEditor = editor;
        editor::SceneFragmentResult fragment =
            editor::capture_scene_fragment(
                readOnlyEditor.scene(),
                readOnlyEditor.selection().objects().selected());

        if (!fragment.success || fragment.fragment.nodes.size() != 3u) {
            return TestResult::fail("Parent plus selected child should serialize the subtree once");
        }

        if (!paste_fragment(editor, history, fragment.fragment)) {
            return TestResult::fail("Hierarchy paste should execute");
        }

        if (editor.scene().tree().size() != 6u
            || editor.selection().objects().selected().size() != 3u) {
            return TestResult::fail("Hierarchy paste should create exactly one copied subtree");
        }

        const editor::SceneNodeId pastedRoot =
            editor.selection().objects().selected().front();
        const editor::SceneNode* pastedParent =
            editor.scene().find_node(pastedRoot);
        if (!pastedParent || pastedParent->children().size() != 2u) {
            return TestResult::fail("Hierarchy paste should preserve children");
        }

        editor::CommandDispatcher dispatcher(editor);
        if (!history.undo(dispatcher).success
            || editor.scene().tree().size() != 3u) {
            return TestResult::fail("Undo paste should remove the whole pasted subtree");
        }

        if (!history.redo(dispatcher).success
            || editor.scene().tree().size() != 6u) {
            return TestResult::fail("Redo paste should restore the whole pasted subtree");
        }
    }

    {
        if (editor::deserialize_scene_fragment("hello world").success) {
            return TestResult::fail("Invalid clipboard text should be rejected");
        }

        const std::string unsupportedVersion =
            "{\"magic\":\"LOCUS3D_SCENE_FRAGMENT\",\"version\":999,\"nodes\":[]}";
        if (editor::deserialize_scene_fragment(unsupportedVersion).success) {
            return TestResult::fail("Unsupported fragment version should be rejected");
        }

        const std::string invalidParent =
            "{\"magic\":\"LOCUS3D_SCENE_FRAGMENT\",\"version\":1,\"nodes\":["
            "{\"id\":0,\"parent\":99,\"type\":\"empty\","
            "\"metadata\":{\"name\":\"Bad\"},"
            "\"transform\":{\"position\":[0,0,0],\"rotation\":[1,0,0,0],\"scale\":[1,1,1]},"
            "\"pivot\":{\"offset\":[0,0,0],\"custom\":false}}]}";
        if (editor::deserialize_scene_fragment(invalidParent).success) {
            return TestResult::fail("Invalid parent reference should be rejected before paste");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
