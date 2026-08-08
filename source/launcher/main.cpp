/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"
#include "application/shortcut/Shortcut.h"
#include "application/tools/MeshToolActivationController.h"
#include "editor/EditorTypes.h"
#include "editor/command/CommandResult.h"
#include "editor/command/transform/NodeTransformChange.h"
#include "editor/command/transform/SetNodeTransformsCommand.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/NodeTransform.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/mesh/edge/EdgeSlideTool.h"
#include "editor/tools/mesh/face/SolidifyTool.h"
#include "editor/tools/mesh/topology/LoopCutTool.h"
#include "editor/tools/selection/SelectTool.h"
#include "editor/transform/TransformSession.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/topology/TopologyBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


//========manufacturing==============================
#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/core/IssueLocation.h"
#include "kernel/manufacturing/core/IssueMeasurement.h"
#include "kernel/manufacturing/core/IssueSeverity.h"
#include "kernel/manufacturing/core/PrintIssue.h"
#include "kernel/manufacturing/core/PrintIssueType.h"
#include "kernel/manufacturing/profiles/PrintTechnology.h"
#include "kernel/manufacturing/profiles/FDMProfile.h"
#include "kernel/manufacturing/profiles/ManufacturingLimits.h"
#include "kernel/manufacturing/profiles/PrintProfile.h"
#include "kernel/manufacturing/profiles/SLAProfile.h"
#include "kernel/manufacturing/profiles/SLSProfile.h"
#include "kernel/manufacturing/core/AnalysisContext.h"
#include "kernel/manufacturing/core/IAnalyzer.h"
#include "kernel/manufacturing/mesh/AnalysisMesh.h"
#include "kernel/manufacturing/mesh/AnalysisMeshBuilder.h"
#include "kernel/manufacturing/mesh/MeshHandleMapping.h"
#include "kernel/geometry/spatial/BVHQuery.h"
#include "kernel/math/Ray.h"
#include "kernel/manufacturing/analyzers/topology/ManifoldAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/WatertightAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/NormalConsistencyAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/OrientationAnalyzer.h"
#include "kernel/manufacturing/analyzers/topology/IslandAnalyzer.h"
#include "kernel/manufacturing/analyzers/geometry/DegenerateGeometryAnalyzer.h"
#include "kernel/manufacturing/analyzers/geometry/SelfIntersectionAnalyzer.h"
#include "kernel/manufacturing/analyzers/geometry/VolumeAnalyzer.h"
#include "kernel/manufacturing/analyzers/geometry/MinimumFeatureSizeAnalyzer.h"

#include "kernel/geometry/mesh/LEM.h"


//========manufacturing==============================


namespace {

    using locus::application::DocumentSession;
    using locus::editor::CommandResult;
    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::NodeTransform;
    using locus::editor::NodeTransformChange;
    using locus::editor::SceneNode;
    using locus::editor::SceneNodeId;
    using locus::editor::SetNodeTransformsCommand;
    using locus::editor::TransformSession;

    constexpr float SmokeEpsilon = 0.00001f;

    [[nodiscard]] bool nearly_equal(
        const glm::vec3& lhs,
        const glm::vec3& rhs)
    {
        return glm::length(lhs - rhs) <= SmokeEpsilon;
    }

    [[nodiscard]] bool same_position(
        const NodeTransform& transform,
        const glm::vec3& expected)
    {
        return nearly_equal(transform.position(), expected);
    }

    void print_position(
        std::string_view label,
        const NodeTransform& transform)
    {
        const glm::vec3& position = transform.position();
        std::cout
            << label
            << " position=("
            << position.x << ", "
            << position.y << ", "
            << position.z << ")\n";
    }

    [[nodiscard]] std::vector<NodeTransformChange> build_changes(
        const TransformSession& session)
    {
        std::vector<NodeTransformChange> changes{};
        changes.reserve(session.targets().size());

        for (const locus::editor::TransformTarget& target :
            session.targets()) {
            if (!target.has_transform_change()) {
                continue;
            }

            NodeTransformChange change{};
            change.node = target.node();
            change.previous = target.initial_transform();
            change.next = target.preview_transform();
            changes.push_back(std::move(change));
        }

        return changes;
    }

    [[nodiscard]] bool commit_session(
        DocumentSession& document,
        TransformSession& session,
        std::string_view label)
    {
        std::vector<NodeTransformChange> changes = build_changes(session);

        std::cout
            << label
            << " target count=" << session.targets().size()
            << " change count=" << changes.size()
            << " history before=" << document.history().undo_size()
            << '\n';

        if (changes.empty()) {
            const bool confirmed = session.confirm();
            std::cout
                << label
                << " no-op confirmed=" << confirmed
                << " history after=" << document.history().undo_size()
                << '\n';
            return confirmed;
        }

        locus::editor::ToolContext toolContext(
            document.editor(),
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());

        CommandResult result =
            toolContext.execute_command(
                std::make_unique<SetNodeTransformsCommand>(
                    std::move(changes)));

        if (!result.success) {
            std::cerr
                << label
                << " commit failed: " << result.message << '\n';
            return false;
        }

        const bool confirmed = session.confirm();

        if (locus::editor::has_flag(
                result.dirtyFlags,
                EditorDirtyFlags::Scene | EditorDirtyFlags::Mesh)) {
            document.mark_dirty();
        }

        std::cout
            << label
            << " committed=" << confirmed
            << " history after=" << document.history().undo_size()
            << " document dirty=" << document.is_dirty()
            << '\n';

        return confirmed;
    }

    [[nodiscard]] SceneNode& require_node(
        Editor& editor,
        SceneNodeId id,
        std::string_view label)
    {
        SceneNode* node = editor.scene().find_node(id);
        if (node == nullptr) {
            std::cerr << "Missing smoke node: " << label << '\n';
            std::abort();
        }

        return *node;
    }

    [[nodiscard]] bool run_transform_history_smoke_test()
    {
        DocumentSession document{ locus::application::DocumentId{ 1u } };
        Editor& editor = document.editor();

        const SceneNodeId cubeA = editor.scene().create_empty("Smoke A");
        const SceneNodeId cubeB = editor.scene().create_empty("Smoke B");
        const SceneNodeId parent = editor.scene().create_empty("Smoke Parent");
        const SceneNodeId child = editor.scene().create_empty("Smoke Child");

        if (!cubeA.is_valid() || !cubeB.is_valid() ||
            !parent.is_valid() || !child.is_valid() ||
            !editor.scene().reparent(child, parent)) {
            std::cerr << "Failed to create smoke scene.\n";
            return false;
        }

        require_node(editor, cubeB, "Smoke B")
            .transform()
            .set_position(glm::vec3{ 2.0f, 0.0f, 0.0f });

        require_node(editor, parent, "Smoke Parent")
            .transform()
            .set_position(glm::vec3{ 10.0f, 0.0f, 0.0f });

        require_node(editor, child, "Smoke Child")
            .transform()
            .set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });

        document.mark_saved({});

        std::cout << "Transform smoke test started.\n";
        std::cout
            << "history size before="
            << document.history().undo_size()
            << " document dirty=" << document.is_dirty()
            << '\n';

        editor.selection_controller().select_object(cubeA);

        TransformSession simpleSession{};
        if (!simpleSession.begin(editor.scene(), editor.selection())) {
            std::cerr << "Commit simple: session did not begin.\n";
            return false;
        }

        std::cout
            << "Commit simple: session active="
            << simpleSession.is_active()
            << " target count=" << simpleSession.targets().size()
            << '\n';

        const NodeTransform beforeA =
            require_node(editor, cubeA, "Smoke A").transform();

        print_position("Commit simple before", beforeA);

        for (int step = 0; step < 8; ++step) {
            if (!simpleSession.translate(
                    editor.scene(),
                    glm::vec3{ 0.125f, 0.0f, 0.0f })) {
                std::cerr << "Commit simple: preview update failed.\n";
                return false;
            }
        }

        const NodeTransform afterA =
            require_node(editor, cubeA, "Smoke A").transform();
        print_position("Commit simple after", afterA);

        if (!commit_session(document, simpleSession, "Commit simple")) {
            return false;
        }

        if (document.history().undo_size() != 1u ||
            !same_position(afterA, glm::vec3{ 1.0f, 0.0f, 0.0f })) {
            std::cerr << "Commit simple invariant failed.\n";
            return false;
        }

        CommandResult undoResult =
            document.history().undo(document.command_dispatcher());

        if (!undoResult.success) {
            std::cerr << "Undo failed: " << undoResult.message << '\n';
            return false;
        }

        document.mark_dirty();
        print_position(
            "Undo transform",
            require_node(editor, cubeA, "Smoke A").transform());

        if (!same_position(
                require_node(editor, cubeA, "Smoke A").transform(),
                beforeA.position())) {
            std::cerr << "Undo did not restore the original transform.\n";
            return false;
        }

        CommandResult redoResult =
            document.history().redo(document.command_dispatcher());

        if (!redoResult.success) {
            std::cerr << "Redo failed: " << redoResult.message << '\n';
            return false;
        }

        document.mark_dirty();
        print_position(
            "Redo transform",
            require_node(editor, cubeA, "Smoke A").transform());

        if (!same_position(
                require_node(editor, cubeA, "Smoke A").transform(),
                afterA.position())) {
            std::cerr << "Redo did not restore the final transform.\n";
            return false;
        }

        editor.selection_controller().select_object(cubeB);
        undoResult = document.history().undo(document.command_dispatcher());

        if (!undoResult.success ||
            !same_position(
                require_node(editor, cubeA, "Smoke A").transform(),
                beforeA.position())) {
            std::cerr
                << "Undo depended on current selection after commit.\n";
            return false;
        }

        redoResult = document.history().redo(document.command_dispatcher());
        if (!redoResult.success) {
            std::cerr << "Redo after selection change failed.\n";
            return false;
        }

        const std::size_t historyAfterCommit =
            document.history().undo_size();

        TransformSession cancelSession{};
        editor.selection().objects().set({ cubeA }, cubeA);

        if (!cancelSession.begin(editor.scene(), editor.selection()) ||
            !cancelSession.translate(
                editor.scene(),
                glm::vec3{ 5.0f, 0.0f, 0.0f })) {
            std::cerr << "Cancel: session setup failed.\n";
            return false;
        }

        const bool cancelled =
            cancelSession.cancel(editor.scene());

        std::cout
            << "Cancel: restored=" << cancelled
            << " history after cancel=" << document.history().undo_size()
            << '\n';

        if (!cancelled ||
            document.history().undo_size() != historyAfterCommit ||
            !same_position(
                require_node(editor, cubeA, "Smoke A").transform(),
                afterA.position())) {
            std::cerr << "Cancel invariant failed.\n";
            return false;
        }

        TransformSession noopSession{};
        if (!noopSession.begin(editor.scene(), editor.selection()) ||
            !commit_session(document, noopSession, "No-op")) {
            return false;
        }

        if (document.history().undo_size() != historyAfterCommit) {
            std::cerr << "No-op created an unnecessary history entry.\n";
            return false;
        }

        editor.selection().objects().set({ cubeA, cubeB }, cubeA);

        TransformSession multiSession{};
        if (!multiSession.begin(editor.scene(), editor.selection()) ||
            !multiSession.translate(
                editor.scene(),
                glm::vec3{ 0.0f, 2.0f, 0.0f }) ||
            !commit_session(document, multiSession, "Multiple objects")) {
            return false;
        }

        if (document.history().undo_size() != historyAfterCommit + 1u ||
            !same_position(
                require_node(editor, cubeA, "Smoke A").transform(),
                glm::vec3{ 1.0f, 2.0f, 0.0f }) ||
            !same_position(
                require_node(editor, cubeB, "Smoke B").transform(),
                glm::vec3{ 2.0f, 2.0f, 0.0f })) {
            std::cerr << "Multiple object commit invariant failed.\n";
            return false;
        }

        undoResult = document.history().undo(document.command_dispatcher());
        if (!undoResult.success ||
            !same_position(
                require_node(editor, cubeA, "Smoke A").transform(),
                glm::vec3{ 1.0f, 0.0f, 0.0f }) ||
            !same_position(
                require_node(editor, cubeB, "Smoke B").transform(),
                glm::vec3{ 2.0f, 0.0f, 0.0f })) {
            std::cerr << "Multiple object undo invariant failed.\n";
            return false;
        }

        redoResult = document.history().redo(document.command_dispatcher());
        if (!redoResult.success ||
            !same_position(
                require_node(editor, cubeA, "Smoke A").transform(),
                glm::vec3{ 1.0f, 2.0f, 0.0f }) ||
            !same_position(
                require_node(editor, cubeB, "Smoke B").transform(),
                glm::vec3{ 2.0f, 2.0f, 0.0f })) {
            std::cerr << "Multiple object redo invariant failed.\n";
            return false;
        }

        editor.selection().objects().set({ child }, child);

        TransformSession hierarchySession{};
        if (!hierarchySession.begin(editor.scene(), editor.selection()) ||
            !hierarchySession.translate(
                editor.scene(),
                glm::vec3{ 0.0f, 0.0f, 3.0f }) ||
            !commit_session(document, hierarchySession, "Hierarchy")) {
            return false;
        }

        if (!same_position(
                require_node(editor, parent, "Smoke Parent").transform(),
                glm::vec3{ 10.0f, 0.0f, 0.0f }) ||
            !same_position(
                require_node(editor, child, "Smoke Child").transform(),
                glm::vec3{ 1.0f, 0.0f, 3.0f })) {
            std::cerr << "Hierarchy local transform invariant failed.\n";
            return false;
        }

        const std::size_t historyBeforeFocusLost =
            document.history().undo_size();

        TransformSession focusLostSession{};
        if (!focusLostSession.begin(editor.scene(), editor.selection()) ||
            !focusLostSession.translate(
                editor.scene(),
                glm::vec3{ 0.0f, 4.0f, 0.0f }) ||
            !focusLostSession.cancel(editor.scene())) {
            std::cerr << "Focus lost cancellation failed.\n";
            return false;
        }

        std::cout
            << "Focus lost: capture released by modal tool path, "
            << "history after focus cancel="
            << document.history().undo_size()
            << '\n';

        if (document.history().undo_size() != historyBeforeFocusLost ||
            !same_position(
                require_node(editor, child, "Smoke Child").transform(),
                glm::vec3{ 1.0f, 0.0f, 3.0f })) {
            std::cerr << "Focus lost invariant failed.\n";
            return false;
        }

        std::cout
            << "Transform smoke test passed. Final history size="
            << document.history().undo_size()
            << " document dirty=" << document.is_dirty()
            << '\n';

        return true;
    }

    [[nodiscard]] locus::editor::ToolEvent make_mesh_pointer_event(
        locus::editor::ToolEventType type,
        const glm::vec3& target)
    {
        locus::editor::ToolEvent event{};
        event.type = type;
        event.button =
            type == locus::editor::ToolEventType::PointerPress ||
            type == locus::editor::ToolEventType::PointerRelease
            ? locus::editor::ToolPointerButton::Primary
            : locus::editor::ToolPointerButton::None;
        event.pointer.viewportPosition = glm::vec2{
            target.x,
            target.y
        };
        event.pointer.worldRay.origin = target + glm::vec3{ 0.0f, 0.0f, 4.0f };
        event.pointer.worldRay.direction = glm::vec3{ 0.0f, 0.0f, -1.0f };
        event.pointer.viewDirection = glm::vec3{ 0.0f, 0.0f, -1.0f };
        event.pointer.viewRight = glm::vec3{ 1.0f, 0.0f, 0.0f };
        event.pointer.viewUp = glm::vec3{ 0.0f, 1.0f, 0.0f };
        event.pointer.visualScale = 1.0f;
        return event;
    }

    [[nodiscard]] bool dispatch_mesh_smoke_event(
        DocumentSession& document,
        const locus::editor::ToolEvent& event,
        std::string_view label)
    {
        locus::editor::ToolContext context(
            document.editor(),
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());

        const locus::editor::ToolResult result =
            document.tool_manager().handle_event(context, event);

        if (result.failed()) {
            std::cerr << label << " failed: " << result.message << '\n';
            return false;
        }

        return true;
    }

    [[nodiscard]] bool selected_edges_are_valid(
        const locus::editor::Editor& editor,
        SceneNodeId meshId)
    {
        const locus::editor::MeshNode* node =
            editor.scene().find_mesh(meshId);

        if (node == nullptr) {
            return false;
        }

        for (const locus::kernel::geometry::EdgeHandle edge :
            editor.selection().mesh().edges().items()) {
            if (!node->mesh().is_valid(edge)) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] bool selected_faces_are_valid(
        const locus::editor::Editor& editor,
        SceneNodeId meshId)
    {
        const locus::editor::MeshNode* node =
            editor.scene().find_mesh(meshId);

        if (node == nullptr) {
            return false;
        }

        for (const locus::kernel::geometry::FaceHandle face :
            editor.selection().mesh().faces().items()) {
            if (!node->mesh().is_valid(face)) {
                return false;
            }
        }

        return true;
    }

    struct OpenCubeBridgeFixture {
        locus::kernel::geometry::EdgeHandle firstBridgeEdge{};
        locus::kernel::geometry::EdgeHandle secondBridgeEdge{};
    };

    [[nodiscard]] OpenCubeBridgeFixture build_open_top_cube_into(
        locus::kernel::geometry::LEM& mesh)
    {
        using locus::kernel::geometry::EdgeHandle;
        using locus::kernel::geometry::LEMEditor;
        using locus::kernel::geometry::VertexHandle;

        LEMEditor editor(mesh);

        const VertexHandle v0 =
            editor.add_vertex(glm::vec3{ -0.5f, -0.5f, -0.5f });
        const VertexHandle v1 =
            editor.add_vertex(glm::vec3{ 0.5f, -0.5f, -0.5f });
        const VertexHandle v2 =
            editor.add_vertex(glm::vec3{ 0.5f, 0.5f, -0.5f });
        const VertexHandle v3 =
            editor.add_vertex(glm::vec3{ -0.5f, 0.5f, -0.5f });
        const VertexHandle v4 =
            editor.add_vertex(glm::vec3{ -0.5f, -0.5f, 0.5f });
        const VertexHandle v5 =
            editor.add_vertex(glm::vec3{ 0.5f, -0.5f, 0.5f });
        const VertexHandle v6 =
            editor.add_vertex(glm::vec3{ 0.5f, 0.5f, 0.5f });
        const VertexHandle v7 =
            editor.add_vertex(glm::vec3{ -0.5f, 0.5f, 0.5f });

        (void)editor.add_face({ v0, v1, v2, v3 });
        (void)editor.add_face({ v0, v4, v5, v1 });
        (void)editor.add_face({ v1, v5, v6, v2 });
        (void)editor.add_face({ v2, v6, v7, v3 });
        (void)editor.add_face({ v3, v7, v4, v0 });

        OpenCubeBridgeFixture fixture{};
        fixture.firstBridgeEdge = mesh.find_edge(v4, v5);
        fixture.secondBridgeEdge = mesh.find_edge(v7, v6);

        if (fixture.secondBridgeEdge.is_invalid()) {
            fixture.secondBridgeEdge = mesh.find_edge(v6, v7);
        }

        return fixture;
    }

    [[nodiscard]] bool run_mesh_edit_smoke_test()
    {
        using locus::editor::SelectionGranularity;
        using locus::kernel::geometry::EdgeHandle;
        using locus::kernel::geometry::FaceHandle;
        using locus::kernel::geometry::TopologyTraversal;
        using locus::kernel::geometry::VertexHandle;

        DocumentSession document{ locus::application::DocumentId{ 2u } };
        Editor& editor = document.editor();

        const SceneNodeId meshId =
            editor.scene().create_mesh("Mesh Edit Smoke Cube");
        const SceneNodeId meshBId =
            editor.scene().create_mesh("Mesh Edit Smoke Cube B");
        locus::editor::MeshNode* meshNode =
            editor.scene().find_mesh(meshId);
        locus::editor::MeshNode* meshBNode =
            editor.scene().find_mesh(meshBId);

        if (meshNode == nullptr ||
            meshBNode == nullptr ||
            !locus::kernel::geometry::TopologyBuilder::build_box_into(
                meshNode->mesh()) ||
            !locus::kernel::geometry::TopologyBuilder::build_box_into(
                meshBNode->mesh())) {
            std::cerr << "Failed to create mesh edit smoke cubes.\n";
            return false;
        }

        meshBNode->transform().set_position(
            glm::vec3{ 4.0f, 0.0f, 0.0f });

        const std::vector<VertexHandle> vertices =
            TopologyTraversal::vertices(meshNode->mesh());
        const std::vector<EdgeHandle> edges =
            TopologyTraversal::edges(meshNode->mesh());
        const std::vector<FaceHandle> faces =
            TopologyTraversal::faces(meshNode->mesh());

        if (vertices.empty() || edges.empty() || faces.empty()) {
            std::cerr << "Mesh edit smoke cube has no editable components.\n";
            return false;
        }

        editor.selection_controller().select_object(meshId);
        if (editor.selection().granularity() !=
                SelectionGranularity::Object ||
            editor.selection().mesh().active_mesh().is_valid()) {
            std::cerr << "Object selection did not start in scene context.\n";
            return false;
        }

        editor.selection_controller().set_granularity(
            SelectionGranularity::Face);

        std::cout << "=== Mesh Edit Mode ===\n";
        if (editor.selection().mesh().active_mesh() != meshId ||
            editor.selection().scope() !=
            locus::editor::SelectionScope::ActiveMesh) {
            std::cerr << "Contextual mesh activation failed.\n";
            return false;
        }

        std::cout
            << "[OK] active mesh valido SceneNodeId="
            << meshId.value << '\n';

        locus::editor::ToolContext activateContext(
            editor,
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());
        (void)document.tool_manager().activate_tool(
            activateContext,
            locus::editor::ToolId{ locus::editor::SelectTool::Id });

        VertexHandle vertex = vertices.front();
        for (const VertexHandle candidate : vertices) {
            if (meshNode->mesh().vertex(candidate).position.z >
                meshNode->mesh().vertex(vertex).position.z) {
                vertex = candidate;
            }
        }
        const glm::vec3 vertexPosition =
            meshNode->mesh().vertex(vertex).position;

        editor.selection_controller().set_granularity(
            SelectionGranularity::Vertex);

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerMove,
                    vertexPosition),
                "Vertex hover") ||
            editor.selection().mesh().hovered_vertex() != vertex ||
            !editor.selection().mesh().empty()) {
            std::cerr << "Vertex hover invariant failed.\n";
            return false;
        }

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerPress,
                    vertexPosition),
                "Vertex select") ||
            !editor.selection().mesh().vertices().contains(vertex)) {
            std::cerr << "Vertex selection invariant failed.\n";
            return false;
        }

        std::cout << "=== Vertex picking ===\n";
        std::cout
            << "[OK] hovered vertex handle=" << vertex.id.value << '\n'
            << "[OK] selected vertex handle=" << vertex.id.value << '\n';

        locus::editor::ToolEvent toggleVertex =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerPress,
                vertexPosition);
        toggleVertex.modifiers = locus::editor::ToolModifiers::Toggle;

        if (!dispatch_mesh_smoke_event(document, toggleVertex, "Vertex toggle") ||
            editor.selection().mesh().vertices().contains(vertex)) {
            std::cerr << "Vertex toggle invariant failed.\n";
            return false;
        }

        EdgeHandle edge = edges.front();
        for (const EdgeHandle candidate : edges) {
            const auto& candidateData = meshNode->mesh().edge(candidate);
            const glm::vec3 a =
                meshNode->mesh().vertex(candidateData.vertexA).position;
            const glm::vec3 b =
                meshNode->mesh().vertex(candidateData.vertexB).position;

            if (a.z > 0.0f && b.z > 0.0f) {
                edge = candidate;
                break;
            }
        }
        const auto& edgeData = meshNode->mesh().edge(edge);
        const glm::vec3 edgePosition =
            (meshNode->mesh().vertex(edgeData.vertexA).position +
                meshNode->mesh().vertex(edgeData.vertexB).position) *
            0.5f;

        editor.selection_controller().set_granularity(
            SelectionGranularity::Edge);

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerMove,
                    edgePosition),
                "Edge hover") ||
            editor.selection().mesh().hovered_edge() != edge) {
            std::cerr << "Edge hover invariant failed.\n";
            return false;
        }

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerPress,
                    edgePosition),
                "Edge select") ||
            !editor.selection().mesh().edges().contains(edge)) {
            std::cerr << "Edge selection invariant failed.\n";
            return false;
        }

        std::cout << "=== Edge picking ===\n";
        std::cout
            << "[OK] hovered edge handle=" << edge.id.value << '\n'
            << "[OK] selected edge handle=" << edge.id.value << '\n';

        const auto edgeADataBefore =
            meshNode->mesh().edge(edge);
        const glm::vec3 edgeAVertexABefore =
            meshNode->mesh().vertex(edgeADataBefore.vertexA).position;
        const glm::vec3 edgeAVertexBBefore =
            meshNode->mesh().vertex(edgeADataBefore.vertexB).position;
        const std::size_t historyBeforeEdgeSlide =
            document.history().undo_size();

        locus::editor::ToolContext edgeSlideActivateContext(
            editor,
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());

        locus::editor::ToolResult edgeSlideActivation =
            document.tool_manager().activate_tool(
                edgeSlideActivateContext,
                locus::editor::ToolId{
                    std::string{
                        locus::editor::EdgeSlideTool::Id } });

        if (edgeSlideActivation.failed()) {
            std::cerr
                << "EdgeSlide activation failed: "
                << edgeSlideActivation.message << '\n';
            return false;
        }

        locus::editor::ToolEvent edgeSlidePress =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerPress,
                edgePosition);
        locus::editor::ToolEvent edgeSlideMove =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerMove,
                edgePosition + glm::vec3{ 0.0f, 0.35f, 0.0f });
        locus::editor::ToolEvent edgeSlideRelease =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerRelease,
                edgePosition + glm::vec3{ 0.0f, 0.35f, 0.0f });

        if (!dispatch_mesh_smoke_event(
                document,
                edgeSlidePress,
                "EdgeSlide press") ||
            !dispatch_mesh_smoke_event(
                document,
                edgeSlideMove,
                "EdgeSlide preview") ||
            !dispatch_mesh_smoke_event(
                document,
                edgeSlideRelease,
                "EdgeSlide commit")) {
            return false;
        }

        const glm::vec3 edgeAVertexAAfter =
            meshNode->mesh().vertex(edgeADataBefore.vertexA).position;
        const glm::vec3 edgeAVertexBAfter =
            meshNode->mesh().vertex(edgeADataBefore.vertexB).position;

        if (document.history().undo_size() != historyBeforeEdgeSlide + 1u ||
            (nearly_equal(edgeAVertexABefore, edgeAVertexAAfter) &&
                nearly_equal(edgeAVertexBBefore, edgeAVertexBAfter)) ||
            !meshNode->mesh().is_valid(edge) ||
            !editor.selection().mesh().edges().contains(edge)) {
            std::cerr << "EdgeSlide commit invariant failed.\n";
            return false;
        }

        CommandResult edgeSlideUndoResult =
            document.history().undo(document.command_dispatcher());

        if (!edgeSlideUndoResult.success ||
            !nearly_equal(
                meshNode->mesh().vertex(edgeADataBefore.vertexA).position,
                edgeAVertexABefore) ||
            !nearly_equal(
                meshNode->mesh().vertex(edgeADataBefore.vertexB).position,
                edgeAVertexBBefore) ||
            !meshNode->mesh().is_valid(edge) ||
            !editor.selection().mesh().edges().contains(edge)) {
            std::cerr << "EdgeSlide undo invariant failed.\n";
            return false;
        }

        CommandResult edgeSlideRedoResult =
            document.history().redo(document.command_dispatcher());

        if (!edgeSlideRedoResult.success ||
            !nearly_equal(
                meshNode->mesh().vertex(edgeADataBefore.vertexA).position,
                edgeAVertexAAfter) ||
            !nearly_equal(
                meshNode->mesh().vertex(edgeADataBefore.vertexB).position,
                edgeAVertexBAfter) ||
            !meshNode->mesh().is_valid(edge) ||
            !editor.selection().mesh().edges().contains(edge)) {
            std::cerr << "EdgeSlide redo invariant failed.\n";
            return false;
        }

        const std::vector<EdgeHandle> meshBEdges =
            TopologyTraversal::edges(meshBNode->mesh());
        if (meshBEdges.empty()) {
            std::cerr << "Mesh B has no edge for EdgeSlide cancel smoke.\n";
            return false;
        }

        const EdgeHandle meshBEdge =
            meshBEdges.front();
        const auto meshBEdgeData =
            meshBNode->mesh().edge(meshBEdge);
        const glm::vec3 meshBEdgeAOriginal =
            meshBNode->mesh().vertex(meshBEdgeData.vertexA).position;
        const glm::vec3 meshBEdgeBOriginal =
            meshBNode->mesh().vertex(meshBEdgeData.vertexB).position;

        if (!editor.selection_controller().enter_mesh_context(
                meshBId,
                SelectionGranularity::Edge) ||
            !editor.selection_controller().select_edge(meshBEdge)) {
            std::cerr << "Failed to switch EdgeSlide smoke to Mesh B.\n";
            return false;
        }

        locus::editor::ToolContext edgeSlideBActivateContext(
            editor,
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());

        edgeSlideActivation =
            document.tool_manager().activate_tool(
                edgeSlideBActivateContext,
                locus::editor::ToolId{
                    std::string{
                        locus::editor::EdgeSlideTool::Id } });

        if (edgeSlideActivation.failed()) {
            std::cerr
                << "EdgeSlide Mesh B activation failed: "
                << edgeSlideActivation.message << '\n';
            return false;
        }

        const glm::vec3 meshBEdgePosition =
            (meshBEdgeAOriginal + meshBEdgeBOriginal) *
            0.5f +
            meshBNode->transform().position();

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerPress,
                    meshBEdgePosition),
                "EdgeSlide Mesh B press") ||
            !dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerMove,
                    meshBEdgePosition + glm::vec3{ 0.0f, 0.25f, 0.0f }),
                "EdgeSlide Mesh B preview")) {
            return false;
        }

        locus::editor::ToolEvent edgeSlideCancel{};
        edgeSlideCancel.type =
            locus::editor::ToolEventType::Cancel;

        if (!dispatch_mesh_smoke_event(
                document,
                edgeSlideCancel,
                "EdgeSlide Mesh B cancel") ||
            document.history().undo_size() != historyBeforeEdgeSlide + 1u ||
            !nearly_equal(
                meshBNode->mesh().vertex(meshBEdgeData.vertexA).position,
                meshBEdgeAOriginal) ||
            !nearly_equal(
                meshBNode->mesh().vertex(meshBEdgeData.vertexB).position,
                meshBEdgeBOriginal) ||
            !meshBNode->mesh().is_valid(meshBEdge) ||
            !editor.selection().mesh().edges().contains(meshBEdge)) {
            std::cerr << "EdgeSlide cancel invariant failed.\n";
            return false;
        }

        std::cout << "=== EdgeSlideTool ===\n";
        std::cout
            << "[OK] G activates EdgeSlide in Edge context\n"
            << "[OK] preview committed one history entry\n"
            << "[OK] Ctrl+Z/Ctrl+Y equivalent undo/redo restored slide\n"
            << "[OK] Mesh B cancel preserved mesh and valid edge handle\n";

        const std::size_t meshBVertexCountBeforeLoopCut =
            TopologyTraversal::vertices(meshBNode->mesh()).size();
        const std::size_t historyBeforeLoopCut =
            document.history().undo_size();

        const locus::application::ApplicationResult<bool> loopCutActivation =
            locus::application::MeshToolActivationController{}
                .activate_shortcut(
                    locus::application::ShortcutAction::ActivateLoopCutTool,
                    document);

        if (!loopCutActivation ||
            !loopCutActivation.value()) {
            std::cerr << "LoopCut shortcut activation failed";

            if (!loopCutActivation) {
                std::cerr << ": "
                    << loopCutActivation.error().message;
            }

            std::cerr << '\n';
            return false;
        }

        const glm::vec3 meshBLoopMove =
            meshBEdgePosition +
            glm::normalize(
                meshBEdgeBOriginal -
                meshBEdgeAOriginal) *
            0.2f;

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerPress,
                    meshBEdgePosition),
                "LoopCut Mesh B press") ||
            !dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerMove,
                    meshBLoopMove),
                "LoopCut Mesh B preview") ||
            !dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerRelease,
                    meshBLoopMove),
                "LoopCut Mesh B commit")) {
            return false;
        }

        const std::size_t meshBVertexCountAfterLoopCut =
            TopologyTraversal::vertices(meshBNode->mesh()).size();

        if (document.history().undo_size() != historyBeforeLoopCut + 1u ||
            meshBVertexCountAfterLoopCut <= meshBVertexCountBeforeLoopCut ||
            !selected_edges_are_valid(editor, meshBId)) {
            std::cerr << "LoopCut commit invariant failed.\n";
            return false;
        }

        CommandResult loopCutUndoResult =
            document.history().undo(document.command_dispatcher());

        if (!loopCutUndoResult.success ||
            TopologyTraversal::vertices(meshBNode->mesh()).size() !=
                meshBVertexCountBeforeLoopCut ||
            !selected_edges_are_valid(editor, meshBId)) {
            std::cerr << "LoopCut undo invariant failed.\n";
            return false;
        }

        CommandResult loopCutRedoResult =
            document.history().redo(document.command_dispatcher());

        if (!loopCutRedoResult.success ||
            TopologyTraversal::vertices(meshBNode->mesh()).size() !=
                meshBVertexCountAfterLoopCut ||
            !selected_edges_are_valid(editor, meshBId)) {
            std::cerr << "LoopCut redo invariant failed.\n";
            return false;
        }

        const EdgeHandle meshALoopCancelEdge =
            TopologyTraversal::edges(meshNode->mesh()).front();
        const auto meshALoopCancelEdgeData =
            meshNode->mesh().edge(meshALoopCancelEdge);
        const glm::vec3 meshALoopCancelPosition =
            (meshNode->mesh().vertex(meshALoopCancelEdgeData.vertexA).position +
                meshNode->mesh().vertex(meshALoopCancelEdgeData.vertexB).position) *
            0.5f;
        const std::size_t meshAVertexCountBeforeLoopCancel =
            TopologyTraversal::vertices(meshNode->mesh()).size();
        const std::size_t historyBeforeLoopCancel =
            document.history().undo_size();

        if (!editor.selection_controller().enter_mesh_context(
                meshId,
                SelectionGranularity::Edge) ||
            !editor.selection_controller().select_edge(meshALoopCancelEdge)) {
            std::cerr << "Failed to prepare Mesh A for LoopCut cancel.\n";
            return false;
        }

        const locus::application::ApplicationResult<bool>
            loopCutCancelActivation =
                locus::application::MeshToolActivationController{}
                    .activate_shortcut(
                        locus::application::ShortcutAction::ActivateLoopCutTool,
                        document);

        if (!loopCutCancelActivation ||
            !loopCutCancelActivation.value()) {
            std::cerr << "LoopCut cancel activation failed";

            if (!loopCutCancelActivation) {
                std::cerr << ": "
                    << loopCutCancelActivation.error().message;
            }

            std::cerr << '\n';
            return false;
        }

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerPress,
                    meshALoopCancelPosition),
                "LoopCut Mesh A cancel press") ||
            !dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerMove,
                    meshALoopCancelPosition + glm::vec3{ 0.15f, 0.0f, 0.0f }),
                "LoopCut Mesh A cancel preview") ||
            !dispatch_mesh_smoke_event(
                document,
                edgeSlideCancel,
                "LoopCut Mesh A cancel")) {
            return false;
        }

        if (document.history().undo_size() != historyBeforeLoopCancel ||
            TopologyTraversal::vertices(meshNode->mesh()).size() !=
                meshAVertexCountBeforeLoopCancel ||
            !meshNode->mesh().is_valid(meshALoopCancelEdge) ||
            !editor.selection().mesh().edges().contains(
                meshALoopCancelEdge) ||
            !selected_edges_are_valid(editor, meshId)) {
            std::cerr << "LoopCut cancel invariant failed.\n";
            return false;
        }

        std::cout << "=== LoopCutTool ===\n";
        std::cout
            << "[OK] R activates LoopCut in Edge context\n"
            << "[OK] preview committed one topological history entry\n"
            << "[OK] undo/redo restored loop cut topology\n"
            << "[OK] cancel preserved mesh and valid edge selection\n";

        locus::editor::ToolContext selectReactivateContext(
            editor,
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());
        (void)document.tool_manager().activate_tool(
            selectReactivateContext,
            locus::editor::ToolId{ locus::editor::SelectTool::Id });

        if (!editor.selection_controller().enter_mesh_context(
                meshId,
                SelectionGranularity::Face)) {
            std::cerr
                << "Failed to restore Mesh A face context after "
                << "EdgeSlide smoke.\n";
            return false;
        }

        FaceHandle face = faces.front();
        for (const FaceHandle candidate : faces) {
            if (meshNode->mesh().face(candidate).normal.z > 0.5f) {
                face = candidate;
                break;
            }
        }
        const std::vector<VertexHandle> faceVertices =
            TopologyTraversal::face_vertices(meshNode->mesh(), face);
        glm::vec3 facePosition{ 0.0f, 0.0f, 0.0f };
        for (const VertexHandle handle : faceVertices) {
            facePosition += meshNode->mesh().vertex(handle).position;
        }
        facePosition /= static_cast<float>(faceVertices.size());

        editor.selection_controller().set_granularity(
            SelectionGranularity::Face);

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerMove,
                    facePosition),
                "Face hover") ||
            editor.selection().mesh().hovered_face() != face) {
            std::cerr << "Face hover invariant failed.\n";
            return false;
        }

        if (!dispatch_mesh_smoke_event(
                document,
                make_mesh_pointer_event(
                    locus::editor::ToolEventType::PointerPress,
                    facePosition),
                "Face select") ||
            !editor.selection().mesh().faces().contains(face)) {
            std::cerr << "Face selection invariant failed.\n";
            return false;
        }

        std::cout << "=== Face picking ===\n";
        std::cout
            << "[OK] triangulo resolve para FaceHandle="
            << face.id.value << '\n';

        const CommandResult undoResult =
            document.history().undo(document.command_dispatcher());
        const CommandResult redoResult =
            document.history().redo(document.command_dispatcher());

        if (!undoResult.success ||
            !redoResult.success ||
            !editor.selection().mesh().faces().contains(face)) {
            std::cerr << "Selection undo/redo invariant failed.\n";
            return false;
        }

        std::cout << "=== Selection history ===\n";
        std::cout << "[OK] undo\n[OK] redo\n";

        const std::size_t vertexCountBeforeSolidify =
            TopologyTraversal::vertices(meshNode->mesh()).size();
        const std::size_t faceCountBeforeSolidify =
            TopologyTraversal::faces(meshNode->mesh()).size();
        const std::size_t historyBeforeSolidify =
            document.history().undo_size();

        const locus::application::ApplicationResult<bool> solidifyActivation =
            locus::application::MeshToolActivationController{}
                .activate_shortcut(
                    locus::application::ShortcutAction::ActivateSolidifyTool,
                    document);

        if (!solidifyActivation ||
            !solidifyActivation.value()) {
            std::cerr << "Solidify shortcut activation failed";
            if (!solidifyActivation) {
                std::cerr
                    << ": "
                    << solidifyActivation.error().message;
            }
            std::cerr << '\n';
            return false;
        }

        locus::editor::ToolEvent solidifyPress =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerPress,
                facePosition);
        locus::editor::ToolEvent solidifyMove =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerMove,
                facePosition + glm::vec3{ 0.0f, -0.35f, 0.0f });
        locus::editor::ToolEvent solidifyRelease =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerRelease,
                facePosition + glm::vec3{ 0.0f, -0.35f, 0.0f });

        if (!dispatch_mesh_smoke_event(
                document,
                solidifyPress,
                "Solidify press") ||
            !dispatch_mesh_smoke_event(
                document,
                solidifyMove,
                "Solidify preview") ||
            !dispatch_mesh_smoke_event(
                document,
                solidifyRelease,
                "Solidify commit")) {
            return false;
        }

        const std::size_t vertexCountAfterSolidify =
            TopologyTraversal::vertices(meshNode->mesh()).size();
        const std::size_t faceCountAfterSolidify =
            TopologyTraversal::faces(meshNode->mesh()).size();

        if (document.history().undo_size() != historyBeforeSolidify + 1u ||
            vertexCountAfterSolidify <= vertexCountBeforeSolidify ||
            faceCountAfterSolidify <= faceCountBeforeSolidify ||
            (meshNode->mesh().is_valid(face) &&
                !editor.selection().mesh().faces().contains(face)) ||
            !selected_faces_are_valid(editor, meshId)) {
            std::cerr << "Solidify commit invariant failed.\n";
            return false;
        }

        CommandResult solidifyUndoResult =
            document.history().undo(document.command_dispatcher());

        if (!solidifyUndoResult.success ||
            TopologyTraversal::vertices(meshNode->mesh()).size() !=
                vertexCountBeforeSolidify ||
            TopologyTraversal::faces(meshNode->mesh()).size() !=
                faceCountBeforeSolidify ||
            !meshNode->mesh().is_valid(face) ||
            !editor.selection().mesh().faces().contains(face)) {
            std::cerr << "Solidify undo invariant failed.\n";
            return false;
        }

        CommandResult solidifyRedoResult =
            document.history().redo(document.command_dispatcher());

        if (!solidifyRedoResult.success ||
            TopologyTraversal::vertices(meshNode->mesh()).size() !=
                vertexCountAfterSolidify ||
            TopologyTraversal::faces(meshNode->mesh()).size() !=
                faceCountAfterSolidify ||
            (meshNode->mesh().is_valid(face) &&
                !editor.selection().mesh().faces().contains(face)) ||
            !selected_faces_are_valid(editor, meshId)) {
            std::cerr << "Solidify redo invariant failed.\n";
            return false;
        }

        solidifyUndoResult =
            document.history().undo(document.command_dispatcher());

        if (!solidifyUndoResult.success ||
            !meshNode->mesh().is_valid(face) ||
            !editor.selection().mesh().faces().contains(face)) {
            std::cerr << "Solidify cancel setup undo invariant failed.\n";
            return false;
        }

        const std::size_t historyBeforeSolidifyCancel =
            document.history().undo_size();
        const std::size_t vertexCountBeforeSolidifyCancel =
            TopologyTraversal::vertices(meshNode->mesh()).size();
        const std::size_t faceCountBeforeSolidifyCancel =
            TopologyTraversal::faces(meshNode->mesh()).size();

        const locus::application::ApplicationResult<bool>
            solidifyCancelActivation =
            locus::application::MeshToolActivationController{}
                .activate_shortcut(
                    locus::application::ShortcutAction::ActivateSolidifyTool,
                    document);

        if (!solidifyCancelActivation ||
            !solidifyCancelActivation.value()) {
            std::cerr << "Solidify cancel activation failed";
            if (!solidifyCancelActivation) {
                std::cerr
                    << ": "
                    << solidifyCancelActivation.error().message;
            }
            std::cerr << '\n';
            return false;
        }

        locus::editor::ToolEvent solidifyCancel{};
        solidifyCancel.type =
            locus::editor::ToolEventType::Cancel;

        if (!dispatch_mesh_smoke_event(
                document,
                solidifyPress,
                "Solidify cancel press") ||
            !dispatch_mesh_smoke_event(
                document,
                solidifyMove,
                "Solidify cancel preview") ||
            !dispatch_mesh_smoke_event(
                document,
                solidifyCancel,
                "Solidify cancel")) {
            return false;
        }

        if (document.history().undo_size() !=
                historyBeforeSolidifyCancel ||
            TopologyTraversal::vertices(meshNode->mesh()).size() !=
                vertexCountBeforeSolidifyCancel ||
            TopologyTraversal::faces(meshNode->mesh()).size() !=
                faceCountBeforeSolidifyCancel ||
            !meshNode->mesh().is_valid(face) ||
            !editor.selection().mesh().faces().contains(face) ||
            !selected_faces_are_valid(editor, meshId)) {
            std::cerr << "Solidify cancel invariant failed.\n";
            return false;
        }

        std::cout << "=== SolidifyTool ===\n";
        std::cout
            << "[OK] F activates Solidify in Face context\n"
            << "[OK] preview committed one topological history entry\n"
            << "[OK] undo/redo restored solidify topology\n"
            << "[OK] cancel preserved mesh and valid face selection\n";

        locus::editor::ToolContext solidifySelectContext(
            editor,
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());
        const locus::editor::ToolResult selectActivation =
            document.tool_manager().activate_tool(
                solidifySelectContext,
                locus::editor::ToolId{ locus::editor::SelectTool::Id });

        if (selectActivation.code ==
            locus::editor::ToolResultCode::Failed) {
            std::cerr << "Select activation before clear click failed";
            if (!selectActivation.message.empty()) {
                std::cerr
                    << ": "
                    << selectActivation.message;
            }
            std::cerr << '\n';
            return false;
        }

        locus::editor::ToolEvent emptyClick =
            make_mesh_pointer_event(
                locus::editor::ToolEventType::PointerPress,
                glm::vec3{ 20.0f, 20.0f, 20.0f });

        if (!dispatch_mesh_smoke_event(document, emptyClick, "Clear click") ||
            !editor.selection().mesh().empty() ||
            editor.selection().mesh().active_mesh() != meshId) {
            std::cerr << "Empty click clear invariant failed.\n";
            return false;
        }

        editor.selection_controller().set_granularity(
            SelectionGranularity::Object);

        if (editor.selection().mesh().active_mesh().is_valid()) {
            std::cerr << "Returning to Objects did not clear active mesh.\n";
            return false;
        }

        editor.selection_controller().select_object(meshBId);
        editor.selection_controller().set_granularity(
            SelectionGranularity::Face);

        if (editor.selection().mesh().active_mesh() != meshBId ||
            !editor.selection().mesh().empty() ||
            editor.selection().mesh().hovered_face().is_valid()) {
            std::cerr
                << "Switching active mesh left stale component state.\n";
            return false;
        }

        std::cout << "[OK] clear preservou active mesh durante Mesh Mode\n";
        std::cout << "[OK] voltar para Objects limpou active mesh\n";
        std::cout
            << "[OK] entrar no contexto de Mesh B limpou componentes de A\n";
        return true;
    }

    [[nodiscard]] bool seed_demo_scene(
        locus::application::ApplicationRuntime& runtime)
    {
        locus::application::DocumentSession* document =
            runtime.documents().active_document();

        if (document == nullptr) {
            std::cerr << "No active document available.\n";
            return false;
        }

        locus::editor::Editor& editor = document->editor();
        const locus::editor::SceneNodeId cubeAId =
            editor.scene().create_mesh("Cube A");
        const locus::editor::SceneNodeId cubeBId =
            editor.scene().create_mesh("Cube B");

        locus::editor::MeshNode* cubeA =
            editor.scene().find_mesh(cubeAId);
        locus::editor::MeshNode* cubeB =
            editor.scene().find_mesh(cubeBId);

        if (cubeA == nullptr || cubeB == nullptr) {
            std::cerr << "Failed to create demo cubes.\n";
            return false;
        }

        const OpenCubeBridgeFixture cubeABridgeFixture =
            build_open_top_cube_into(cubeA->mesh());
        const auto cubeBResult =
            locus::kernel::geometry::TopologyBuilder::build_box_into(
                cubeB->mesh());

        if (cubeABridgeFixture.firstBridgeEdge.is_invalid() ||
            cubeABridgeFixture.secondBridgeEdge.is_invalid() ||
            !cubeBResult) {
            std::cerr << "Failed to build demo cubes.\n";
            return false;
        }

        cubeA->transform().set_position(
            glm::vec3{ -1.4f, 0.0f, 0.0f });
        cubeB->transform().set_position(
            glm::vec3{ 1.4f, 0.0f, 0.0f });

        if (!editor.selection_controller().enter_mesh_context(
                cubeAId,
                locus::editor::SelectionGranularity::Edge) ||
            !editor.selection_controller().select_edge(
                cubeABridgeFixture.firstBridgeEdge) ||
            !editor.selection_controller().toggle_edge(
                cubeABridgeFixture.secondBridgeEdge)) {
            std::cerr
                << "Failed to preselect Bridge Edge smoke edges.\n";
            return false;
        }

        editor.mark_dirty(
            locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
                locus::editor::EditorDirtyFlags::Picking);

        std::cout
            << "Interactive selection/context checkpoint started.\n"
            << "Cube A SceneNodeId=" << cubeAId.value
            << " at x=-1.4, open top face for Bridge Edge testing\n"
            << "Cube B SceneNodeId=" << cubeBId.value
            << " at x=1.4\n"
            << "Bridge Edge test is preselected on Cube A: "
            << "EdgeHandle "
            << cubeABridgeFixture.firstBridgeEdge.id.value
            << " + "
            << cubeABridgeFixture.secondBridgeEdge.id.value
            << ". Press J to create a bridge across the opening, "
            << "then Ctrl+Z/Ctrl+Y to undo/redo.\n"
            << "Keys: 1 Object, 2 Vertex, 3 Edge, 4 Face, "
            << "Q Select, E Extrude in Face context, "
            << "F Solidify in Face context, "
            << "G EdgeSlide in Edge context, "
            << "J Bridge Edge in Edge context, "
            << "R LoopCut in Edge context, "
            << "I Inset in Face context, "
            << "W/E/R/T transform gizmo in Object context, "
            << "Esc Cancel, Ctrl+Z Undo, Ctrl+Shift+Z Redo.\n"
            << "Test path: press 1 and click Cube A; press 4 and click "
            << "a face on Cube A; press 1 and click Cube B; press 4 "
            << "and click a face on Cube B. For Extrude: keep Face "
            << "granularity, press E, press-drag-release on the selected "
            << "face, then use Ctrl+Z/Ctrl+Shift+Z or start another "
            << "Extrude and press Esc before release. For Inset: keep Face "
            << "granularity, press I, press-drag inward on the selected "
            << "face, release to commit, then use Ctrl+Z/Ctrl+Y or start "
            << "another Inset and press Esc before release. For Solidify: "
            << "keep Face granularity, press F, press-drag along the face "
            << "normal, release to commit, then use Ctrl+Z/Ctrl+Y or start "
            << "another Solidify and press Esc before release. For EdgeSlide: "
            << "press 3, click an edge on Cube A, press G, press-drag "
            << "along an adjacent rail, release to commit, then use "
            << "Ctrl+Z/Ctrl+Y or start another EdgeSlide and press Esc "
            << "before release. For LoopCut: press 3, click an edge, "
            << "press R, press-drag along the edge to position the cut, "
            << "release to commit, then use Ctrl+Z/Ctrl+Y or start "
            << "another LoopCut and press Esc before release.\n"
            << "Watch [selection] logs for activeMesh, component counts, "
            << "stale hover/handle values, and [tool] logs for preview, "
            << "commit, cancel, and history.\n";

        return true;
    }

    [[nodiscard]] int run_application()
    {
        locus::application::ApplicationRuntime runtime{};

        const locus::application::ApplicationResult<void> initializeResult =
            runtime.initialize();

        if (!initializeResult) {
            std::cerr << initializeResult.error().message << '\n';
            return 1;
        }

        if (!seed_demo_scene(runtime)) {
            runtime.shutdown();
            return 1;
        }

        const locus::application::ApplicationResult<int> runResult =
            runtime.run();

        if (!runResult) {
            std::cerr << runResult.error().message << '\n';
            return 1;
        }

        return runResult.value();
    }

} // namespace

int main(int argc, char** argv)
{

    //=============================================================================
  // Manufacturing MinimumFeatureSizeAnalyzer Smoke Test
  //=============================================================================

    {
        using namespace locus::kernel;
        using namespace locus::kernel::geometry;
        using namespace locus::kernel::manufacturing;

        std::cout
            << "\n=== Locus3D Manufacturing MinimumFeatureSizeAnalyzer Smoke Test ===\n\n";

        MinimumFeatureSizeAnalyzer analyzer;

        //-------------------------------------------------------------------------
        // Metadata
        //-------------------------------------------------------------------------

        std::cout << "=== Analyzer metadata ===\n";

        if (analyzer.name() == "MinimumFeatureSizeAnalyzer") {
            std::cout
                << "[OK] analyzer possui nome estavel\n";
        }
        else {
            std::cout
                << "[FAIL] nome do analyzer inconsistente\n";
        }

        //-------------------------------------------------------------------------
        // Missing dependencies
        //-------------------------------------------------------------------------

        std::cout << "\n=== Missing dependencies ===\n";

        AnalysisContext missingContext;
        AnalysisReport missingReport;

        analyzer.analyze(
            missingContext,
            missingReport);

        if (!missingReport.has_issues()) {
            std::cout
                << "[OK] contexto vazio nao gera feature issue\n";
        }
        else {
            std::cout
                << "[FAIL] analyzer executou sem dependencias\n";
        }

        LEM noProfileMesh;

        const VertexHandle np0 =
            noProfileMesh.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle np1 =
            noProfileMesh.add_vertex(
                glm::vec3{ 0.1f, 0.0f, 0.0f });

        noProfileMesh.find_or_create_edge(
            np0,
            np1);

        AnalysisContext noProfileContext;
        noProfileContext.mesh =
            &noProfileMesh;

        AnalysisReport noProfileReport;

        analyzer.analyze(
            noProfileContext,
            noProfileReport);

        if (!noProfileReport.has_issues()) {
            std::cout
                << "[OK] ausencia de PrintProfile impede comparacao arbitraria\n";
        }
        else {
            std::cout
                << "[FAIL] analyzer inventou limite sem profile\n";
        }

        //-------------------------------------------------------------------------
        // Profile without feature-size limit
        //-------------------------------------------------------------------------

        std::cout << "\n=== Profile without limit ===\n";

        FDMProfile noLimitFdm;
        noLimitFdm.name =
            "No feature limit";

        PrintProfile noLimitProfile{
            noLimitFdm
        };

        AnalysisContext noLimitContext;
        noLimitContext.mesh =
            &noProfileMesh;
        noLimitContext.profile =
            &noLimitProfile;

        AnalysisReport noLimitReport;

        analyzer.analyze(
            noLimitContext,
            noLimitReport);

        if (!noLimitReport.has_issues()) {
            std::cout
                << "[OK] limite ausente nao e interpretado como zero\n";
        }
        else {
            std::cout
                << "[FAIL] profile sem limite gerou issue\n";
        }

        //-------------------------------------------------------------------------
        // Configured profile
        //-------------------------------------------------------------------------

        FDMProfile fdm;
        fdm.name =
            "Minimum Feature Test";

        fdm.limits.minimumFeatureSize =
            0.5;

        PrintProfile profile{ fdm };

        //-------------------------------------------------------------------------
        // Feature above limit
        //-------------------------------------------------------------------------

        std::cout << "\n=== Feature above limit ===\n";

        LEM largeMesh;

        const VertexHandle l0 =
            largeMesh.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle l1 =
            largeMesh.add_vertex(
                glm::vec3{ 1.0f, 0.0f, 0.0f });

        const VertexHandle l2 =
            largeMesh.add_vertex(
                glm::vec3{ 0.0f, 1.0f, 0.0f });

        largeMesh.add_face(
            { l0, l1, l2 });

        AnalysisContext largeContext;
        largeContext.mesh =
            &largeMesh;
        largeContext.profile =
            &profile;

        AnalysisReport largeReport;

        analyzer.analyze(
            largeContext,
            largeReport);

        if (!largeReport.has_issue_type(
            PrintIssueType::MinimumFeatureSize)) {

            std::cout
                << "[OK] features acima do limite nao sao reportadas\n";
        }
        else {
            std::cout
                << "[FAIL] geometria grande gerou falso positivo\n";
        }

        //-------------------------------------------------------------------------
        // One undersized edge in a surface
        //-------------------------------------------------------------------------

        std::cout << "\n=== One undersized feature ===\n";

        LEM smallMesh;

        const VertexHandle s0 =
            smallMesh.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle s1 =
            smallMesh.add_vertex(
                glm::vec3{ 0.2f, 0.0f, 0.0f });

        const VertexHandle s2 =
            smallMesh.add_vertex(
                glm::vec3{ 0.0f, 2.0f, 0.0f });

        const FaceHandle smallFace =
            smallMesh.add_face(
                { s0, s1, s2 });

        const EdgeHandle shortEdge =
            smallMesh.find_edge(
                s0,
                s1);

        AnalysisContext smallContext;
        smallContext.mesh =
            &smallMesh;
        smallContext.profile =
            &profile;

        AnalysisReport smallReport;

        analyzer.analyze(
            smallContext,
            smallReport);

        if (smallFace.is_valid() &&
            shortEdge.is_valid() &&
            smallReport.issue_count(
                PrintIssueType::MinimumFeatureSize) == 1 &&
            smallReport.warning_count() == 1) {

            std::cout
                << "[OK] edge abaixo do limite foi detectada\n";
        }
        else {
            std::cout
                << "[FAIL] feature pequena nao foi reportada corretamente\n";
        }

        if (smallReport.issue_count() == 1) {
            const PrintIssue& issue =
                smallReport.issues().front();

            const bool hasShortEdge =
                std::find(
                    issue.location.edges.begin(),
                    issue.location.edges.end(),
                    shortEdge) !=
                issue.location.edges.end();

            if (hasShortEdge &&
                issue.location.vertices.size() == 2 &&
                issue.location.faces.size() == 1 &&
                issue.location.has_samples() &&
                issue.location.has_region()) {

                std::cout
                    << "[OK] feature preserva edge, vertices, face e regiao visual\n";
            }
            else {
                std::cout
                    << "[FAIL] localizacao visual da feature incompleta\n";
            }

            if (issue.has_measurement() &&
                issue.measurement->kind ==
                IssueMeasurementKind::Length &&
                std::abs(
                    issue.measurement->value -
                    0.2) < 1.0e-6 &&
                issue.measurement->has_limit() &&
                std::abs(
                    issue.measurement->limit.value() -
                    0.5) < 1.0e-9) {

                std::cout
                    << "[OK] issue preserva tamanho medido e limite do profile\n";
            }
            else {
                std::cout
                    << "[FAIL] medida da feature inconsistente\n";
            }
        }

        //-------------------------------------------------------------------------
        // Connected short edges become one feature
        //-------------------------------------------------------------------------

        std::cout << "\n=== Connected undersized edges ===\n";

        LEM connectedMesh;

        const VertexHandle c0 =
            connectedMesh.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle c1 =
            connectedMesh.add_vertex(
                glm::vec3{ 0.2f, 0.0f, 0.0f });

        const VertexHandle c2 =
            connectedMesh.add_vertex(
                glm::vec3{ 0.2f, 0.2f, 0.0f });

        const VertexHandle c3 =
            connectedMesh.add_vertex(
                glm::vec3{ 2.0f, 2.0f, 0.0f });

        connectedMesh.find_or_create_edge(
            c0,
            c1);

        connectedMesh.find_or_create_edge(
            c1,
            c2);

        connectedMesh.find_or_create_edge(
            c2,
            c3);

        AnalysisContext connectedContext;
        connectedContext.mesh =
            &connectedMesh;
        connectedContext.profile =
            &profile;

        AnalysisReport connectedReport;

        analyzer.analyze(
            connectedContext,
            connectedReport);

        if (connectedReport.issue_count(
            PrintIssueType::MinimumFeatureSize) == 1) {

            std::cout
                << "[OK] edges pequenas conectadas formam uma unica feature\n";
        }
        else {
            std::cout
                << "[FAIL] feature conectada foi fragmentada em varios issues\n";
        }

        if (connectedReport.issue_count() == 1 &&
            connectedReport.issues()
            .front().location.edges.size() == 2 &&
            connectedReport.issues()
            .front().location.vertices.size() == 3) {

            std::cout
                << "[OK] feature conectada preserva todo o pequeno detalhe\n";
        }
        else {
            std::cout
                << "[FAIL] localizacao da feature conectada inconsistente\n";
        }

        //-------------------------------------------------------------------------
        // Disconnected short features
        //-------------------------------------------------------------------------

        std::cout << "\n=== Independent undersized features ===\n";

        LEM independentMesh;

        const VertexHandle a0 =
            independentMesh.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle a1 =
            independentMesh.add_vertex(
                glm::vec3{ 0.2f, 0.0f, 0.0f });

        independentMesh.find_or_create_edge(
            a0,
            a1);

        const VertexHandle b0 =
            independentMesh.add_vertex(
                glm::vec3{ 5.0f, 0.0f, 0.0f });

        const VertexHandle b1 =
            independentMesh.add_vertex(
                glm::vec3{ 5.3f, 0.0f, 0.0f });

        independentMesh.find_or_create_edge(
            b0,
            b1);

        AnalysisContext independentContext;
        independentContext.mesh =
            &independentMesh;
        independentContext.profile =
            &profile;

        AnalysisReport independentReport;

        analyzer.analyze(
            independentContext,
            independentReport);

        if (independentReport.issue_count(
            PrintIssueType::MinimumFeatureSize) == 2) {

            std::cout
                << "[OK] features pequenas desconectadas geram issues independentes\n";
        }
        else {
            std::cout
                << "[FAIL] features independentes foram agrupadas incorretamente\n";
        }

        //-------------------------------------------------------------------------
        // Degenerate geometry remains separate
        //-------------------------------------------------------------------------

        std::cout << "\n=== Degenerate separation ===\n";

        LEM degenerateMesh;

        const VertexHandle d0 =
            degenerateMesh.add_vertex(
                glm::vec3{ 1.0f, 1.0f, 1.0f });

        const VertexHandle d1 =
            degenerateMesh.add_vertex(
                glm::vec3{ 1.0f, 1.0f, 1.0f });

        degenerateMesh.find_or_create_edge(
            d0,
            d1);

        AnalysisContext degenerateContext;
        degenerateContext.mesh =
            &degenerateMesh;
        degenerateContext.profile =
            &profile;

        AnalysisReport degenerateReport;

        analyzer.analyze(
            degenerateContext,
            degenerateReport);

        if (!degenerateReport.has_issue_type(
            PrintIssueType::MinimumFeatureSize)) {

            std::cout
                << "[OK] edge degenerada continua responsabilidade do DegenerateGeometryAnalyzer\n";
        }
        else {
            std::cout
                << "[FAIL] feature analyzer duplicou geometria degenerada\n";
        }

        std::cout
            << "\n=== Manufacturing MinimumFeatureSizeAnalyzer Smoke Test Finished ===\n\n";
    }

    //=============================================================================
    // End Manufacturing MinimumFeatureSizeAnalyzer Smoke Test
    //=============================================================================

    if (argc > 1 &&
        std::string_view{ argv[1] } == "--transform-smoke-test") {
        return run_transform_history_smoke_test() ? 0 : 1;
    }

    if (argc > 1 &&
        std::string_view{ argv[1] } == "--mesh-edit-smoke-test") {
        return run_mesh_edit_smoke_test() ? 0 : 1;
    }

    return run_application();
}
