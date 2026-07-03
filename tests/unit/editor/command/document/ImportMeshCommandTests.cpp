/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DocumentCommandTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/document/ImportMeshCommand.h"

#include <glm/vec3.hpp>

namespace {

[[nodiscard]] locus::kernel::geometry::LEM make_import_mesh()
{
    using namespace locus::kernel::geometry;

    LEM mesh;
    const VertexHandle v0 = mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    mesh.add_face({ v0, v1, v2 });
    return mesh;
}

} // namespace

namespace locus::tests {

TestResult run_import_mesh_command_tests()
{
    const kernel::geometry::LEM mesh = make_import_mesh();

    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);
    const editor::SceneNodeId parent = editor.scene().create_empty("Parent");

    editor::ImportMeshCommand fallbackName{ mesh, "", parent };
    if (fallbackName.name() != "Import Mesh" ||
        fallbackName.imported_node().is_valid()) {
        return TestResult::fail("ImportMeshCommand should expose name and invalid id before execution");
    }

    editor::ImportMeshCommand missingParent{ mesh, "Missing Parent", editor::SceneNodeId{ 999 } };
    if (missingParent.execute(dispatcher.context())) {
        return TestResult::fail("ImportMeshCommand should reject a missing parent node");
    }

    editor::ImportMeshCommand notExecuted{ mesh, "Mesh" };
    if (notExecuted.undo(dispatcher.context()) ||
        notExecuted.redo(dispatcher.context())) {
        return TestResult::fail("ImportMeshCommand undo/redo should fail before execution");
    }

    const editor::CommandResult fallbackResult = dispatcher.execute(fallbackName);
    const editor::SceneNodeId fallbackId = fallbackName.imported_node();
    const editor::MeshNode* fallbackNode = editor.scene().find_mesh(fallbackId);
    if (!fallbackResult ||
        !fallbackId.is_valid() ||
        !fallbackNode ||
        fallbackNode->metadata().name != "Imported Mesh" ||
        fallbackNode->parent() != parent ||
        fallbackNode->mesh().vertex_count() != 3u ||
        fallbackNode->mesh().face_count() != 1u ||
        !editor.selection().objects().contains(fallbackId) ||
        editor.selection().objects().active() != fallbackId ||
        editor.selection().mesh().active_mesh() != fallbackId ||
        editor.selection().granularity() != editor::SelectionGranularity::Object ||
        editor.selection().scope() != editor::SelectionScope::Scene) {
        return TestResult::fail("ImportMeshCommand should create, parent and select the imported mesh node");
    }

    if (!dispatcher.undo(fallbackName) ||
        editor.scene().find_node(fallbackId) ||
        editor.selection().objects().contains(fallbackId) ||
        editor.selection().mesh().active_mesh() == fallbackId) {
        return TestResult::fail("ImportMeshCommand undo should remove the node and clean related selection");
    }

    if (!dispatcher.redo(fallbackName) ||
        fallbackName.imported_node() != fallbackId ||
        !editor.scene().find_mesh(fallbackId) ||
        editor.scene().find_node(fallbackId)->parent() != parent) {
        return TestResult::fail("ImportMeshCommand redo should restore the same imported node id");
    }

    if (fallbackName.redo(dispatcher.context())) {
        return TestResult::fail("ImportMeshCommand redo should fail while the imported node already exists");
    }

    const editor::CommandResult executeAgainResult = dispatcher.execute(fallbackName);
    if (executeAgainResult) {
        return TestResult::fail("ImportMeshCommand execute after first run should fail while node already exists");
    }

    editor::ImportMeshCommand keepSelection{ mesh, "Unselected Mesh", {}, false };
    editor.selection().objects().set(parent);
    const editor::CommandResult keepSelectionResult = dispatcher.execute(keepSelection);
    const editor::SceneNodeId unselectedId = keepSelection.imported_node();
    if (!keepSelectionResult ||
        !editor.scene().find_mesh(unselectedId) ||
        editor.selection().objects().active() != parent ||
        editor.selection().objects().contains(unselectedId) ||
        editor.selection().mesh().active_mesh() == unselectedId) {
        return TestResult::fail("ImportMeshCommand should optionally leave selection unchanged");
    }

    editor.scene().find_node(unselectedId)->metadata().name = "Renamed Import";
    editor.scene().find_node(unselectedId)->transform().set_position(glm::vec3{ 4.0f, 5.0f, 6.0f });
    if (!dispatcher.undo(keepSelection) ||
        !dispatcher.redo(keepSelection)) {
        return TestResult::fail("ImportMeshCommand should undo and redo unselected imports");
    }

    const editor::SceneNode* restoredUnselected = editor.scene().find_node(unselectedId);
    if (!restoredUnselected ||
        restoredUnselected->metadata().name != "Unselected Mesh" ||
        restoredUnselected->transform().position() != glm::vec3{ 0.0f, 0.0f, 0.0f }) {
        return TestResult::fail("ImportMeshCommand redo should restore the first captured imported node state");
    }

    return TestResult::pass();
}

} // namespace locus::tests
