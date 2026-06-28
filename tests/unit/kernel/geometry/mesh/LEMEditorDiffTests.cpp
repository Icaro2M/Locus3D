/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

namespace locus::tests {

TestResult run_lem_editor_diff_tests()
{
    using namespace kernel::geometry;

    LEM mesh;
    LEMEditor editor(mesh);

    if (!editor.diff().empty()) {
        return TestResult::fail("new LEMEditor diff should start empty");
    }

    const QuadMesh quad = make_quad(editor);
    if (!quad.face.is_valid()) {
        return TestResult::fail("test quad should be created through LEMEditor");
    }

    if (editor.diff().size() != 17) {
        return TestResult::fail("quad creation should record vertices, edges, loops, and face");
    }

    const LEMDiff createdDiff = editor.take_diff();
    if (createdDiff.size() != 17 || !editor.diff().empty()) {
        return TestResult::fail("take_diff should return and clear accumulated changes");
    }

    if (createdDiff.changes().front().type != LEMChangeType::VertexAdded ||
        createdDiff.changes()[12].type != LEMChangeType::FaceAdded ||
        createdDiff.changes().back().type != LEMChangeType::VertexModified) {
        return TestResult::fail("quad creation diff should record creation and affected vertices");
    }

    if (!editor.set_vertex_position(quad.v0, glm::vec3{ -2.0f, -1.0f, 0.0f })) {
        return TestResult::fail("set_vertex_position should accept active vertices");
    }

    bool recordedVertexModification = false;
    for (const LEMChange& change : editor.diff().changes()) {
        if (change.type == LEMChangeType::VertexModified &&
            change.elementType == LEMElementType::Vertex &&
            change.id == quad.v0.id) {
            recordedVertexModification = true;
            break;
        }
    }

    if (!recordedVertexModification) {
        return TestResult::fail("position edit should record a vertex modification");
    }

    editor.clear_diff();
    if (!editor.diff().empty()) {
        return TestResult::fail("clear_diff should remove accumulated changes");
    }

    editor.clear();
    if (!mesh.empty()) {
        return TestResult::fail("LEMEditor clear should clear the edited mesh");
    }

    if (editor.diff().size() != 1 ||
        editor.diff().changes().front().type != LEMChangeType::MeshCleared) {
        return TestResult::fail("LEMEditor clear should record a mesh cleared change");
    }

    return TestResult::pass();
}

} // namespace locus::tests
