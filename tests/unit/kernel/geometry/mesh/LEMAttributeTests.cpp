/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

namespace locus::tests {

TestResult run_lem_attribute_tests()
{
    using namespace kernel::geometry;

    LEM mesh;
    LEMEditor editor(mesh);
    const QuadMesh quad = make_quad(editor);
    const EdgeHandle edge = mesh.find_edge(quad.v0, quad.v1);

    if (!edge.is_valid()) {
        return TestResult::fail("test quad should expose a boundary edge");
    }

    editor.clear_diff();

    if (!editor.set_selected(quad.v0, true) ||
        !editor.set_selected(edge, true) ||
        !editor.set_selected(quad.face, true)) {
        return TestResult::fail("set_selected should accept active elements");
    }

    if (!mesh.vertex(quad.v0).selected ||
        !mesh.edge(edge).selected ||
        !mesh.face(quad.face).selected) {
        return TestResult::fail("set_selected should update element selection flags");
    }

    editor.clear_selection();
    if (mesh.vertex(quad.v0).selected ||
        mesh.edge(edge).selected ||
        mesh.face(quad.face).selected) {
        return TestResult::fail("clear_selection should clear selected flags");
    }

    if (!editor.set_hidden(quad.v1, true) ||
        !editor.set_hidden(edge, true) ||
        !editor.set_hidden(quad.face, true)) {
        return TestResult::fail("set_hidden should accept active elements");
    }

    if (!mesh.vertex(quad.v1).hidden ||
        !mesh.edge(edge).hidden ||
        !mesh.face(quad.face).hidden) {
        return TestResult::fail("set_hidden should update element hidden flags");
    }

    editor.clear_visibility();
    if (mesh.vertex(quad.v1).hidden ||
        mesh.edge(edge).hidden ||
        mesh.face(quad.face).hidden) {
        return TestResult::fail("clear_visibility should clear hidden flags");
    }

    if (!editor.set_smooth(edge, true) || !editor.set_crease(edge, 2.0f)) {
        return TestResult::fail("edge shading attributes should accept active edges");
    }

    if (!mesh.edge(edge).smooth || mesh.edge(edge).crease != 1.0f) {
        return TestResult::fail("edge crease should be clamped to the range [0, 1]");
    }

    if (!editor.set_tag(quad.v0, 11u) ||
        !editor.set_tag(edge, 22u) ||
        !editor.set_tag(quad.face, 33u)) {
        return TestResult::fail("set_tag should accept active elements");
    }

    if (mesh.vertex(quad.v0).tag != 11u ||
        mesh.edge(edge).tag != 22u ||
        mesh.face(quad.face).tag != 33u) {
        return TestResult::fail("set_tag should update element tags");
    }

    editor.clear_tags();
    if (mesh.vertex(quad.v0).tag != 0u ||
        mesh.edge(edge).tag != 0u ||
        mesh.face(quad.face).tag != 0u) {
        return TestResult::fail("clear_tags should reset element tags");
    }

    if (editor.diff().empty()) {
        return TestResult::fail("attribute edits should record diff changes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
