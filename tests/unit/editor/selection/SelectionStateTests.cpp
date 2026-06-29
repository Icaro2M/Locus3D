/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SelectionTestSuite.h"

#include "editor/selection/SelectionSerializer.h"

namespace locus::tests {

TestResult run_selection_state_tests()
{
    using namespace kernel::geometry;

    editor::SelectionState state;
    if (!state.is_dirty() ||
        state.granularity() != editor::SelectionGranularity::Object ||
        state.scope() != editor::SelectionScope::Scene) {
        return TestResult::fail("SelectionState should start dirty in object scene scope");
    }

    state.clear_dirty();
    state.set_granularity(editor::SelectionGranularity::Vertex);
    if (!state.is_dirty() ||
        state.granularity() != editor::SelectionGranularity::Vertex ||
        state.scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("mesh granularity should switch scope to active mesh");
    }

    state.clear_dirty();
    state.set_scope(editor::SelectionScope::Scene);
    if (!state.is_dirty() ||
        state.scope() != editor::SelectionScope::Scene ||
        state.granularity() != editor::SelectionGranularity::Object) {
        return TestResult::fail("scene scope should force object granularity");
    }

    state.objects().set({ editor::SceneNodeId{ 1 }, editor::SceneNodeId{ 2 } }, editor::SceneNodeId{ 1 });
    state.objects().set_hovered(editor::SceneNodeId{ 2 });
    state.mesh().set_active_mesh(editor::SceneNodeId{ 2 });
    state.mesh().add_vertex(VertexHandle{ 7 });
    state.mesh().add_edge(EdgeHandle{ 8 });
    state.mesh().set_hovered_face(FaceHandle{ 9 });
    state.set_granularity(editor::SelectionGranularity::Face);

    const editor::SelectionSnapshot snapshot = editor::SelectionSerializer::capture(state);

    editor::SelectionState restored;
    restored.clear_dirty();
    editor::SelectionSerializer::restore(snapshot, restored);

    if (!restored.is_dirty() ||
        restored.granularity() != editor::SelectionGranularity::Face ||
        restored.scope() != editor::SelectionScope::ActiveMesh) {
        return TestResult::fail("restore should preserve granularity, scope, and dirty state");
    }

    if (restored.objects().active() != editor::SceneNodeId{ 1 } ||
        restored.objects().hovered() != editor::SceneNodeId{ 2 } ||
        restored.mesh().active_mesh() != editor::SceneNodeId{ 2 }) {
        return TestResult::fail("restore should preserve object and active mesh state");
    }

    if (!restored.mesh().vertices().contains(VertexHandle{ 7 }) ||
        !restored.mesh().edges().contains(EdgeHandle{ 8 }) ||
        restored.mesh().hovered_face() != FaceHandle{ 9 }) {
        return TestResult::fail("restore should preserve mesh component selections");
    }

    restored.clear();
    if (!restored.objects().empty() ||
        !restored.mesh().empty() ||
        restored.mesh().active_mesh().is_valid() ||
        restored.granularity() != editor::SelectionGranularity::Object ||
        restored.scope() != editor::SelectionScope::Scene) {
        return TestResult::fail("clear should reset all selection state");
    }

    return TestResult::pass();
}

} // namespace locus::tests
