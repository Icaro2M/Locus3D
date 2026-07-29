/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"
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
#include "editor/tools/selection/SelectTool.h"
#include "editor/transform/TransformSession.h"
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
        event.button = type == locus::editor::ToolEventType::PointerPress
            ? locus::editor::ToolPointerButton::Primary
            : locus::editor::ToolPointerButton::None;
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

        const auto cubeAResult =
            locus::kernel::geometry::TopologyBuilder::build_box_into(
                cubeA->mesh());
        const auto cubeBResult =
            locus::kernel::geometry::TopologyBuilder::build_box_into(
                cubeB->mesh());

        if (!cubeAResult || !cubeBResult) {
            std::cerr << "Failed to build demo cubes.\n";
            return false;
        }

        cubeA->transform().set_position(
            glm::vec3{ -1.4f, 0.0f, 0.0f });
        cubeB->transform().set_position(
            glm::vec3{ 1.4f, 0.0f, 0.0f });

        editor.mark_dirty(
            locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
                locus::editor::EditorDirtyFlags::Picking);

        std::cout
            << "Interactive selection/context checkpoint started.\n"
            << "Cube A SceneNodeId=" << cubeAId.value
            << " at x=-1.4\n"
            << "Cube B SceneNodeId=" << cubeBId.value
            << " at x=1.4\n"
            << "Keys: 1 Object, 2 Vertex, 3 Edge, 4 Face, "
            << "Q Select, E Extrude in Face context, "
            << "W/E/R/T transform gizmo in Object context, "
            << "Esc Cancel, Ctrl+Z Undo, Ctrl+Shift+Z Redo.\n"
            << "Test path: press 1 and click Cube A; press 4 and click "
            << "a face on Cube A; press 1 and click Cube B; press 4 "
            << "and click a face on Cube B. For Extrude: keep Face "
            << "granularity, press E, press-drag-release on the selected "
            << "face, then use Ctrl+Z/Ctrl+Shift+Z or start another "
            << "Extrude and press Esc before release.\n"
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
