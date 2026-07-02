/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/selection/ClearMeshSelectionCommand.h"
#include "editor/command/selection/MeshSelectionSnapshot.h"
#include "editor/command/selection/SelectMeshComponentCommand.h"
#include "editor/command/selection/ToggleMeshComponentSelectionCommand.h"

namespace {

[[nodiscard]] bool has_all_flags(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flags)
{
    return (mask & flags) == flags;
}

[[nodiscard]] bool has_selection_dirty_flags(locus::editor::CommandResult result)
{
    return has_all_flags(
        result.dirtyFlags,
        locus::editor::EditorDirtyFlags::Selection |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking);
}

} // namespace

namespace locus::tests {

TestResult run_mesh_selection_command_tests()
{
    using namespace kernel::geometry;

    const VertexHandle v0{ 1 };
    const VertexHandle v1{ 2 };
    const EdgeHandle e0{ 3 };
    const EdgeHandle e1{ 4 };
    const LoopHandle l0{ 5 };
    const LoopHandle l1{ 6 };
    const FaceHandle f0{ 7 };
    const FaceHandle f1{ 8 };

    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    const editor::SceneNodeId empty = editor.scene().create_empty("Empty");
    const editor::SceneNodeId mesh = editor.scene().create_mesh("Mesh");

    if (!editor.selection_controller().set_active_mesh(mesh)) {
        return TestResult::fail("test setup should accept a mesh node as active mesh");
    }

    editor.selection().mesh().add_vertex(v0);
    editor.selection().mesh().add_edge(e0);
    editor.selection().mesh().add_loop(l0);
    editor.selection().mesh().add_face(f0);
    editor.selection().mesh().set_hovered_vertex(v1);
    editor.selection().mesh().set_hovered_edge(e1);
    editor.selection().mesh().set_hovered_loop(l1);
    editor.selection().mesh().set_hovered_face(f1);
    editor.selection().set_granularity(editor::SelectionGranularity::Face);
    editor.selection().set_scope(editor::SelectionScope::ActiveMesh);

    editor::MeshSelectionSnapshot snapshot;
    if (snapshot.is_valid()) {
        return TestResult::fail("MeshSelectionSnapshot should start invalid");
    }

    snapshot.capture(editor.selection());
    editor.selection().mesh().clear();
    editor.selection().set_granularity(editor::SelectionGranularity::Object);
    editor.selection().set_scope(editor::SelectionScope::Scene);
    snapshot.restore(editor.selection());

    if (!snapshot.is_valid() ||
        editor.selection().mesh().active_mesh() != mesh ||
        !editor.selection().mesh().vertices().contains(v0) ||
        !editor.selection().mesh().edges().contains(e0) ||
        !editor.selection().mesh().loops().contains(l0) ||
        !editor.selection().mesh().faces().contains(f0) ||
        editor.selection().mesh().hovered_vertex() != v1 ||
        editor.selection().mesh().hovered_edge() != e1 ||
        editor.selection().mesh().hovered_loop() != l1 ||
        editor.selection().mesh().hovered_face() != f1 ||
        editor.selection().granularity() != editor::SelectionGranularity::Face ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("MeshSelectionSnapshot should capture and restore mesh selection state");
    }

    editor::SelectMeshComponentCommand invalidSelect{ VertexHandle{} };
    if (invalidSelect.execute(dispatcher.context())) {
        return TestResult::fail("SelectMeshComponentCommand should reject invalid handles");
    }

    editor.selection().mesh().set_active_mesh(empty);
    editor::SelectMeshComponentCommand selectWithoutMesh{ v0 };
    if (selectWithoutMesh.execute(dispatcher.context())) {
        return TestResult::fail("SelectMeshComponentCommand should require a valid active mesh");
    }

    editor.selection_controller().set_active_mesh(mesh);
    editor.selection().mesh().add_vertex(v0);
    editor.selection().mesh().add_edge(e0);
    editor.selection().mesh().add_loop(l0);
    editor.selection().mesh().add_face(f0);
    editor.selection().mesh().set_hovered_edge(e1);

    editor::SelectMeshComponentCommand selectVertex{ v1 };
    if (selectVertex.name() != "Select Mesh Component") {
        return TestResult::fail("SelectMeshComponentCommand should expose a stable command name");
    }

    editor.clear_dirty();
    const editor::CommandResult selectVertexResult = dispatcher.execute(selectVertex);
    if (!selectVertexResult ||
        !has_selection_dirty_flags(selectVertexResult) ||
        !editor.selection().mesh().vertices().contains(v1) ||
        editor.selection().mesh().vertices().contains(v0) ||
        !editor.selection().mesh().edges().empty() ||
        !editor.selection().mesh().loops().empty() ||
        !editor.selection().mesh().faces().empty() ||
        editor.selection().granularity() != editor::SelectionGranularity::Vertex ||
        editor.selection().scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("SelectMeshComponentCommand should select one vertex and clear other components");
    }

    const editor::CommandResult selectVertexUndo = dispatcher.undo(selectVertex);
    if (!selectVertexUndo ||
        !editor.selection().mesh().vertices().contains(v0) ||
        !editor.selection().mesh().edges().contains(e0) ||
        !editor.selection().mesh().loops().contains(l0) ||
        !editor.selection().mesh().faces().contains(f0) ||
        editor.selection().mesh().hovered_edge() != e1) {
        return TestResult::fail("SelectMeshComponentCommand undo should restore previous mesh selection");
    }

    editor::SelectMeshComponentCommand notExecutedSelect{ e0 };
    if (notExecutedSelect.undo(dispatcher.context())) {
        return TestResult::fail("SelectMeshComponentCommand undo should fail before execution");
    }

    editor::SelectMeshComponentCommand selectEdge{ e1 };
    editor::SelectMeshComponentCommand selectLoop{ l1 };
    editor::SelectMeshComponentCommand selectFace{ f1 };
    if (!dispatcher.execute(selectEdge) ||
        editor.selection().granularity() != editor::SelectionGranularity::Edge ||
        !editor.selection().mesh().edges().contains(e1) ||
        !dispatcher.execute(selectLoop) ||
        editor.selection().granularity() != editor::SelectionGranularity::Loop ||
        !editor.selection().mesh().loops().contains(l1) ||
        !dispatcher.execute(selectFace) ||
        editor.selection().granularity() != editor::SelectionGranularity::Face ||
        !editor.selection().mesh().faces().contains(f1)) {
        return TestResult::fail("SelectMeshComponentCommand should support edge, loop and face handles");
    }

    editor.selection().mesh().add_vertex(v0);
    editor.selection().mesh().add_edge(e0);
    editor.selection().mesh().add_loop(l0);
    editor.selection().mesh().add_face(f0);
    editor.selection().mesh().set_hovered_vertex(v1);

    editor::ClearMeshSelectionCommand clearSelection;
    if (clearSelection.name() != "Clear Mesh Selection") {
        return TestResult::fail("ClearMeshSelectionCommand should expose a stable command name");
    }

    const editor::CommandResult clearResult = dispatcher.execute(clearSelection);
    if (!clearResult ||
        !has_selection_dirty_flags(clearResult) ||
        editor.selection().mesh().active_mesh() != mesh ||
        !editor.selection().mesh().empty() ||
        editor.selection().mesh().hovered_vertex().is_valid()) {
        return TestResult::fail("ClearMeshSelectionCommand should clear components and keep active mesh");
    }

    const editor::CommandResult clearUndo = dispatcher.undo(clearSelection);
    if (!clearUndo ||
        !editor.selection().mesh().vertices().contains(v0) ||
        !editor.selection().mesh().edges().contains(e0) ||
        !editor.selection().mesh().loops().contains(l0) ||
        !editor.selection().mesh().faces().contains(f0) ||
        editor.selection().mesh().hovered_vertex() != v1) {
        return TestResult::fail("ClearMeshSelectionCommand undo should restore previous mesh selection");
    }

    editor::ClearMeshSelectionCommand notExecutedClear;
    if (notExecutedClear.undo(dispatcher.context())) {
        return TestResult::fail("ClearMeshSelectionCommand undo should fail before execution");
    }

    editor::ToggleMeshComponentSelectionCommand invalidToggle{ FaceHandle{} };
    if (invalidToggle.execute(dispatcher.context())) {
        return TestResult::fail("ToggleMeshComponentSelectionCommand should reject invalid handles");
    }

    editor.selection().mesh().set_active_mesh(empty);
    editor::ToggleMeshComponentSelectionCommand toggleWithoutMesh{ v0 };
    if (toggleWithoutMesh.execute(dispatcher.context())) {
        return TestResult::fail("ToggleMeshComponentSelectionCommand should require a valid active mesh");
    }

    editor.selection_controller().set_active_mesh(mesh);
    editor.selection().mesh().clear_components();

    editor::ToggleMeshComponentSelectionCommand toggleVertex{ v0 };
    if (toggleVertex.name() != "Toggle Mesh Component Selection") {
        return TestResult::fail("ToggleMeshComponentSelectionCommand should expose a stable command name");
    }

    const editor::CommandResult toggleVertexResult = dispatcher.execute(toggleVertex);
    if (!toggleVertexResult ||
        !has_selection_dirty_flags(toggleVertexResult) ||
        !editor.selection().mesh().vertices().contains(v0) ||
        editor.selection().granularity() != editor::SelectionGranularity::Vertex) {
        return TestResult::fail("ToggleMeshComponentSelectionCommand should add an unselected vertex");
    }

    const editor::CommandResult toggleVertexUndo = dispatcher.undo(toggleVertex);
    if (!toggleVertexUndo || editor.selection().mesh().vertices().contains(v0)) {
        return TestResult::fail("ToggleMeshComponentSelectionCommand undo should restore previous mesh selection");
    }

    editor.selection().mesh().add_vertex(v0);
    editor::ToggleMeshComponentSelectionCommand removeVertex{ v0 };
    const editor::CommandResult removeVertexResult = dispatcher.execute(removeVertex);
    if (!removeVertexResult || editor.selection().mesh().vertices().contains(v0)) {
        return TestResult::fail("ToggleMeshComponentSelectionCommand should remove a selected vertex");
    }

    editor::ToggleMeshComponentSelectionCommand notExecutedToggle{ v0 };
    if (notExecutedToggle.undo(dispatcher.context())) {
        return TestResult::fail("ToggleMeshComponentSelectionCommand undo should fail before execution");
    }

    editor::ToggleMeshComponentSelectionCommand toggleEdge{ e0 };
    editor::ToggleMeshComponentSelectionCommand toggleLoop{ l0 };
    editor::ToggleMeshComponentSelectionCommand toggleFace{ f0 };
    if (!dispatcher.execute(toggleEdge) ||
        editor.selection().granularity() != editor::SelectionGranularity::Edge ||
        !editor.selection().mesh().edges().contains(e0) ||
        !dispatcher.execute(toggleLoop) ||
        editor.selection().granularity() != editor::SelectionGranularity::Loop ||
        !editor.selection().mesh().loops().contains(l0) ||
        !dispatcher.execute(toggleFace) ||
        editor.selection().granularity() != editor::SelectionGranularity::Face ||
        !editor.selection().mesh().faces().contains(f0)) {
        return TestResult::fail("ToggleMeshComponentSelectionCommand should support edge, loop and face handles");
    }

    return TestResult::pass();
}

} // namespace locus::tests
