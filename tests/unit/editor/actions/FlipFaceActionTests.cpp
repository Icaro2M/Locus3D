/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ActionTestSuite.h"

#include "editor/Editor.h"
#include "editor/actions/ActionExecutor.h"
#include "editor/actions/ActionRegistry.h"
#include "editor/actions/Actions.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionResult.h"
#include "editor/actions/mesh/face/RegisterFaceActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace {

struct ActionHarness {
    locus::editor::Editor editor{};
    locus::editor::CommandDispatcher dispatcher;
    locus::editor::HistoryStack history{};
    locus::editor::ActionRegistry registry{};
    locus::editor::ActionExecutor executor;

    ActionHarness()
        : dispatcher(editor)
        , executor(registry)
    {
    }

    [[nodiscard]] locus::editor::ActionContext context()
    {
        return locus::editor::ActionContext(
            editor,
            dispatcher,
            history);
    }
};

struct FaceFixture {
    locus::editor::SceneNodeId meshId{};
    locus::kernel::geometry::FaceHandle firstFace{};
    locus::kernel::geometry::FaceHandle secondFace{};
    locus::kernel::geometry::VertexHandle firstVertex{};
    locus::kernel::geometry::EdgeHandle firstEdge{};
};

struct QuadMesh {
    locus::kernel::geometry::VertexHandle v0{};
    locus::kernel::geometry::VertexHandle v1{};
    locus::kernel::geometry::VertexHandle v2{};
    locus::kernel::geometry::VertexHandle v3{};
    locus::kernel::geometry::FaceHandle face{};
};

[[nodiscard]] locus::editor::ActionId flip_faces_id()
{
    return locus::editor::ActionId{
        std::string{ locus::editor::face_actions::FlipFaceId }
    };
}

[[nodiscard]] QuadMesh add_offset_quad(
    locus::kernel::geometry::LEMEditor& editor,
    float offsetX)
{
    QuadMesh quad{};
    quad.v0 = editor.add_vertex(glm::vec3{ offsetX - 1.0f, -1.0f, 0.0f });
    quad.v1 = editor.add_vertex(glm::vec3{ offsetX + 1.0f, -1.0f, 0.0f });
    quad.v2 = editor.add_vertex(glm::vec3{ offsetX + 1.0f, 1.0f, 0.0f });
    quad.v3 = editor.add_vertex(glm::vec3{ offsetX - 1.0f, 1.0f, 0.0f });
    quad.face = editor.add_face({ quad.v0, quad.v1, quad.v2, quad.v3 });
    return quad;
}

[[nodiscard]] FaceFixture create_face_fixture(ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    FaceFixture fixture{};
    fixture.meshId =
        harness.editor.scene().create_mesh("Flip Faces Mesh");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(fixture.meshId);

    LEMEditor editor(node->mesh());
    const QuadMesh first =
        add_offset_quad(editor, -3.0f);
    const QuadMesh second =
        add_offset_quad(editor, 3.0f);

    fixture.firstFace = first.face;
    fixture.secondFace = second.face;
    fixture.firstVertex = first.v0;
    fixture.firstEdge = node->mesh().find_edge(first.v0, first.v1);

    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Face);
    harness.editor.selection().mesh().set_active_mesh(fixture.meshId);
    harness.editor.selection().mesh().clear_components();

    return fixture;
}

[[nodiscard]] std::vector<locus::kernel::geometry::VertexHandle>
face_vertices(
    const locus::kernel::geometry::LEM& mesh,
    locus::kernel::geometry::FaceHandle face)
{
    return locus::kernel::geometry::TopologyTraversal::face_vertices(
        mesh,
        face);
}

[[nodiscard]] bool same_cycle(
    std::vector<locus::kernel::geometry::VertexHandle> lhs,
    std::vector<locus::kernel::geometry::VertexHandle> rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    const auto by_id = [](const auto left, const auto right) {
        return left.id.value < right.id.value;
    };

    std::sort(lhs.begin(), lhs.end(), by_id);
    std::sort(rhs.begin(), rhs.end(), by_id);
    return lhs == rhs;
}

[[nodiscard]] bool reversed_winding(
    const std::vector<locus::kernel::geometry::VertexHandle>& before,
    const std::vector<locus::kernel::geometry::VertexHandle>& after)
{
    if (before.size() != after.size() || before.empty()) {
        return false;
    }

    if (!same_cycle(before, after)) {
        return false;
    }

    const auto start =
        std::find(after.begin(), after.end(), before.front());

    if (start == after.end()) {
        return false;
    }

    const std::size_t afterStart =
        static_cast<std::size_t>(
            std::distance(after.begin(), start));

    for (std::size_t i = 0u; i < before.size(); ++i) {
        const std::size_t expectedIndex =
            (before.size() - i) % before.size();

        if (after[(afterStart + i) % after.size()]
            != before[expectedIndex]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool execute_flip(ActionHarness& harness)
{
    locus::editor::ActionContext context = harness.context();
    const locus::editor::ActionResult result =
        harness.executor.execute(context, flip_faces_id());
    return result.succeeded();
}

} // namespace

namespace locus::tests {

TestResult run_flip_face_action_tests()
{
    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    ActionHarness harness;

    if (!register_default_actions(harness.registry)) {
        return TestResult::fail(
            "default action registration should succeed");
    }

    const ActionId actionId = flip_faces_id();

    if (!harness.registry.contains(actionId)
        || harness.registry.find(actionId) == nullptr) {
        return TestResult::fail(
            "Flip Faces action should be registered and discoverable");
    }

    const ActionDescriptor* descriptor =
        harness.registry.descriptor(actionId);

    if (descriptor == nullptr
        || descriptor->id != actionId
        || descriptor->name != "Flip Face"
        || descriptor->category != ActionCategory::Mesh) {
        return TestResult::fail(
            "Flip Faces descriptor should expose mesh action metadata");
    }

    {
        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, actionId);

        if (!result.is_unavailable()
            || harness.history.undo_size() != 0u) {
            return TestResult::fail(
                "Flip Faces should be unavailable without an active mesh");
        }
    }

    const FaceFixture fixture = create_face_fixture(harness);
    MeshNode* node =
        harness.editor.scene().find_mesh(fixture.meshId);

    if (node == nullptr) {
        return TestResult::fail(
            "Flip Faces fixture should create a mesh node");
    }

    const std::array<std::vector<VertexHandle>, 2u> originalVertices{
        face_vertices(node->mesh(), fixture.firstFace),
        face_vertices(node->mesh(), fixture.secondFace)
    };
    const glm::vec3 originalNormal =
        NormalBuilder::face_normal(node->mesh(), fixture.firstFace);

    harness.editor.selection().mesh().add_face(fixture.firstFace);
    {
        ActionContext context = harness.context();

        if (!harness.executor.can_execute(context, actionId)
            || !execute_flip(harness)
            || harness.history.undo_size() != 1u
            || harness.history.redo_size() != 0u
            || !harness.editor.selection().mesh().faces().contains(
                fixture.firstFace)) {
            return TestResult::fail(
                "Flip Faces should execute one selected face through history and preserve selection");
        }
    }

    if (!reversed_winding(
            originalVertices[0],
            face_vertices(node->mesh(), fixture.firstFace))
        || glm::dot(
            originalNormal,
            NormalBuilder::face_normal(node->mesh(), fixture.firstFace))
            > -0.999f
        || !TopologyValidator::validate(node->mesh()).valid()) {
        return TestResult::fail(
            "Flip Faces should reverse the selected face winding and normal");
    }

    if (!harness.history.undo(harness.dispatcher).success
        || face_vertices(node->mesh(), fixture.firstFace)
            != originalVertices[0]
        || harness.history.undo_size() != 0u
        || harness.history.redo_size() != 1u) {
        return TestResult::fail(
            "Flip Faces undo should restore the original winding");
    }

    if (!harness.history.redo(harness.dispatcher).success
        || !reversed_winding(
            originalVertices[0],
            face_vertices(node->mesh(), fixture.firstFace))
        || harness.history.undo_size() != 1u
        || harness.history.redo_size() != 0u) {
        return TestResult::fail(
            "Flip Faces redo should restore the flipped winding");
    }

    (void)harness.history.undo(harness.dispatcher);
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_face(fixture.firstFace);
    harness.editor.selection().mesh().add_face(fixture.secondFace);

    if (!execute_flip(harness)
        || harness.history.undo_size() != 1u
        || !reversed_winding(
            originalVertices[0],
            face_vertices(node->mesh(), fixture.firstFace))
        || !reversed_winding(
            originalVertices[1],
            face_vertices(node->mesh(), fixture.secondFace))
        || !harness.editor.selection().mesh().faces().contains(
            fixture.firstFace)
        || !harness.editor.selection().mesh().faces().contains(
            fixture.secondFace)) {
        return TestResult::fail(
            "Flip Faces should flip multiple selected faces as one history entry");
    }

    const std::size_t historyAfterMultiFlip =
        harness.history.undo_size();
    const std::vector<VertexHandle> flippedFirst =
        face_vertices(node->mesh(), fixture.firstFace);

    harness.editor.selection().set_scope(SelectionScope::Scene);
    harness.editor.selection().set_granularity(SelectionGranularity::Object);
    {
        ActionContext context = harness.context();

        if (harness.executor.can_execute(context, actionId)
            || harness.executor.execute(context, actionId).succeeded()
            || harness.history.undo_size() != historyAfterMultiFlip
            || face_vertices(node->mesh(), fixture.firstFace)
                != flippedFirst) {
            return TestResult::fail(
                "Flip Faces should reject object selection context without history");
        }
    }

    const struct InvalidComponentCase {
        SelectionGranularity granularity;
        VertexHandle vertex;
        EdgeHandle edge;
        FaceHandle face;
    } invalidCases[] = {
        { SelectionGranularity::Vertex, fixture.firstVertex, {}, {} },
        { SelectionGranularity::Edge, {}, fixture.firstEdge, {} },
        { SelectionGranularity::Face, {}, {}, FaceHandle{ 999u } },
    };

    for (const InvalidComponentCase& testCase : invalidCases) {
        harness.editor.selection().set_scope(SelectionScope::ActiveMesh);
        harness.editor.selection().set_granularity(testCase.granularity);
        harness.editor.selection().mesh().set_active_mesh(fixture.meshId);
        harness.editor.selection().mesh().clear_components();

        switch (testCase.granularity) {
        case SelectionGranularity::Vertex:
            harness.editor.selection().mesh().add_vertex(testCase.vertex);
            break;
        case SelectionGranularity::Edge:
            harness.editor.selection().mesh().add_edge(testCase.edge);
            break;
        case SelectionGranularity::Face:
            harness.editor.selection().mesh().add_face(testCase.face);
            break;
        default:
            return TestResult::fail(
                "Flip Faces invalid fixture used an unsupported granularity");
        }

        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, actionId);

        if (!result.is_unavailable()
            || harness.history.undo_size() != historyAfterMultiFlip
            || face_vertices(node->mesh(), fixture.firstFace)
                != flippedFirst) {
            return TestResult::fail(
                "Flip Faces should reject invalid component contexts without changes");
        }
    }

    harness.editor.selection().set_scope(SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(SelectionGranularity::Face);
    harness.editor.selection().mesh().set_active_mesh(SceneNodeId{ 999u });
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_face(fixture.firstFace);
    {
        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, actionId);

        if (!result.is_unavailable()
            || harness.history.undo_size() != historyAfterMultiFlip
            || face_vertices(node->mesh(), fixture.firstFace)
                != flippedFirst) {
            return TestResult::fail(
                "Flip Faces should reject a missing active mesh without changes");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
