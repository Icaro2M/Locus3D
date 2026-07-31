/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformTestSuite.h"

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/sync/PickingSync.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/transform/MeshTransformTargetResolver.h"
#include "editor/tools/transform/MeshTransformToolSession.h"
#include "editor/tools/transform/TransformTool.h"
#include "kernel/geometry/mesh/LEM.h"

#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] locus::editor::GizmoPointerInput gizmo_pointer_at(
    float x,
    float y,
    float z = 3.0f)
{
    locus::editor::GizmoPointerInput pointer{};
    pointer.ray.origin = glm::vec3{ x, y, z };
    pointer.ray.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
    pointer.viewDirection = glm::vec3{ 0.0f, 0.0f, -1.0f };
    pointer.viewRight = glm::vec3{ 1.0f, 0.0f, 0.0f };
    pointer.viewUp = glm::vec3{ 0.0f, 1.0f, 0.0f };
    pointer.visualScale = 1.0f;
    return pointer;
}

struct MeshFixture {
    locus::editor::Editor editor{};
    locus::editor::CommandDispatcher dispatcher{ editor };
    locus::editor::HistoryStack history{};
    locus::editor::PickingSync picking{};
    locus::editor::ToolContext context{ editor, dispatcher, history, picking };
    locus::editor::SceneNodeId meshId{};
    locus::kernel::geometry::VertexHandle v0{};
    locus::kernel::geometry::VertexHandle v1{};
    locus::kernel::geometry::VertexHandle v2{};
    locus::kernel::geometry::VertexHandle v3{};
    locus::kernel::geometry::EdgeHandle e01{};
    locus::kernel::geometry::EdgeHandle e12{};
    locus::kernel::geometry::EdgeHandle e23{};
    locus::kernel::geometry::FaceHandle face{};

    MeshFixture()
    {
        using namespace locus::kernel::geometry;

        meshId = editor.scene().create_mesh("Mesh");
        locus::editor::MeshNode* node = editor.scene().find_mesh(meshId);
        LEM& mesh = node->mesh();

        v0 = mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        v1 = mesh.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f });
        v2 = mesh.add_vertex(glm::vec3{ 2.0f, 2.0f, 0.0f });
        v3 = mesh.add_vertex(glm::vec3{ 0.0f, 2.0f, 0.0f });
        face = mesh.add_face({ v0, v1, v2, v3 });
        e01 = mesh.find_edge(v0, v1);
        e12 = mesh.find_edge(v1, v2);
        e23 = mesh.find_edge(v2, v3);

        editor.selection().set_scope(locus::editor::SelectionScope::ActiveMesh);
        editor.selection().mesh().set_active_mesh(meshId);
    }

    [[nodiscard]] locus::editor::MeshNode& node()
    {
        return *editor.scene().find_mesh(meshId);
    }
};

void select_vertices(MeshFixture& fixture)
{
    fixture.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Vertex);
    fixture.editor.selection().mesh().clear_components();
    fixture.editor.selection().mesh().add_vertex(fixture.v0);
    fixture.editor.selection().mesh().add_vertex(fixture.v1);
}

} // namespace

namespace locus::tests {

TestResult run_mesh_transform_target_resolver_tests()
{
    using namespace editor;

    MeshFixture fixture;

    select_vertices(fixture);
    MeshTransformTargetResolveResult resolved =
        MeshTransformTargetResolver::resolve(
            fixture.editor.scene(),
            fixture.editor.selection());

    if (!resolved.success ||
        resolved.target.node != fixture.meshId ||
        resolved.target.vertices.size() != 2u ||
        resolved.target.vertices[0] != fixture.v0 ||
        resolved.target.vertices[1] != fixture.v1 ||
        !near_vec3(resolved.target.pivot, glm::vec3{ 1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("resolver should map vertex selection to selected vertices and center pivot");
    }

    fixture.editor.selection().set_granularity(SelectionGranularity::Edge);
    fixture.editor.selection().mesh().clear_components();
    fixture.editor.selection().mesh().add_edge(fixture.e01);
    fixture.editor.selection().mesh().add_edge(fixture.e12);
    resolved = MeshTransformTargetResolver::resolve(
        fixture.editor.scene(),
        fixture.editor.selection());

    if (!resolved.success ||
        resolved.target.vertices.size() != 3u ||
        !near_vec3(resolved.target.pivot, glm::vec3{ 4.0f / 3.0f, 2.0f / 3.0f, 0.0f })) {
        return TestResult::fail("resolver should map adjacent edges to unique endpoints");
    }

    fixture.editor.selection().set_granularity(SelectionGranularity::Face);
    fixture.editor.selection().mesh().clear_components();
    fixture.editor.selection().mesh().add_face(fixture.face);
    resolved = MeshTransformTargetResolver::resolve(
        fixture.editor.scene(),
        fixture.editor.selection());

    if (!resolved.success ||
        resolved.target.vertices.size() != 4u ||
        !near_vec3(resolved.target.pivot, glm::vec3{ 1.0f, 1.0f, 0.0f })) {
        return TestResult::fail("resolver should map faces to unique boundary vertices");
    }

    fixture.editor.selection().mesh().clear_components();
    resolved = MeshTransformTargetResolver::resolve(
        fixture.editor.scene(),
        fixture.editor.selection());
    if (resolved.success) {
        return TestResult::fail("resolver should reject empty component selections");
    }

    fixture.editor.selection().mesh().add_face(
        kernel::geometry::FaceHandle{ 999 });
    resolved = MeshTransformTargetResolver::resolve(
        fixture.editor.scene(),
        fixture.editor.selection());
    if (resolved.success) {
        return TestResult::fail("resolver should reject invalid component handles");
    }

    fixture.editor.selection().mesh().set_active_mesh(
        editor::SceneNodeId{ 999 });
    resolved = MeshTransformTargetResolver::resolve(
        fixture.editor.scene(),
        fixture.editor.selection());
    if (resolved.success) {
        return TestResult::fail("resolver should reject missing active meshes");
    }

    fixture.editor.selection().set_scope(SelectionScope::Scene);
    resolved = MeshTransformTargetResolver::resolve(
        fixture.editor.scene(),
        fixture.editor.selection());
    if (resolved.success) {
        return TestResult::fail("resolver should reject incompatible selection scope");
    }

    return TestResult::pass();
}

TestResult run_mesh_transform_tool_session_tests()
{
    using namespace editor;

    MeshFixture fixture;
    select_vertices(fixture);

    MeshTransformToolSession session;
    TransformToolSessionBeginInput begin{};
    begin.mode = GizmoMode::Translate;
    begin.pointer = gizmo_pointer_at(2.0f, 0.0f);

    ToolResult result = session.begin(fixture.context, begin);
    if (!result.was_consumed() ||
        !session.is_active() ||
        !near_vec3(session.target().pivot, glm::vec3{ 1.0f, 0.0f, 0.0f })) {
        return TestResult::fail("mesh transform session should begin from a valid vertex selection");
    }

    if (!session.preview_translate(fixture.context, glm::vec3{ 1.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ 1.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v1).position, glm::vec3{ 3.0f, 0.0f, 0.0f })) {
        return TestResult::fail("translate preview should move all affected vertices by the world delta");
    }

    if (!session.preview_translate(fixture.context, glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v1).position, glm::vec3{ 4.0f, 0.0f, 0.0f })) {
        return TestResult::fail("translate preview should be non-cumulative from original positions");
    }

    result = session.cancel(fixture.context, ToolCancelReason::UserRequest);
    if (!result.was_consumed() ||
        session.is_active() ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ 0.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v1).position, glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
        fixture.history.undo_size() != 0u) {
        return TestResult::fail("cancel should restore original positions without history");
    }

    result = session.begin(fixture.context, begin);
    if (!result.was_consumed()) {
        return TestResult::fail("mesh transform session should begin again after cancel");
    }

    const glm::quat quarterTurn =
        glm::angleAxis(1.57079632679f, glm::vec3{ 0.0f, 0.0f, 1.0f });
    if (!session.preview_rotate(fixture.context, quarterTurn) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ 1.0f, -1.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v1).position, glm::vec3{ 1.0f, 1.0f, 0.0f })) {
        return TestResult::fail("rotate preview should rotate vertices around the shared pivot");
    }

    result = session.cancel(fixture.context, ToolCancelReason::UserRequest);
    if (!result.was_consumed()) {
        return TestResult::fail("cancel after rotation preview should succeed");
    }

    result = session.begin(fixture.context, begin);
    if (!result.was_consumed()) {
        return TestResult::fail("mesh transform session should begin for scale");
    }

    if (!session.preview_scale(fixture.context, glm::vec3{ 2.0f, 1.0f, 1.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ -1.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v1).position, glm::vec3{ 3.0f, 0.0f, 0.0f })) {
        return TestResult::fail("scale preview should scale vertices around the shared pivot");
    }

    result = session.confirm(fixture.context);
    if (!result.was_consumed() ||
        session.is_active() ||
        fixture.history.undo_size() != 1u ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ -1.0f, 0.0f, 0.0f }) ||
        !fixture.editor.selection().mesh().vertices().contains(fixture.v0) ||
        !fixture.editor.selection().mesh().vertices().contains(fixture.v1)) {
        return TestResult::fail("confirm should keep final positions, store one history entry, and preserve selection");
    }

    if (!fixture.history.undo(fixture.dispatcher) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ 0.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v1).position, glm::vec3{ 2.0f, 0.0f, 0.0f })) {
        return TestResult::fail("undo should restore original vertex positions");
    }

    if (!fixture.history.redo(fixture.dispatcher) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v0).position, glm::vec3{ -1.0f, 0.0f, 0.0f }) ||
        !near_vec3(fixture.node().mesh().vertex(fixture.v1).position, glm::vec3{ 3.0f, 0.0f, 0.0f })) {
        return TestResult::fail("redo should reapply final vertex positions");
    }

    const glm::vec3 normal = fixture.node().mesh().face(fixture.face).normal;
    if (glm::length(normal) <= epsilon) {
        return TestResult::fail("mesh transform should leave face normals valid");
    }

    return TestResult::pass();
}

TestResult run_transform_tool_selection_tests()
{
    using namespace editor;

    MeshFixture fixture;
    TransformTool tool;

    if (tool.can_activate(fixture.context)) {
        return TestResult::fail("TransformTool should reject mesh scope without selected components");
    }

    fixture.editor.selection().set_scope(SelectionScope::Scene);
    fixture.editor.selection().set_granularity(SelectionGranularity::Object);
    fixture.editor.selection().objects().set(fixture.meshId);
    if (!tool.can_activate(fixture.context)) {
        return TestResult::fail("TransformTool should activate for object selection");
    }

    fixture.editor.selection().set_scope(SelectionScope::ActiveMesh);
    fixture.editor.selection().mesh().set_active_mesh(fixture.meshId);
    fixture.editor.selection().set_granularity(SelectionGranularity::Vertex);
    fixture.editor.selection().mesh().clear_components();
    fixture.editor.selection().mesh().add_vertex(fixture.v0);
    if (!tool.can_activate(fixture.context)) {
        return TestResult::fail("TransformTool should activate for vertex selection");
    }

    fixture.editor.selection().set_granularity(SelectionGranularity::Edge);
    fixture.editor.selection().mesh().clear_components();
    fixture.editor.selection().mesh().add_edge(fixture.e01);
    if (!tool.can_activate(fixture.context)) {
        return TestResult::fail("TransformTool should activate for edge selection");
    }

    fixture.editor.selection().set_granularity(SelectionGranularity::Face);
    fixture.editor.selection().mesh().clear_components();
    fixture.editor.selection().mesh().add_face(fixture.face);
    if (!tool.can_activate(fixture.context)) {
        return TestResult::fail("TransformTool should activate for face selection");
    }

    ToolResult activation = tool.activate(fixture.context);
    if (activation.failed() ||
        !tool.set_mode(GizmoMode::Translate)) {
        return TestResult::fail("TransformTool should activate and accept translate mode for face selection");
    }

    tool.refresh_gizmo_state(fixture.context);
    const GizmoState& state = tool.gizmo_state();
    if (!state.visible ||
        state.mode != GizmoMode::Translate ||
        !near_vec3(state.pivot, glm::vec3{ 1.0f, 1.0f, 0.0f })) {
        return TestResult::fail("TransformTool should present a translate gizmo at the selected face pivot");
    }

    fixture.editor.selection().set_granularity(SelectionGranularity::Loop);
    if (tool.can_activate(fixture.context)) {
        return TestResult::fail("TransformTool should reject unsupported mesh granularity");
    }

    return TestResult::pass();
}

} // namespace locus::tests
