/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"
#include "application/shortcut/Shortcut.h"
#include "application/tools/MeshToolActivationController.h"
#include "editor/EditorTypes.h"
#include "editor/command/CommandResult.h"
#include "editor/command/transform/SetNodePivotCommand.h"
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
#include "editor/tools/transform/PivotTool.h"
#include "editor/tools/selection/SelectTool.h"
#include "editor/transform/TransformPivotResolver.h"
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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
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
#include "kernel/manufacturing/analyzers/thinwall/IThinWallAnalyzer.h"
#include "kernel/manufacturing/analyzers/thinwall/ThinWallQuality.h"
#include "kernel/manufacturing/analyzers/thinwall/ThinWallResult.h"
#include "kernel/manufacturing/analyzers/thinwall/RaycastThinWallAnalyzer.h"
#include "kernel/manufacturing/analyzers/thinwall/ThinWallAnalyzerFactory.h"
#include "kernel/manufacturing/analyzers/process/OverhangAnalyzer.h"
#include "kernel/manufacturing/pipeline/AnalysisPipeline.h"

#include "kernel/geometry/mesh/LEM.h"


//========manufacturing==============================


namespace {

    using locus::application::DocumentSession;
    using locus::editor::CommandResult;
    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::NodeTransform;
    using locus::editor::NodeTransformChange;
    using locus::editor::NodePivot;
    using locus::editor::PivotTool;
    using locus::editor::SceneNode;
    using locus::editor::SceneNodeId;
    using locus::editor::SetNodePivotCommand;
    using locus::editor::SetNodeTransformsCommand;
    using locus::editor::TransformPivotResolver;
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

    [[nodiscard]] bool same_pivot(
        const NodePivot& lhs,
        const NodePivot& rhs)
    {
        return lhs.custom == rhs.custom &&
            nearly_equal(lhs.offset, rhs.offset);
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
            document.mark_history_changed();
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

    [[nodiscard]] glm::vec2 project_smoke_point(
        const glm::mat4& viewProjection,
        const glm::vec2& viewportSize,
        const glm::vec3& world)
    {
        const glm::vec4 clip =
            viewProjection * glm::vec4{ world, 1.0f };
        const glm::vec3 ndc = glm::vec3{ clip } / clip.w;
        return {
            (ndc.x * 0.5f + 0.5f) * viewportSize.x,
            (ndc.y * 0.5f + 0.5f) * viewportSize.y
        };
    }

    [[nodiscard]] locus::editor::ToolEvent make_pivot_pointer_event(
        locus::editor::ToolEventType type,
        const glm::vec3& rayPlanePoint,
        const glm::vec2& viewportPosition,
        const glm::mat4& viewProjection,
        const glm::vec2& viewportSize)
    {
        locus::editor::ToolEvent event{};
        event.type = type;
        event.button =
            type == locus::editor::ToolEventType::PointerPress ||
            type == locus::editor::ToolEventType::PointerRelease
            ? locus::editor::ToolPointerButton::Primary
            : locus::editor::ToolPointerButton::None;
        event.pointer.viewportPosition = viewportPosition;
        event.pointer.viewportSize = viewportSize;
        event.pointer.viewProjection = viewProjection;
        event.pointer.viewDirection = { 0.0f, 0.0f, -1.0f };
        event.pointer.viewRight = { 1.0f, 0.0f, 0.0f };
        event.pointer.viewUp = { 0.0f, 1.0f, 0.0f };
        event.pointer.cameraPosition = { 0.0f, 0.0f, 5.0f };
        event.pointer.worldRay.origin =
            rayPlanePoint + glm::vec3{ 0.0f, 0.0f, 5.0f };
        event.pointer.worldRay.direction = { 0.0f, 0.0f, -1.0f };
        return event;
    }

    [[nodiscard]] bool run_pivot_smoke_test()
    {
        DocumentSession document{ locus::application::DocumentId{ 7u } };
        Editor& editor = document.editor();

        const SceneNodeId object =
            editor.scene().create_empty("Pivot Smoke Object");
        if (object.is_invalid()) {
            std::cerr << "Pivot smoke: failed to create object.\n";
            return false;
        }

        SceneNode& node =
            require_node(editor, object, "Pivot Smoke Object");
        node.transform().set_position(glm::vec3{ 10.0f, 0.0f, 0.0f });
        editor.selection_controller().select_object(object);

        const NodeTransform transformBefore = node.transform();
        const NodePivot initialPivot = node.pivot();
        if (initialPivot.custom ||
            !nearly_equal(initialPivot.offset, glm::vec3{ 0.0f }) ||
            !nearly_equal(
                TransformPivotResolver::node_pivot_position(
                    editor.scene(),
                    object),
                glm::vec3{ 10.0f, 0.0f, 0.0f })) {
            std::cerr << "Pivot smoke: initial pivot invariant failed.\n";
            return false;
        }

        locus::editor::ToolContext toolContext(
            editor,
            document.command_dispatcher(),
            document.history(),
            document.editor_sync().picking_sync());

        PivotTool tool{};
        if (!tool.can_activate(toolContext) ||
            tool.activate(toolContext).failed()) {
            std::cerr << "Pivot smoke: tool activation failed.\n";
            return false;
        }

        const glm::mat4 viewProjection =
            glm::ortho(8.0f, 14.0f, -3.0f, 3.0f, -10.0f, 10.0f);
        const glm::vec2 viewportSize{ 600.0f, 600.0f };
        const glm::vec3 initialWorld =
            TransformPivotResolver::node_pivot_position(
                editor.scene(),
                object);
        const glm::vec2 pressPosition =
            project_smoke_point(
                viewProjection,
                viewportSize,
                initialWorld);

        const locus::editor::ToolEvent hoverEvent =
            make_pivot_pointer_event(
                locus::editor::ToolEventType::PointerMove,
                initialWorld,
                pressPosition,
                viewProjection,
                viewportSize);

        if (!tool.handle_event(toolContext, hoverEvent).was_consumed() ||
            !tool.hovered()) {
            std::cerr << "Pivot smoke: hover failed.\n";
            return false;
        }

        const std::size_t historyBeforeDrag =
            document.history().undo_size();
        const locus::editor::ToolEvent pressEvent =
            make_pivot_pointer_event(
                locus::editor::ToolEventType::PointerPress,
                initialWorld,
                pressPosition,
                viewProjection,
                viewportSize);

        if (!tool.handle_event(toolContext, pressEvent).was_consumed() ||
            !tool.dragging()) {
            std::cerr << "Pivot smoke: drag did not start on marker.\n";
            return false;
        }

        const glm::vec3 finalWorld{ 12.0f, 0.0f, 0.0f };
        for (int step = 1; step <= 6; ++step) {
            const float t = static_cast<float>(step) / 6.0f;
            const glm::vec3 candidate =
                initialWorld + (finalWorld - initialWorld) * t;
            const locus::editor::ToolEvent moveEvent =
                make_pivot_pointer_event(
                    locus::editor::ToolEventType::PointerMove,
                    candidate,
                    pressPosition,
                    viewProjection,
                    viewportSize);

            if (!tool.handle_event(toolContext, moveEvent).was_consumed()) {
                std::cerr << "Pivot smoke: drag update ignored.\n";
                return false;
            }
        }

        if (document.history().undo_size() != historyBeforeDrag ||
            !same_position(node.transform(), transformBefore.position()) ||
            !nearly_equal(node.pivot().offset, glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
            !node.pivot().custom) {
            std::cerr << "Pivot smoke: preview invariant failed.\n";
            return false;
        }

        const locus::editor::ToolEvent releaseEvent =
            make_pivot_pointer_event(
                locus::editor::ToolEventType::PointerRelease,
                finalWorld,
                pressPosition,
                viewProjection,
                viewportSize);

        if (tool.handle_event(toolContext, releaseEvent).failed() ||
            document.history().undo_size() != historyBeforeDrag + 1u ||
            !same_position(node.transform(), transformBefore.position()) ||
            !nearly_equal(
                TransformPivotResolver::node_pivot_position(
                    editor.scene(),
                    object),
                finalWorld)) {
            std::cerr << "Pivot smoke: commit invariant failed.\n";
            return false;
        }

        CommandResult undoResult =
            document.history().undo(document.command_dispatcher());
        if (!undoResult.success ||
            !same_pivot(node.pivot(), initialPivot) ||
            !same_position(node.transform(), transformBefore.position())) {
            std::cerr << "Pivot smoke: undo invariant failed.\n";
            return false;
        }

        CommandResult redoResult =
            document.history().redo(document.command_dispatcher());
        if (!redoResult.success ||
            !node.pivot().custom ||
            !nearly_equal(node.pivot().offset, glm::vec3{ 2.0f, 0.0f, 0.0f }) ||
            !same_position(node.transform(), transformBefore.position())) {
            std::cerr << "Pivot smoke: redo invariant failed.\n";
            return false;
        }

        const std::size_t historyAfterCommit =
            document.history().undo_size();
        const NodePivot committedPivot = node.pivot();

        if (!tool.activate(toolContext).was_consumed()) {
            std::cerr << "Pivot smoke: second activation failed.\n";
            return false;
        }

        const glm::vec2 committedScreen =
            project_smoke_point(
                viewProjection,
                viewportSize,
                finalWorld);
        if (!tool.handle_event(
                toolContext,
                make_pivot_pointer_event(
                    locus::editor::ToolEventType::PointerPress,
                    finalWorld,
                    committedScreen,
                    viewProjection,
                    viewportSize)).was_consumed()) {
            std::cerr << "Pivot smoke: cancel drag setup failed.\n";
            return false;
        }

        (void)tool.handle_event(
            toolContext,
            make_pivot_pointer_event(
                locus::editor::ToolEventType::PointerMove,
                glm::vec3{ 11.0f, 1.0f, 0.0f },
                committedScreen,
                viewProjection,
                viewportSize));

        if (!tool.cancel(toolContext).was_consumed() ||
            document.history().undo_size() != historyAfterCommit ||
            !same_pivot(node.pivot(), committedPivot)) {
            std::cerr << "Pivot smoke: cancel invariant failed.\n";
            return false;
        }

        const SceneNodeId parent =
            editor.scene().create_empty("Pivot Smoke Parent");
        const SceneNodeId child =
            editor.scene().create_empty("Pivot Smoke Child");
        if (parent.is_invalid() ||
            child.is_invalid() ||
            !editor.scene().reparent(child, parent)) {
            std::cerr << "Pivot smoke: hierarchy setup failed.\n";
            return false;
        }

        SceneNode& parentNode =
            require_node(editor, parent, "Pivot Smoke Parent");
        parentNode.transform().set_position(glm::vec3{ 5.0f, 2.0f, 0.0f });
        parentNode.transform().set_rotation(
            glm::angleAxis(
                glm::half_pi<float>(),
                glm::vec3{ 0.0f, 0.0f, 1.0f }));
        parentNode.transform().set_scale(glm::vec3{ 2.0f, 1.5f, 1.0f });

        SceneNode& childNode =
            require_node(editor, child, "Pivot Smoke Child");
        childNode.transform().set_position(glm::vec3{ 1.0f, 0.0f, 0.0f });

        const glm::vec3 hierarchyWorld{ 4.0f, 6.0f, 0.0f };
        NodePivot hierarchyPivot{};
        hierarchyPivot.offset =
            TransformPivotResolver::node_local_offset_from_world(
                editor.scene(),
                child,
                hierarchyWorld);
        hierarchyPivot.custom = true;

        CommandResult hierarchyResult =
            toolContext.execute_command(
                std::make_unique<SetNodePivotCommand>(
                    child,
                    hierarchyPivot));

        if (!hierarchyResult.success ||
            !nearly_equal(
                TransformPivotResolver::node_pivot_position(
                    editor.scene(),
                    child),
                hierarchyWorld)) {
            std::cerr << "Pivot smoke: hierarchy world/local invariant failed.\n";
            return false;
        }

        std::cout
            << "Pivot smoke test passed. offset=("
            << node.pivot().offset.x << ", "
            << node.pivot().offset.y << ", "
            << node.pivot().offset.z << ") history="
            << document.history().undo_size()
            << '\n';

        return true;
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

        document.mark_history_changed();
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

        document.mark_history_changed();
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
            << "M Manufacturing Analysis, "
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
    // Manufacturing AnalysisPipeline Smoke Test
    //=============================================================================

    {
        using namespace locus::kernel;
        using namespace locus::kernel::geometry;
        using namespace locus::kernel::manufacturing;

        std::cout
            << "\n=== Locus3D Manufacturing AnalysisPipeline Smoke Test ===\n\n";

        //-------------------------------------------------------------------------
        // Closed tetrahedron without profile
        //-------------------------------------------------------------------------

        std::cout << "=== Closed tetrahedron ===\n";

        LEM tetrahedron;

        const VertexHandle t0 =
            tetrahedron.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle t1 =
            tetrahedron.add_vertex(
                glm::vec3{ 1.0f, 0.0f, 0.0f });

        const VertexHandle t2 =
            tetrahedron.add_vertex(
                glm::vec3{ 0.0f, 1.0f, 0.0f });

        const VertexHandle t3 =
            tetrahedron.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 1.0f });

        tetrahedron.add_face({ t0, t2, t1 });
        tetrahedron.add_face({ t0, t1, t3 });
        tetrahedron.add_face({ t1, t2, t3 });
        tetrahedron.add_face({ t2, t0, t3 });

        AnalysisReport tetraReport =
            AnalysisPipeline::analyze(
                tetrahedron);

        if (tetraReport.metrics()
            .has_analysis_triangle_count() &&
            tetraReport.metrics()
            .analysisTriangleCount.value() == 4) {

            std::cout
                << "[OK] pipeline constroi uma triangulacao canonica\n";
        }
        else {
            std::cout
                << "[FAIL] triangle count do pipeline incorreto\n";
        }

        if (tetraReport.metrics().has_volume() &&
            std::abs(
                tetraReport.metrics().volume.value() -
                (1.0 / 6.0)) <
            1.0e-9) {

            std::cout
                << "[OK] pipeline calcula volume quando prerequisites passam\n";
        }
        else {
            std::cout
                << "[FAIL] volume integrado do pipeline incorreto\n";
        }

        if (!tetraReport.has_issue_type(
            PrintIssueType::OpenBoundary) &&
            !tetraReport.has_issue_type(
                PrintIssueType::NonManifoldEdge) &&
            !tetraReport.has_issue_type(
                PrintIssueType::InconsistentNormals)) {

            std::cout
                << "[OK] topologia fechada atravessa pipeline sem falsos positivos\n";
        }
        else {
            std::cout
                << "[FAIL] pipeline introduziu topology issue indevido\n";
        }

        //-------------------------------------------------------------------------
        // Open surface blocks volume
        //-------------------------------------------------------------------------

        std::cout << "\n=== Open surface ===\n";

        LEM openMesh;

        const VertexHandle o0 =
            openMesh.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle o1 =
            openMesh.add_vertex(
                glm::vec3{ 1.0f, 0.0f, 0.0f });

        const VertexHandle o2 =
            openMesh.add_vertex(
                glm::vec3{ 0.0f, 1.0f, 0.0f });

        openMesh.add_face(
            { o0, o1, o2 });

        AnalysisReport openReport =
            AnalysisPipeline::analyze(
                openMesh);

        if (openReport.has_issue_type(
            PrintIssueType::OpenBoundary)) {

            std::cout
                << "[OK] pipeline preserva diagnostico de open boundary\n";
        }
        else {
            std::cout
                << "[FAIL] open boundary nao chegou ao report consolidado\n";
        }

        if (!openReport.metrics().has_volume()) {
            std::cout
                << "[OK] topology issue bloqueia volume nao confiavel\n";
        }
        else {
            std::cout
                << "[FAIL] pipeline publicou volume de superficie aberta\n";
        }

        //-------------------------------------------------------------------------
        // Profile-dependent analysis
        //-------------------------------------------------------------------------

        std::cout << "\n=== Profile integration ===\n";

        FDMProfile fdm;
        fdm.name =
            "Pipeline Test";

        fdm.limits.minimumFeatureSize =
            0.5;

        fdm.limits.minimumWallThickness =
            0.5;

        fdm.limits
            .maximumUnsupportedOverhangAngleDegrees =
            45.0;

        PrintProfile profile{
            fdm
        };

        LEM featureMesh;

        const VertexHandle f0 =
            featureMesh.add_vertex(
                glm::vec3{ 0.0f, 0.0f, 0.0f });

        const VertexHandle f1 =
            featureMesh.add_vertex(
                glm::vec3{ 0.2f, 0.0f, 0.0f });

        featureMesh.find_or_create_edge(
            f0,
            f1);

        AnalysisReport featureReport =
            AnalysisPipeline::analyze(
                featureMesh,
                profile);

        if (featureReport.has_issue_type(
            PrintIssueType::MinimumFeatureSize)) {

            std::cout
                << "[OK] pipeline encaminha PrintProfile aos analyzers dependentes\n";
        }
        else {
            std::cout
                << "[FAIL] profile nao chegou ao feature analyzer\n";
        }

        //-------------------------------------------------------------------------
        // Options disable analysis
        //-------------------------------------------------------------------------

        std::cout << "\n=== AnalysisOptions ===\n";

        AnalysisOptions disabledOptions;

        disabledOptions.analyzeMinimumFeatureSize =
            false;

        AnalysisReport disabledReport =
            AnalysisPipeline::analyze(
                featureMesh,
                profile,
                disabledOptions);

        if (!disabledReport.has_issue_type(
            PrintIssueType::MinimumFeatureSize)) {

            std::cout
                << "[OK] AnalysisOptions desabilita analyzer individual\n";
        }
        else {
            std::cout
                << "[FAIL] analyzer desabilitado ainda executou\n";
        }

        //-------------------------------------------------------------------------
        // Disabled prerequisite blocks dependent metric
        //-------------------------------------------------------------------------

        std::cout << "\n=== Prerequisite safety ===\n";

        AnalysisOptions unsafeVolumeOptions;

        unsafeVolumeOptions.analyzeNormalConsistency =
            false;

        AnalysisReport unsafeVolumeReport =
            AnalysisPipeline::analyze(
                tetrahedron,
                unsafeVolumeOptions);

        if (!unsafeVolumeReport.metrics()
            .has_volume()) {

            std::cout
                << "[OK] prerequisite desabilitada impede volume nao verificado\n";
        }
        else {
            std::cout
                << "[FAIL] pipeline assumiu prerequisite nao executada\n";
        }

        //-------------------------------------------------------------------------
        // Existing report is replaced by analyze_into
        //-------------------------------------------------------------------------

        std::cout << "\n=== analyze_into snapshot semantics ===\n";

        AnalysisReport reusableReport;

        reusableReport.add_issue(
            PrintIssue{
                PrintIssueType::ThinWall,
                IssueSeverity::Warning,
                "Old result."
            });

        AnalysisPipeline::analyze_into(
            tetrahedron,
            reusableReport);

        if (!reusableReport.has_issue_type(
            PrintIssueType::ThinWall) &&
            reusableReport.metrics()
            .has_volume()) {

            std::cout
                << "[OK] analyze_into substitui snapshot anterior\n";
        }
        else {
            std::cout
                << "[FAIL] analyze_into preservou dados obsoletos\n";
        }

        //-------------------------------------------------------------------------
        // Thin-wall quality is routed by pipeline
        //-------------------------------------------------------------------------

        std::cout << "\n=== Thin-wall options routing ===\n";

        AnalysisOptions highOptions;
        highOptions.thinWallQuality =
            ThinWallQuality::High;

        /*
         * This test verifies that High can travel through the public pipeline API.
         * Detailed detection behavior is already covered by the dedicated
         * RaycastThinWallAnalyzer smoke test.
         */
        AnalysisReport highReport =
            AnalysisPipeline::analyze(
                tetrahedron,
                profile,
                highOptions);

        if (highReport.metrics()
            .has_analysis_triangle_count()) {

            std::cout
                << "[OK] pipeline aceita ThinWallQuality configuravel\n";
        }
        else {
            std::cout
                << "[FAIL] pipeline falhou com ThinWallQuality High\n";
        }

        std::cout
            << "\n=== Manufacturing AnalysisPipeline Smoke Test Finished ===\n\n";
    }

    //=============================================================================
    // End Manufacturing AnalysisPipeline Smoke Test
    //=============================================================================

    if (argc > 1 &&
        std::string_view{ argv[1] } == "--transform-smoke-test") {
        return run_transform_history_smoke_test() ? 0 : 1;
    }

    if (argc > 1 &&
        std::string_view{ argv[1] } == "--pivot-smoke-test") {
        return run_pivot_smoke_test() ? 0 : 1;
    }

    if (argc > 1 &&
        std::string_view{ argv[1] } == "--mesh-edit-smoke-test") {
        return run_mesh_edit_smoke_test() ? 0 : 1;
    }

    return run_application();
}
