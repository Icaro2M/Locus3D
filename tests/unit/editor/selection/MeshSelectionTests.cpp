/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SelectionTestSuite.h"

#include "editor/selection/MeshSelection.h"

namespace locus::tests {

TestResult run_mesh_selection_tests()
{
    using namespace kernel::geometry;

    editor::MeshSelection selection;
    const editor::SceneNodeId meshA{ 10 };
    const editor::SceneNodeId meshB{ 20 };

    const VertexHandle v0{ 0 };
    const VertexHandle v1{ 1 };
    const VertexHandle v2{ 5 };
    const EdgeHandle e0{ 2 };
    const LoopHandle l0{ 3 };
    const FaceHandle f0{ 4 };

    selection.set_active_mesh(meshA);
    if (selection.active_mesh() != meshA || !selection.empty()) {
        return TestResult::fail("set_active_mesh should set mesh and keep empty components");
    }

    selection.set_vertex(v0);
    selection.set_edge(e0);
    selection.set_loop(l0);
    selection.set_face(f0);

    if (!selection.vertices().contains(v0) ||
        !selection.edges().contains(e0) ||
        !selection.loops().contains(l0) ||
        !selection.faces().contains(f0)) {
        return TestResult::fail("single component setters should select valid handles");
    }

    if (!selection.add_vertex(v1) || selection.add_vertex(v1)) {
        return TestResult::fail("add_vertex should accept unique valid handles");
    }

    if (!selection.toggle_vertex(v2) || selection.toggle_vertex(v2)) {
        return TestResult::fail("toggle_vertex should add then remove handles");
    }

    selection.set_hovered_vertex(v0);
    selection.set_hovered_edge(e0);
    selection.set_hovered_loop(l0);
    selection.set_hovered_face(f0);
    selection.clear_hovered();

    if (selection.hovered_vertex().is_valid() ||
        selection.hovered_edge().is_valid() ||
        selection.hovered_loop().is_valid() ||
        selection.hovered_face().is_valid()) {
        return TestResult::fail("clear_hovered should reset every hovered component");
    }

    selection.set_active_mesh(meshB);
    if (selection.active_mesh() != meshB || !selection.empty()) {
        return TestResult::fail("changing active mesh should clear component selections");
    }

    selection.set_vertex(v0);
    selection.clear_components();
    if (!selection.empty() || selection.active_mesh() != meshB) {
        return TestResult::fail("clear_components should keep active mesh and clear components");
    }

    selection.clear();
    if (selection.active_mesh().is_valid() || !selection.empty()) {
        return TestResult::fail("clear should reset active mesh and selected components");
    }

    return TestResult::pass();
}

} // namespace locus::tests
