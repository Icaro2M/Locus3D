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
#include "editor/actions/core/ActionId.h"
#include "editor/actions/core/ActionResult.h"
#include "editor/actions/edit/RegisterEditActions.h"
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/actions/mesh/face/RegisterFaceActions.h"
#include "editor/actions/mesh/vertex/RegisterVertexActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/vec3.hpp>

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

struct TwoQuadSelection {
    locus::editor::SceneNodeId meshId{};
    locus::kernel::geometry::EdgeHandle sharedEdge{};
    locus::kernel::geometry::FaceHandle face{};
};

struct VertexChainSelection {
    locus::editor::SceneNodeId meshId{};
    locus::kernel::geometry::VertexHandle middle{};
};

[[nodiscard]] locus::editor::ActionId action_id(std::string value)
{
    return locus::editor::ActionId{ std::move(value) };
}

[[nodiscard]] locus::editor::ActionId dissolve_id()
{
    return action_id(
        std::string{ locus::editor::edit_actions::DissolveId });
}

[[nodiscard]] TwoQuadSelection create_two_quads(ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    TwoQuadSelection selection{};
    selection.meshId = harness.editor.scene().create_mesh("Dissolve Quads");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    LEMEditor editor(node->mesh());
    const VertexHandle v0 =
        editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    const VertexHandle v1 =
        editor.add_vertex(glm::vec3{ 0.0f, -1.0f, 0.0f });
    const VertexHandle v2 =
        editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    const VertexHandle v3 =
        editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });
    const VertexHandle v4 =
        editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    const VertexHandle v5 =
        editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });

    selection.face = editor.add_face({ v0, v1, v4, v3 });
    (void)editor.add_face({ v1, v2, v5, v4 });
    selection.sharedEdge = node->mesh().find_edge(v1, v4);
    return selection;
}

[[nodiscard]] VertexChainSelection create_vertex_chain(
    ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    VertexChainSelection selection{};
    selection.meshId = harness.editor.scene().create_mesh("Dissolve Chain");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    LEMEditor editor(node->mesh());
    const VertexHandle a =
        editor.add_vertex(glm::vec3{ -1.0f, 0.0f, 0.0f });
    selection.middle =
        editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle b =
        editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    (void)editor.find_or_create_edge(a, selection.middle);
    (void)editor.find_or_create_edge(selection.middle, b);
    return selection;
}

void select_edge(
    ActionHarness& harness,
    const TwoQuadSelection& selection)
{
    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Edge);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_edge(selection.sharedEdge);
}

void select_face(
    ActionHarness& harness,
    const TwoQuadSelection& selection)
{
    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Face);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_face(selection.face);
}

void select_vertex(
    ActionHarness& harness,
    const VertexChainSelection& selection)
{
    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Vertex);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_vertex(selection.middle);
}

} // namespace

namespace locus::tests {

TestResult run_dissolve_action_tests()
{
    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    ActionHarness harness;

    if (!register_default_actions(harness.registry)) {
        return TestResult::fail(
            "default action registration should succeed");
    }

    if (!harness.registry.contains(dissolve_id())
        || !harness.registry.contains(
            action_id(std::string{ vertex_actions::DissolveId }))
        || !harness.registry.contains(
            action_id(std::string{ edge_actions::DissolveId }))
        || harness.registry.contains(
            action_id(std::string{ face_actions::DissolveId }))) {
        return TestResult::fail(
            "Dissolve actions should register contextual, vertex, and edge actions only");
    }

    const TwoQuadSelection edgeSelection = create_two_quads(harness);
    select_edge(harness, edgeSelection);

    {
        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, dissolve_id());

        if (!result.succeeded()
            || harness.history.undo_size() != 1u
            || !harness.editor.selection().mesh().edges().empty()) {
            return TestResult::fail(
                "contextual Dissolve should dissolve an edge, add one history entry, and clean dead selection");
        }
    }

    MeshNode* node =
        harness.editor.scene().find_mesh(edgeSelection.meshId);

    if (node == nullptr
        || TopologyTraversal::faces(node->mesh()).size() != 1u
        || TopologyTraversal::edges(node->mesh()).size() != 6u
        || !harness.history.undo(harness.dispatcher).success
        || TopologyTraversal::faces(node->mesh()).size() != 2u
        || !harness.history.redo(harness.dispatcher).success
        || TopologyTraversal::faces(node->mesh()).size() != 1u) {
        return TestResult::fail(
            "Dissolve Edge action should support undo and redo");
    }

    const VertexChainSelection vertexSelection =
        create_vertex_chain(harness);
    select_vertex(harness, vertexSelection);
    const std::size_t undoBefore = harness.history.undo_size();

    {
        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, dissolve_id());

        node = harness.editor.scene().find_mesh(vertexSelection.meshId);

        if (!result.succeeded()
            || harness.history.undo_size() != undoBefore + 1u
            || node == nullptr
            || TopologyTraversal::vertices(node->mesh()).size() != 2u
            || !harness.editor.selection().mesh().vertices().empty()) {
            return TestResult::fail(
                "contextual Dissolve should dissolve loose chain vertices through the vertex action");
        }
    }

    const TwoQuadSelection surfaceVertexSelection = create_two_quads(harness);
    harness.editor.selection().set_scope(SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(SelectionGranularity::Vertex);
    harness.editor.selection().mesh().set_active_mesh(
        surfaceVertexSelection.meshId);
    harness.editor.selection().mesh().clear_components();

    node = harness.editor.scene().find_mesh(
        surfaceVertexSelection.meshId);

    if (node == nullptr) {
        return TestResult::fail(
            "surface vertex dissolve fixture should create a mesh node");
    }

    const VertexHandle surfaceVertex =
        TopologyTraversal::face_vertices(
            node->mesh(),
            surfaceVertexSelection.face).front();
    harness.editor.selection().mesh().add_vertex(surfaceVertex);

    const std::size_t undoBeforeSurfaceVertex =
        harness.history.undo_size();

    {
        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, dissolve_id());

        if (!result.is_unavailable()
            || harness.history.undo_size() != undoBeforeSurfaceVertex
            || TopologyTraversal::faces(node->mesh()).size() != 2u
            || !node->mesh().is_valid(surfaceVertex)) {
            return TestResult::fail(
                "contextual Dissolve should be unavailable for surface vertices instead of failing the runtime");
        }
    }

    const TwoQuadSelection faceSelection = create_two_quads(harness);
    select_face(harness, faceSelection);
    const std::size_t undoBeforeFace = harness.history.undo_size();

    {
        ActionContext context = harness.context();
        const ActionResult result =
            harness.executor.execute(context, dissolve_id());

        node = harness.editor.scene().find_mesh(faceSelection.meshId);

        if (!result.is_unavailable()
            || harness.history.undo_size() != undoBeforeFace
            || node == nullptr
            || TopologyTraversal::faces(node->mesh()).size() != 2u) {
            return TestResult::fail(
                "contextual Dissolve should be unavailable for face selection until a preserving policy exists");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
