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
#include "editor/actions/mesh/topology/RegisterTopologyActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <glm/vec3.hpp>

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

struct QuadHoleSelection {
    locus::editor::SceneNodeId meshId{};
    std::array<locus::kernel::geometry::VertexHandle, 4u> vertices{};
    std::vector<locus::kernel::geometry::EdgeHandle> edges{};
};

[[nodiscard]] locus::editor::ActionId fill_hole_action_id()
{
    return locus::editor::ActionId{
        std::string{ locus::editor::topology_actions::FillHoleId }
    };
}

void select_edges(
    ActionHarness& harness,
    locus::editor::SceneNodeId meshId,
    const std::vector<locus::kernel::geometry::EdgeHandle>& edges)
{
    harness.editor.selection().set_scope(
        locus::editor::SelectionScope::ActiveMesh);
    harness.editor.selection().set_granularity(
        locus::editor::SelectionGranularity::Edge);
    harness.editor.selection().mesh().set_active_mesh(meshId);
    harness.editor.selection().mesh().clear_components();

    for (locus::kernel::geometry::EdgeHandle edge : edges) {
        harness.editor.selection().mesh().add_edge(edge);
    }
}

[[nodiscard]] QuadHoleSelection create_quad_hole_selection(
    ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    QuadHoleSelection selection{};
    selection.meshId =
        harness.editor.scene().create_mesh("Fill Hole Quad");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    LEMEditor editor(node->mesh());
    selection.vertices = {
        editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f })
    };

    selection.edges = {
        editor.find_or_create_edge(selection.vertices[0], selection.vertices[1]),
        editor.find_or_create_edge(selection.vertices[1], selection.vertices[2]),
        editor.find_or_create_edge(selection.vertices[2], selection.vertices[3]),
        editor.find_or_create_edge(selection.vertices[3], selection.vertices[0])
    };

    select_edges(
        harness,
        selection.meshId,
        {
            selection.edges[2],
            selection.edges[0],
            selection.edges[3],
            selection.edges[1]
        });

    return selection;
}

[[nodiscard]] QuadHoleSelection create_open_edge_selection(
    ActionHarness& harness)
{
    using namespace locus::kernel::geometry;

    QuadHoleSelection selection{};
    selection.meshId =
        harness.editor.scene().create_mesh("Open Edge Set");

    locus::editor::MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    LEMEditor editor(node->mesh());
    selection.vertices = {
        editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f }),
        editor.add_vertex(glm::vec3{ 3.0f, 0.0f, 0.0f })
    };

    selection.edges = {
        editor.find_or_create_edge(selection.vertices[0], selection.vertices[1]),
        editor.find_or_create_edge(selection.vertices[1], selection.vertices[2]),
        editor.find_or_create_edge(selection.vertices[2], selection.vertices[3])
    };

    select_edges(harness, selection.meshId, selection.edges);

    return selection;
}

[[nodiscard]] std::size_t active_face_count(
    const locus::editor::MeshNode& node)
{
    return locus::kernel::geometry::TopologyTraversal::faces(
        node.mesh()).size();
}

[[nodiscard]] std::size_t active_edge_count(
    const locus::editor::MeshNode& node)
{
    return locus::kernel::geometry::TopologyTraversal::edges(
        node.mesh()).size();
}

[[nodiscard]] std::size_t active_vertex_count(
    const locus::editor::MeshNode& node)
{
    return locus::kernel::geometry::TopologyTraversal::vertices(
        node.mesh()).size();
}

} // namespace

namespace locus::tests {

TestResult run_fill_hole_action_tests()
{
    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    ActionHarness harness;

    if (!register_default_actions(harness.registry)) {
        return TestResult::fail(
            "default action registration should succeed");
    }

    const ActionId fillHoleId =
        fill_hole_action_id();

    if (!harness.registry.contains(fillHoleId) ||
        harness.registry.find(fillHoleId) == nullptr) {
        return TestResult::fail(
            "Fill Hole action should be registered and discoverable");
    }

    const ActionDescriptor* descriptor =
        harness.registry.descriptor(fillHoleId);

    if (descriptor == nullptr ||
        descriptor->id != fillHoleId ||
        descriptor->name != "Fill Hole" ||
        descriptor->category != ActionCategory::Mesh) {
        return TestResult::fail(
            "Fill Hole descriptor should expose mesh action metadata");
    }

    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, fillHoleId)) {
            return TestResult::fail(
                "Fill Hole should be unavailable without an active mesh");
        }

        const ActionResult result =
            harness.executor.execute(context, fillHoleId);

        if (!result.is_unavailable() ||
            harness.history.undo_size() != 0u) {
            return TestResult::fail(
                "Unavailable Fill Hole should not change history");
        }
    }

    const QuadHoleSelection selection =
        create_quad_hole_selection(harness);
    MeshNode* node =
        harness.editor.scene().find_mesh(selection.meshId);

    if (node == nullptr) {
        return TestResult::fail(
            "Fill Hole fixture should create an active mesh node");
    }

    harness.editor.selection().set_granularity(
        SelectionGranularity::Face);
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, fillHoleId)) {
            return TestResult::fail(
                "Fill Hole should reject non-edge granularity");
        }
    }

    harness.editor.selection().set_granularity(
        SelectionGranularity::Edge);
    harness.editor.selection().mesh().clear_components();
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, fillHoleId)) {
            return TestResult::fail(
                "Fill Hole should reject zero selected edges");
        }
    }

    harness.editor.selection().mesh().add_edge(selection.edges[0]);
    harness.editor.selection().mesh().add_edge(selection.edges[1]);
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, fillHoleId)) {
            return TestResult::fail(
                "Fill Hole should reject fewer than three selected edges");
        }
    }

    harness.editor.selection().mesh().add_edge(selection.edges[2]);
    harness.editor.selection().mesh().add_edge(EdgeHandle{ 999u });
    {
        ActionContext context =
            harness.context();

        if (harness.executor.can_execute(context, fillHoleId)) {
            return TestResult::fail(
                "Fill Hole should reject invalid selected handles");
        }
    }

    select_edges(
        harness,
        selection.meshId,
        {
            selection.edges[2],
            selection.edges[0],
            selection.edges[3],
            selection.edges[1]
        });
    {
        const std::size_t verticesBefore = active_vertex_count(*node);
        const std::size_t edgesBefore = active_edge_count(*node);

        ActionContext context =
            harness.context();

        if (!harness.executor.can_execute(context, fillHoleId)) {
            return TestResult::fail(
                "Fill Hole should be available for four boundary edges");
        }

        const ActionResult result =
            harness.executor.execute(context, fillHoleId);

        const std::vector<FaceHandle> faces =
            TopologyTraversal::faces(node->mesh());

        if (!result.succeeded() ||
            harness.history.undo_size() != 1u ||
            harness.history.redo_size() != 0u ||
            faces.size() != 1u ||
            active_vertex_count(*node) != verticesBefore ||
            active_edge_count(*node) != edgesBefore ||
            TopologyTraversal::face_loops(
                node->mesh(),
                faces.front()).size() != 4u) {
            return TestResult::fail(
                "Fill Hole should create exactly one quad face through history");
        }

        for (EdgeHandle edge : selection.edges) {
            if (!harness.editor.selection().mesh().edges().contains(edge)) {
                return TestResult::fail(
                    "Fill Hole should preserve still-valid boundary edge selection");
            }
        }

        if (!TopologyValidator::validate(node->mesh()).valid()) {
            return TestResult::fail(
                "Fill Hole action result should leave valid topology");
        }
    }

    if (!harness.history.undo(harness.dispatcher) ||
        active_face_count(*node) != 0u ||
        harness.history.undo_size() != 0u ||
        harness.history.redo_size() != 1u) {
        return TestResult::fail(
            "Fill Hole undo should restore the hole");
    }

    if (!harness.history.redo(harness.dispatcher) ||
        active_face_count(*node) != 1u ||
        harness.history.undo_size() != 1u ||
        harness.history.redo_size() != 0u) {
        return TestResult::fail(
            "Fill Hole redo should restore the filled face");
    }

    {
        ActionHarness failingHarness;
        (void)register_default_actions(failingHarness.registry);
        const QuadHoleSelection failingSelection =
            create_open_edge_selection(failingHarness);
        MeshNode* failingNode =
            failingHarness.editor.scene().find_mesh(
                failingSelection.meshId);

        ActionContext context =
            failingHarness.context();

        if (!failingHarness.executor.can_execute(context, fillHoleId)) {
            return TestResult::fail(
                "Open boundary edge sets should reach kernel validation");
        }

        const ActionResult result =
            failingHarness.executor.execute(context, fillHoleId);

        if (!result.failed() ||
            failingHarness.history.undo_size() != 0u ||
            failingNode == nullptr ||
            active_face_count(*failingNode) != 0u) {
            return TestResult::fail(
                "Kernel-rejected Fill Hole should fail without history");
        }
    }

    return TestResult::pass();
}

} // namespace locus::tests
