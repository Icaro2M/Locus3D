/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformTestSuite.h"

#include "editor/Editor.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/mesh/face/SolidifyTool.h"
#include "kernel/geometry/mesh/LEM.h"

#include <glm/vec3.hpp>

namespace {

struct FaceToolFixture {
    locus::editor::Editor editor{};
    locus::editor::ToolContext context{ editor };
    locus::editor::SceneNodeId meshId{};
    locus::kernel::geometry::FaceHandle face{};

    FaceToolFixture()
    {
        using namespace locus::kernel::geometry;

        meshId = editor.scene().create_mesh("Mesh");
        locus::kernel::geometry::LEM& mesh =
            editor.scene().find_mesh(meshId)->mesh();

        const VertexHandle v0 =
            mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle v1 =
            mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const VertexHandle v2 =
            mesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
        face = mesh.add_face({ v0, v1, v2 });

        editor.selection_controller().enter_mesh_context(
            meshId,
            locus::editor::SelectionGranularity::Face);
    }

    void set_stale_hover()
    {
        editor.selection().mesh().set_hovered_face(face);
        editor.selection_controller().set_hovered_object(meshId);
    }
};

[[nodiscard]] bool has_dirty(
    locus::editor::EditorDirtyFlags mask,
    locus::editor::EditorDirtyFlags flag)
{
    return locus::editor::has_flag(mask, flag);
}

[[nodiscard]] bool common_hover_is_clear(const FaceToolFixture& fixture)
{
    return
        fixture.editor.selection().mesh().hovered_face().is_invalid() &&
        fixture.editor.selection().objects().hovered().is_invalid();
}

} // namespace

namespace locus::tests {

TestResult run_mesh_drag_operation_tool_hover_tests()
{
    using namespace editor;

    FaceToolFixture fixture;
    SolidifyTool tool;

    fixture.set_stale_hover();
    ToolResult activation = tool.activate(fixture.context);

    if (activation.failed() ||
        !activation.was_consumed() ||
        !common_hover_is_clear(fixture) ||
        !has_dirty(activation.dirtyFlags, EditorDirtyFlags::Selection) ||
        !has_dirty(activation.dirtyFlags, EditorDirtyFlags::Render)) {
        return TestResult::fail("mesh drag tools should clear stale common hover when activated");
    }

    fixture.set_stale_hover();
    ToolResult deactivation = tool.deactivate(fixture.context);

    if (deactivation.failed() ||
        !deactivation.was_consumed() ||
        !common_hover_is_clear(fixture) ||
        !has_dirty(deactivation.dirtyFlags, EditorDirtyFlags::Selection) ||
        !has_dirty(deactivation.dirtyFlags, EditorDirtyFlags::Render)) {
        return TestResult::fail("mesh drag tools should clear common hover when deactivated");
    }

    return TestResult::pass();
}

} // namespace locus::tests
