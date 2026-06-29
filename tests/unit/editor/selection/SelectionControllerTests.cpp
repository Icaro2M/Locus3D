/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SelectionTestSuite.h"

#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionController.h"

namespace locus::tests {

TestResult run_selection_controller_tests()
{
    using namespace kernel::geometry;

    editor::EditorScene scene;
    const editor::SceneNodeId root = scene.create_empty("Root");
    const editor::SceneNodeId mesh = scene.create_mesh("Mesh");
    const editor::SceneNodeId other = scene.create_empty("Other");
    const editor::SceneNodeId locked = scene.create_empty("Locked");

    scene.find_node(locked)->metadata().locked = true;

    editor::SelectionState state;
    state.clear_dirty();
    editor::SelectionController controller(scene, state);

    if (controller.select_object(editor::SceneNodeId{}) || controller.select_object(locked)) {
        return TestResult::fail("select_object should reject invalid or unselectable objects");
    }

    if (!controller.select_object(root) ||
        !state.objects().contains(root) ||
        state.objects().active() != root ||
        state.granularity() != editor::SelectionGranularity::Object ||
        state.scope() != editor::SelectionScope::Scene ||
        !state.is_dirty()) {
        return TestResult::fail("select_object should select valid scene objects in object mode");
    }

    state.clear_dirty();
    if (!controller.add_object(mesh) || !state.objects().contains(mesh)) {
        return TestResult::fail("add_object should add selectable objects");
    }

    if (!controller.toggle_object(other) || controller.toggle_object(other)) {
        return TestResult::fail("toggle_object should add then remove selectable objects");
    }

    if (controller.set_active_mesh(root)) {
        return TestResult::fail("set_active_mesh should reject non-mesh scene nodes");
    }

    if (!controller.set_active_mesh(mesh) ||
        state.mesh().active_mesh() != mesh ||
        state.objects().active() != mesh ||
        state.scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("set_active_mesh should accept selectable mesh nodes");
    }

    const VertexHandle vertex{ 1 };
    const EdgeHandle edge{ 2 };
    const LoopHandle loop{ 3 };
    const FaceHandle face{ 4 };

    if (!controller.select_vertex(vertex) ||
        state.granularity() != editor::SelectionGranularity::Vertex ||
        !state.mesh().vertices().contains(vertex)) {
        return TestResult::fail("select_vertex should enter vertex selection mode");
    }

    if (!controller.select_edge(edge) ||
        state.granularity() != editor::SelectionGranularity::Edge ||
        !state.mesh().edges().contains(edge)) {
        return TestResult::fail("select_edge should enter edge selection mode");
    }

    if (!controller.select_loop(loop) ||
        state.granularity() != editor::SelectionGranularity::Loop ||
        !state.mesh().loops().contains(loop)) {
        return TestResult::fail("select_loop should enter loop selection mode");
    }

    if (!controller.select_face(face) ||
        state.granularity() != editor::SelectionGranularity::Face ||
        !state.mesh().faces().contains(face)) {
        return TestResult::fail("select_face should enter face selection mode");
    }

    controller.clear_mesh_components();
    if (!state.mesh().empty()) {
        return TestResult::fail("clear_mesh_components should clear component selections");
    }

    controller.clear_objects();
    if (!state.objects().empty() || state.objects().active().is_valid()) {
        return TestResult::fail("clear_objects should reset object selection");
    }

    return TestResult::pass();
}

} // namespace locus::tests
