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
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <glm/vec3.hpp>

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

struct BridgeSelection {
    locus::editor::SceneNodeId meshId{};
    locus::kernel::geometry::EdgeHandle firstEdge{};
    locus::kernel::geometry::EdgeHandle secondEdge{};
};

[[nodiscard]] locus::editor::ActionId bridge_action_id()
{
    return locus::editor::ActionId{
        std::string{ locus::editor::edge_actions::BridgeId }
    };
}

[[nodiscard]] BridgeSelection create_bridge_selection(
    ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    BridgeSelection selection{};
    selection.meshId =
        harness.editor.scene().create_mesh("Bridge Mesh");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    LEMEditor editor(node->mesh());
    const VertexHandle a0 =
        editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle a1 =
        editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    const VertexHandle b0 =
        editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });
    const VertexHandle b1 =
        editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });

    selection.firstEdge =
        editor.find_or_create_edge(a0, a1);
    selection.secondEdge =
        editor.find_or_create_edge(b0, b1);

    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Edge);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().add_edge(selection.firstEdge);
    harness.editor.selection().mesh().add_edge(selection.secondEdge);

    return selection;
}

[[nodiscard]] BridgeSelection create_shared_vertex_selection(
    ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    BridgeSelection selection{};
    selection.meshId =
        harness.editor.scene().create_mesh("Shared Vertex Mesh");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    LEMEditor editor(node->mesh());
    const VertexHandle shared =
        editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const VertexHandle first =
        editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
    const VertexHandle second =
        editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });

    selection.firstEdge =
        editor.find_or_create_edge(shared, first);
    selection.secondEdge =
        editor.find_or_create_edge(shared, second);

    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Edge);
    harness.editor.selection().mesh().set_active_mesh(selection.meshId);
    harness.editor.selection().mesh().add_edge(selection.firstEdge);
    harness.editor.selection().mesh().add_edge(selection.secondEdge);

    return selection;
}

[[nodiscard]] std::size_t active_face_count(
    const locus::editor::MeshNode& node)
{
    return locus::kernel::geometry::TopologyTraversal::faces(
        node.mesh()).size();
}

} // namespace

namespace locus::tests {

TestResult run_bridge_edge_action_tests()
{
    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    ActionHarness harness;

    if (!register_default_actions(harness.registry)) {
        return TestResult::fail(
            "default action registration should succeed");
    }

    const ActionId bridgeId =
        bridge_action_id();

    if (!harness.registry.contains(bridgeId) ||
        harness.registry.find(bridgeId) == nullptr) {
        return TestResult::fail(
            "Bridge Edge action should be registered and discoverable");
    }

    const ActionDescriptor* descriptor =
        harness.registry.descriptor(bridgeId);

    if (descriptor == nullptr ||
        descriptor->id != bridgeId ||
        descriptor->name != "Bridge Edge" ||
        descriptor->category != ActionCategory::Mesh) {
        return TestResult::fail(
            "Bridge Edge descriptor should expose mesh action metadata");
    }

    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, bridgeId)) {
            return TestResult::fail(
                "Bridge Edge should be unavailable without an active mesh");
        }

        const ActionResult result =
            harness.executor.execute(context, bridgeId);

        if (!result.is_unavailable() ||
            harness.history.undo_size() != 0u) {
            return TestResult::fail(
                "Unavailable Bridge Edge should not change history");
        }
    }

    const BridgeSelection selection =
        create_bridge_selection(harness);
    MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    if (node == nullptr) {
        return TestResult::fail(
            "Bridge Edge fixture should create an active mesh node");
    }

    harness.editor.selection().set_granularity(
        SelectionGranularity::Face);
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, bridgeId)) {
            return TestResult::fail(
                "Bridge Edge should reject non-edge granularity");
        }
    }

    harness.editor.selection().set_granularity(
        SelectionGranularity::Edge);
    harness.editor.selection().mesh().clear_components();
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, bridgeId)) {
            return TestResult::fail(
                "Bridge Edge should reject zero selected edges");
        }
    }

    harness.editor.selection().mesh().add_edge(selection.firstEdge);
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, bridgeId)) {
            return TestResult::fail(
                "Bridge Edge should reject one selected edge");
        }
    }

    harness.editor.selection().mesh().add_edge(selection.secondEdge);
    harness.editor.selection().mesh().add_edge(EdgeHandle{ 999u });
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, bridgeId)) {
            return TestResult::fail(
                "Bridge Edge should reject invalid selected handles");
        }
    }

    harness.editor.selection().mesh().clear_components();
    harness.editor.selection().mesh().add_edge(selection.firstEdge);
    harness.editor.selection().mesh().add_edge(selection.secondEdge);
    {
        ActionContext context =
            harness.context();

        if (!harness.executor.can_execute(context, bridgeId)) {
            return TestResult::fail(
                "Bridge Edge should be available for two boundary edges");
        }

        const ActionResult result =
            harness.executor.execute(context, bridgeId);

        if (!result.succeeded() ||
            harness.history.undo_size() != 1u ||
            harness.history.redo_size() != 0u ||
            active_face_count(*node) != 1u) {
            return TestResult::fail(
                "Bridge Edge should execute once through history");
        }

        if (!harness.editor.selection().mesh().edges().contains(
                selection.firstEdge) ||
            !harness.editor.selection().mesh().edges().contains(
                selection.secondEdge)) {
            return TestResult::fail(
                "Bridge Edge should preserve still-valid selected edges");
        }

        if (!TopologyValidator::validate(node->mesh()).valid()) {
            return TestResult::fail(
                "Bridge Edge action result should leave valid topology");
        }
    }

    if (!harness.history.undo(harness.dispatcher) ||
        active_face_count(*node) != 0u ||
        harness.history.undo_size() != 0u ||
        harness.history.redo_size() != 1u) {
        return TestResult::fail(
            "Bridge Edge undo should restore the previous mesh");
    }

    if (!harness.history.redo(harness.dispatcher) ||
        active_face_count(*node) != 1u ||
        harness.history.undo_size() != 1u ||
        harness.history.redo_size() != 0u) {
        return TestResult::fail(
            "Bridge Edge redo should restore the bridged mesh");
    }

    {
        ActionHarness failingHarness;
        (void)register_default_actions(failingHarness.registry);
        const BridgeSelection failingSelection =
            create_shared_vertex_selection(failingHarness);
        MeshNode* failingNode =
            failingHarness.editor.scene().find_mesh(
                failingSelection.meshId);

        ActionContext context =
            failingHarness.context();

        if (!failingHarness.executor.can_execute(context, bridgeId)) {
            return TestResult::fail(
                "Shared-vertex boundary edges should reach kernel validation");
        }

        const ActionResult result =
            failingHarness.executor.execute(context, bridgeId);

        if (!result.failed() ||
            failingHarness.history.undo_size() != 0u ||
            failingNode == nullptr ||
            active_face_count(*failingNode) != 0u) {
            return TestResult::fail(
                "Kernel-rejected Bridge Edge should fail without history");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
