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
#include "editor/actions/core/ActionResult.h"
#include "editor/actions/edit/RegisterEditActions.h"
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/actions/mesh/face/RegisterFaceActions.h"
#include "editor/actions/mesh/vertex/RegisterVertexActions.h"
#include "editor/actions/scene/RegisterSceneActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/vec3.hpp>

#include <array>
#include <string>

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

struct QuadSelection {
    locus::editor::SceneNodeId meshId{};
    std::array<locus::kernel::geometry::VertexHandle, 4u> vertices{};
    locus::kernel::geometry::EdgeHandle edge{};
    locus::kernel::geometry::FaceHandle face{};
};

[[nodiscard]] locus::editor::ActionId action_id(std::string value)
{
    return locus::editor::ActionId{ std::move(value) };
}

[[nodiscard]] locus::editor::ActionId edit_delete_id()
{
    return action_id(
        std::string{ locus::editor::edit_actions::DeleteId });
}

[[nodiscard]] QuadSelection create_quad_mesh(ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    QuadSelection selection{};
    selection.meshId = harness.editor.scene().create_mesh("Delete Quad");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    LEMEditor editor(node->mesh());
    selection.vertices = {
        editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f })
    };

    selection.face =
        editor.add_face(
            {
                selection.vertices[0],
                selection.vertices[1],
                selection.vertices[2],
                selection.vertices[3]
            });
    selection.edge =
        node->mesh().find_edge(
            selection.vertices[0],
            selection.vertices[1]);

    return selection;
}

void select_face(ActionHarness& harness, const QuadSelection& selection)
{
    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Face);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_face(selection.face);
}

void select_edge(ActionHarness& harness, const QuadSelection& selection)
{
    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Edge);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_edge(selection.edge);
}

void select_vertex(ActionHarness& harness, const QuadSelection& selection)
{
    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Vertex);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_vertex(selection.vertices[0]);
}

[[nodiscard]] bool execute_delete(ActionHarness& harness)
{
    locus::editor::ActionContext context = harness.context();
    const locus::editor::ActionResult result =
        harness.executor.execute(context, edit_delete_id());
    return result.succeeded();
}

} // namespace

namespace locus::tests {

TestResult run_delete_action_tests()
{
    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    ActionHarness harness;

    if (!register_default_actions(harness.registry)) {
        return TestResult::fail(
            "default action registration should succeed");
    }

    const ActionId deleteId = edit_delete_id();

    if (!harness.registry.contains(deleteId)
        || !harness.registry.contains(
            action_id(std::string{ scene_actions::DeleteObjectsId }))
        || !harness.registry.contains(
            action_id(std::string{ vertex_actions::DeleteId }))
        || !harness.registry.contains(
            action_id(std::string{ edge_actions::DeleteId }))
        || !harness.registry.contains(
            action_id(std::string{ face_actions::DeleteId }))) {
        return TestResult::fail(
            "Delete actions should be registered");
    }

    {
        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, deleteId);

        if (!result.is_unavailable()
            || harness.history.undo_size() != 0u) {
            return TestResult::fail(
                "contextual Delete should be unavailable for empty selection");
        }
    }

    const SceneNodeId objectA =
        harness.editor.scene().create_mesh("Object A");
    const SceneNodeId objectB =
        harness.editor.scene().create_mesh("Object B");

    harness.editor.selection().set_scope(SelectionScope::Scene);
    harness.editor.selection().set_granularity(
        SelectionGranularity::Object);
    harness.editor.selection().objects().add(objectA);
    harness.editor.selection().objects().add(objectB);
    harness.editor.selection().objects().set_active(objectB);

    if (!execute_delete(harness)
        || harness.editor.scene().find_node(objectA) != nullptr
        || harness.editor.scene().find_node(objectB) != nullptr
        || harness.history.undo_size() != 1u) {
        return TestResult::fail(
            "contextual Delete should delete multiple selected objects in one history entry");
    }

    if (!harness.history.undo(harness.dispatcher).success
        || harness.editor.scene().find_node(objectA) == nullptr
        || harness.editor.scene().find_node(objectB) == nullptr
        || !harness.history.redo(harness.dispatcher).success
        || harness.editor.scene().find_node(objectA) != nullptr
        || harness.editor.scene().find_node(objectB) != nullptr) {
        return TestResult::fail(
            "object Delete should support undo and redo");
    }

    const QuadSelection faceSelection = create_quad_mesh(harness);
    select_face(harness, faceSelection);

    if (!execute_delete(harness)) {
        return TestResult::fail(
            "contextual Delete should execute face Delete");
    }

    MeshNode* node =
        harness.editor.scene().find_mesh(faceSelection.meshId);

    if (node == nullptr
        || TopologyTraversal::faces(node->mesh()).size() != 0u
        || TopologyTraversal::edges(node->mesh()).size() != 4u
        || !harness.editor.selection().mesh().faces().empty()) {
        return TestResult::fail(
            "face Delete should remove selected faces and clean selection");
    }

    (void)harness.history.undo(harness.dispatcher);

    const QuadSelection edgeSelection = create_quad_mesh(harness);
    select_edge(harness, edgeSelection);

    if (!execute_delete(harness)) {
        return TestResult::fail(
            "contextual Delete should execute edge Delete");
    }

    node = harness.editor.scene().find_mesh(edgeSelection.meshId);

    if (node == nullptr
        || TopologyTraversal::faces(node->mesh()).size() != 0u
        || TopologyTraversal::edges(node->mesh()).size() != 3u
        || !harness.editor.selection().mesh().edges().empty()) {
        return TestResult::fail(
            "edge Delete should remove incident faces, selected edge, and clean selection");
    }

    const QuadSelection vertexSelection = create_quad_mesh(harness);
    select_vertex(harness, vertexSelection);

    if (!execute_delete(harness)) {
        return TestResult::fail(
            "contextual Delete should execute vertex Delete");
    }

    node = harness.editor.scene().find_mesh(vertexSelection.meshId);

    if (node == nullptr
        || TopologyTraversal::faces(node->mesh()).size() != 0u
        || TopologyTraversal::edges(node->mesh()).size() != 2u
        || TopologyTraversal::vertices(node->mesh()).size() != 3u
        || !harness.editor.selection().mesh().vertices().empty()) {
        return TestResult::fail(
            "vertex Delete should remove incident topology and clean selection");
    }

    return TestResult::pass();
}

} // namespace locus::tests
