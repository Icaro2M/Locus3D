/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/runtime/ApplicationRuntime.h"

#include "application/tools/MeshToolActivationController.h"
#include "editor/EditorTypes.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/edit/RegisterEditActions.h"
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/actions/mesh/face/RegisterFaceActions.h"
#include "editor/actions/mesh/topology/RegisterTopologyActions.h"
#include "editor/command/CommandResult.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/gizmo/GizmoState.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/core/ToolState.h"
#include "editor/tools/selection/SelectTool.h"
#include "editor/tools/transform/TransformTool.h"
#include "graphics/camera/CameraRayBuilder.h"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <glm/vec2.hpp>

namespace locus::application {

    namespace {

        void append_startup_log(const char* message)
        {
            std::ofstream stream(
                "locus3d_startup.log",
                std::ios::app);

            if (stream.is_open()) {
                stream << message << '\n';
            }
        }

        [[nodiscard]] editor::ToolModifiers to_tool_modifiers(
            InputModifiers modifiers) noexcept
        {
            editor::ToolModifiers result =
                editor::ToolModifiers::None;

            if (has_input_modifier(
                    modifiers,
                    InputModifiers::Shift)) {
                result |= editor::ToolModifiers::Additive;
            }

            if (has_input_modifier(
                    modifiers,
                    InputModifiers::Control)) {
                result |= editor::ToolModifiers::Toggle;
            }

            if (has_input_modifier(
                    modifiers,
                    InputModifiers::Alt)) {
                result |= editor::ToolModifiers::Alternate;
            }

            return result;
        }

        [[nodiscard]] editor::ToolContext make_tool_context(
            DocumentSession& document)
        {
            return editor::ToolContext(
                document.editor(),
                document.command_dispatcher(),
                document.history(),
                document.editor_sync().picking_sync());
        }

        [[nodiscard]] const char* granularity_name(
            editor::SelectionGranularity granularity) noexcept
        {
            switch (granularity) {
            case editor::SelectionGranularity::Object:
                return "Object";
            case editor::SelectionGranularity::Vertex:
                return "Vertex";
            case editor::SelectionGranularity::Edge:
                return "Edge";
            case editor::SelectionGranularity::Loop:
                return "Loop";
            case editor::SelectionGranularity::Face:
                return "Face";
            }

            return "Unknown";
        }

        [[nodiscard]] const char* scope_name(
            editor::SelectionScope scope) noexcept
        {
            switch (scope) {
            case editor::SelectionScope::Scene:
                return "Scene";
            case editor::SelectionScope::ActiveMesh:
                return "ActiveMesh";
            }

            return "Unknown";
        }

        [[nodiscard]] const char* tool_result_code_name(
            editor::ToolResultCode code) noexcept
        {
            switch (code) {
            case editor::ToolResultCode::Ignored:
                return "Ignored";
            case editor::ToolResultCode::Consumed:
                return "Consumed";
            case editor::ToolResultCode::Started:
                return "Started";
            case editor::ToolResultCode::Updated:
                return "Updated";
            case editor::ToolResultCode::Confirmed:
                return "Confirmed";
            case editor::ToolResultCode::Cancelled:
                return "Cancelled";
            case editor::ToolResultCode::Failed:
                return "Failed";
            }

            return "Unknown";
        }

        void print_selection_summary(
            const char* label,
            const DocumentSession& document)
        {
            const editor::SelectionState& selection =
                document.editor().selection();
            const editor::MeshSelection& meshSelection =
                selection.mesh();

            std::cout
                << "[selection] " << label
                << " scope=" << scope_name(selection.scope())
                << " granularity="
                << granularity_name(selection.granularity())
                << " activeObject="
                << selection.objects().active().value
                << " hoveredObject="
                << selection.objects().hovered().value
                << " activeMesh="
                << meshSelection.active_mesh().value
                << " vertices=" << meshSelection.vertices().size()
                << " edges=" << meshSelection.edges().size()
                << " faces=" << meshSelection.faces().size()
                << " hoverVertex="
                << meshSelection.hovered_vertex().id.value
                << " hoverEdge="
                << meshSelection.hovered_edge().id.value
                << " hoverFace="
                << meshSelection.hovered_face().id.value
                << " undo=" << document.history().undo_size()
                << " redo=" << document.history().redo_size()
                << '\n';
        }

        void print_tool_result(
            const char* label,
            const editor::ToolResult& result,
            const DocumentSession& document)
        {
            std::cout
                << "[tool] " << label
                << " code=" << tool_result_code_name(result.code)
                << " state=";

            const editor::ITool* tool =
                document.tool_manager().active_tool();

            if (tool == nullptr) {
                std::cout << "None";
            }
            else {
                switch (tool->state()) {
                case editor::ToolState::Inactive:
                    std::cout << "Inactive";
                    break;
                case editor::ToolState::Ready:
                    std::cout << "Ready";
                    break;
                case editor::ToolState::Interacting:
                    std::cout << "Interacting";
                    break;
                case editor::ToolState::Suspended:
                    std::cout << "Suspended";
                    break;
                }
            }

            if (!result.message.empty()) {
                std::cout << " message=\"" << result.message << '"';
            }

            std::cout
                << " history=("
                << document.history().undo_size()
                << '/'
                << document.history().redo_size()
                << ")\n";
        }

        [[nodiscard]] editor::ToolEvent make_pointer_event(
            editor::ToolEventType type,
            editor::ToolPointerButton button,
            const InputState& input,
            const ViewportPickingResult& picking)
        {
            editor::ToolEvent event{};
            event.type = type;
            event.button = button;
            event.modifiers = to_tool_modifiers(input.modifiers());
            event.pointer.viewportPosition = glm::vec2{
                static_cast<float>(picking.framebufferX),
                static_cast<float>(picking.framebufferY)
            };
            event.pointer.viewportDelta = glm::vec2{
                static_cast<float>(input.cursor_delta().x),
                static_cast<float>(input.cursor_delta().y)
            };

            if (picking.has_hit()) {
                event.pointer.pickingId = picking.pickingId;
            }

            return event;
        }

        void populate_camera_pointer_data(
            editor::ToolEvent& event,
            const EditorViewport& viewport)
        {
            const graphics::Viewport& graphicsViewport =
                viewport.viewport();
            const graphics::Camera& camera =
                graphicsViewport.camera();

            event.pointer.viewDirection = camera.forward();
            event.pointer.viewRight = camera.right();
            event.pointer.viewUp = camera.up();
            event.pointer.cameraPosition = camera.position();
            event.pointer.orthographicProjection =
                camera.projection().type()
                == graphics::ProjectionType::Orthographic;
            event.pointer.viewportSize = glm::vec2{
                static_cast<float>(graphicsViewport.state().rect.width),
                static_cast<float>(graphicsViewport.state().rect.height)
            };
            event.pointer.viewProjection =
                camera.view_projection_matrix();
            event.pointer.visualScale =
                viewport.visual_scale_at(
                    viewport.orbit_rig().target());

            // Picking stores local framebuffer Y in OpenGL bottom-left space,
            // while CameraRayBuilder expects window-style top-down pixels.
            const graphics::ViewportRect& rect =
                graphicsViewport.state().rect;
            const float rayPixelX =
                static_cast<float>(rect.x)
                + event.pointer.viewportPosition.x;
            const float rayPixelY =
                static_cast<float>(rect.y + rect.height - 1)
                - event.pointer.viewportPosition.y;

            const graphics::CameraRay ray =
                graphics::CameraRayBuilder::from_viewport_pixel(
                    camera,
                    rect,
                    rayPixelX,
                    rayPixelY);

            event.pointer.worldRay.origin = ray.origin;
            event.pointer.worldRay.direction = ray.direction;
        }

        [[nodiscard]] editor::SelectTool* active_select_tool(
            DocumentSession& document)
        {
            return dynamic_cast<editor::SelectTool*>(
                document.tool_manager().active_tool());
        }

        [[nodiscard]] ViewportShadingFrameConfig viewport_policy(
            const EditorViewport& viewport)
        {
            return viewport_shading_frame_config(
                ViewportDisplaySettings{
                    viewport.shading_mode(),
                    viewport.face_orientation_enabled() });
        }

        void apply_selection_depth_policy(
            DocumentSession& document,
            const EditorViewport& viewport)
        {
            editor::SelectTool* selectTool =
                active_select_tool(document);
            if (selectTool == nullptr) {
                return;
            }

            const ViewportShadingFrameConfig policy =
                viewport_policy(viewport);
            selectTool->set_selection_depth_modes(
                policy.pointSelectionDepthMode,
                policy.regionSelectionDepthMode);
        }

        void populate_transform_pointer_scale(
            editor::ToolEvent& event,
            DocumentSession& document,
            const EditorViewport& viewport)
        {
            auto* transformTool =
                dynamic_cast<editor::TransformTool*>(
                    document.tool_manager().active_tool());

            if (transformTool == nullptr) {
                return;
            }

            editor::ToolContext toolContext = make_tool_context(document);
            transformTool->refresh_gizmo_state(toolContext);

            const editor::GizmoState& gizmoState =
                transformTool->gizmo_state();

            if (!gizmoState.visible) {
                return;
            }

            event.pointer.visualScale =
                viewport.visual_scale_at(gizmoState.pivot);
        }

        [[nodiscard]] ApplicationResult<void> tool_failure(
            const editor::ToolResult& result)
        {
            return ApplicationError::make(
                ApplicationErrorCode::RuntimeFailure,
                result.message.empty()
                    ? "Editor tool input failed."
                    : result.message);
        }

        [[nodiscard]] ApplicationResult<editor::ToolResult>
        dispatch_tool_event_result(
            DocumentSession& document,
            const editor::ToolEvent& event)
        {
            const std::size_t undoSizeBefore =
                document.history().undo_size();

            editor::ToolContext toolContext = make_tool_context(document);
            const editor::ToolResult result =
                document.tool_manager().handle_event(
                    toolContext,
                    event);

            if (result.failed()) {
                return tool_failure(result).error();
            }

            if (event.type != editor::ToolEventType::PointerMove
                && result.was_consumed()) {
                print_tool_result(
                    "event",
                    result,
                    document);
                print_selection_summary(
                    "after tool event",
                    document);
            }
            else if (event.type == editor::ToolEventType::PointerMove
                && result.code == editor::ToolResultCode::Updated
                && MeshToolActivationController{}
                    .is_logged_preview_tool(document)) {
                static std::size_t previewLogCounter = 0;

                if ((previewLogCounter % 12u) == 0u) {
                    print_tool_result(
                        "preview update",
                        result,
                        document);
                }

                ++previewLogCounter;
            }

            const bool persistentSceneChange =
                result.code == editor::ToolResultCode::Confirmed
                && document.history().undo_size() > undoSizeBefore
                && editor::has_flag(
                    result.dirtyFlags,
                    editor::EditorDirtyFlags::Scene |
                    editor::EditorDirtyFlags::Mesh);

            if (persistentSceneChange) {
                document.mark_dirty();
            }

            return result;
        }

        [[nodiscard]] ApplicationResult<void> dispatch_tool_event(
            DocumentSession& document,
            const editor::ToolEvent& event)
        {
            const auto result =
                dispatch_tool_event_result(document, event);

            if (!result) {
                return result.error();
            }

            return {};
        }

        [[nodiscard]] bool can_route_pointer_to_tool(
            const ViewportPickingResult& picking) noexcept
        {
            return picking.status == ViewportPickingStatus::Hit
                || picking.status == ViewportPickingStatus::Background;
        }

        [[nodiscard]] bool can_route_hover_to_tool(
            const ViewportPickingResult& picking) noexcept
        {
            return can_route_pointer_to_tool(picking)
                || picking.status == ViewportPickingStatus::OutsideViewport;
        }

        [[nodiscard]] ApplicationResult<void> activate_tool(
            DocumentSession& document,
            const editor::ToolId& id)
        {
            editor::ToolContext toolContext = make_tool_context(document);
            const editor::ToolResult result =
                document.tool_manager().activate_tool(
                    toolContext,
                    id);

            if (result.failed()) {
                return tool_failure(result);
            }

            return {};
        }

        [[nodiscard]] ApplicationResult<void> set_selection_granularity(
            DocumentSession& document,
            editor::SelectionGranularity granularity)
        {
            document.editor().selection_controller()
                .set_granularity(granularity);
            document.editor().mark_dirty(
                editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render);

            const auto activateResult =
                activate_tool(
                    document,
                    editor::ToolId{ editor::SelectTool::Id });

            if (!activateResult) {
                return activateResult.error();
            }

            std::cout
                << "[shortcut] Selection granularity -> "
                << granularity_name(
                    document.editor().selection().granularity())
                << '\n';
            print_selection_summary(
                "after granularity shortcut",
                document);

            return {};
        }

        [[nodiscard]] ApplicationResult<void> activate_transform_tool(
            DocumentSession& document,
            editor::GizmoMode mode)
        {
            editor::ToolContext toolContext = make_tool_context(document);

            if (!document.tool_manager().is_active(
                    editor::ToolId{ editor::TransformTool::Id })) {
                const editor::ToolResult activateResult =
                    document.tool_manager().activate_tool(
                        toolContext,
                        editor::ToolId{ editor::TransformTool::Id });

                if (activateResult.failed()) {
                    return tool_failure(activateResult);
                }
            }

            editor::ITool* activeTool =
                document.tool_manager().active_tool();
            auto* transformTool =
                dynamic_cast<editor::TransformTool*>(activeTool);

            if (transformTool == nullptr
                || !transformTool->set_mode(mode)) {
                return ApplicationError::make(
                    ApplicationErrorCode::RuntimeFailure,
                    "Failed to activate transform shortcut.");
            }

            transformTool->refresh_gizmo_state(toolContext);

            document.editor().mark_dirty(
                editor::EditorDirtyFlags::Render);
            return {};
        }

        [[nodiscard]] ApplicationResult<void> execute_editor_action(
            DocumentSession& document,
            const editor::ActionId& id)
        {
            const std::size_t undoSizeBefore =
                document.history().undo_size();

            editor::ActionContext actionContext(
                document.editor(),
                document.command_dispatcher(),
                document.history());

            const editor::ActionResult result =
                document.action_executor().execute(
                    actionContext,
                    id);

            if (result.failed()) {
                return ApplicationError::make(
                    ApplicationErrorCode::RuntimeFailure,
                    result.message.empty()
                        ? "Editor action execution failed."
                        : result.message);
            }

            if (result.is_unavailable()) {
                if (!result.message.empty()) {
                    std::cout
                        << "[action] unavailable: "
                        << result.message
                        << '\n';
                }

                return {};
            }

            if (document.history().undo_size() > undoSizeBefore
                && editor::has_flag(
                    result.dirtyFlags,
                    editor::EditorDirtyFlags::Scene |
                    editor::EditorDirtyFlags::Mesh)) {
                document.mark_dirty();
            }

            std::cout
                << "[action] "
                << id.value
                << " success undo="
                << document.history().undo_size()
                << " redo="
                << document.history().redo_size()
                << '\n';

            return {};
        }

        [[nodiscard]] ApplicationResult<void> execute_shortcut_action(
            ShortcutAction action,
            DocumentSession& document,
            EditorViewport& viewport)
        {
            const ApplicationResult<bool> meshToolActivation =
                MeshToolActivationController{}
                    .activate_shortcut(
                        action,
                        document);

            if (!meshToolActivation) {
                return meshToolActivation.error();
            }

            if (meshToolActivation.value()) {
                return {};
            }

            switch (action) {
            case ShortcutAction::None:
            case ShortcutAction::Save:
            case ShortcutAction::Open:
            case ShortcutAction::ActivateShrinkFattenTool:
                return {};

            case ShortcutAction::DeleteSelection:
                return execute_editor_action(
                    document,
                    editor::ActionId{
                        std::string{
                            editor::edit_actions::DeleteId } });

            case ShortcutAction::ExecuteBridgeEdgeAction:
                return execute_editor_action(
                    document,
                    editor::ActionId{
                        std::string{
                            editor::edge_actions::BridgeId } });

            case ShortcutAction::ExecuteFillHoleAction:
                return execute_editor_action(
                    document,
                    editor::ActionId{
                        std::string{
                            editor::topology_actions::FillHoleId } });

            case ShortcutAction::ExecuteFlipFacesAction:
                return execute_editor_action(
                    document,
                    editor::ActionId{
                        std::string{
                            editor::face_actions::FlipFaceId } });

            case ShortcutAction::ExecuteDissolveAction:
                return execute_editor_action(
                    document,
                    editor::ActionId{
                        std::string{
                            editor::edit_actions::DissolveId } });

            case ShortcutAction::ActivateSelectTool:
                return activate_tool(
                    document,
                    editor::ToolId{ editor::SelectTool::Id });

            case ShortcutAction::SetObjectGranularity:
                return set_selection_granularity(
                    document,
                    editor::SelectionGranularity::Object);

            case ShortcutAction::SetVertexGranularity:
                return set_selection_granularity(
                    document,
                    editor::SelectionGranularity::Vertex);

            case ShortcutAction::SetEdgeGranularity:
                return set_selection_granularity(
                    document,
                    editor::SelectionGranularity::Edge);

            case ShortcutAction::SetFaceGranularity:
                return set_selection_granularity(
                    document,
                    editor::SelectionGranularity::Face);

            case ShortcutAction::ToggleProjection:
                viewport.toggle_projection_mode();
                return {};

            case ShortcutAction::FrontView:
                viewport.set_view_orientation(ViewOrientation::Front);
                return {};

            case ShortcutAction::BackView:
                viewport.set_view_orientation(ViewOrientation::Back);
                return {};

            case ShortcutAction::LeftView:
                viewport.set_view_orientation(ViewOrientation::Left);
                return {};

            case ShortcutAction::RightView:
                viewport.set_view_orientation(ViewOrientation::Right);
                return {};

            case ShortcutAction::TopView:
                viewport.set_view_orientation(ViewOrientation::Top);
                return {};

            case ShortcutAction::BottomView:
                viewport.set_view_orientation(ViewOrientation::Bottom);
                return {};

            case ShortcutAction::ToggleViewportShading:
                viewport.toggle_shading_mode();
                return {};

            case ShortcutAction::ToggleFaceOrientation:
                viewport.toggle_face_orientation();
                return {};

            case ShortcutAction::ActivateTranslateTool:
                return activate_transform_tool(
                    document,
                    editor::GizmoMode::Translate);

            case ShortcutAction::ActivateRotateTool:
                return activate_transform_tool(
                    document,
                    editor::GizmoMode::Rotate);

            case ShortcutAction::ActivateScaleTool:
                return activate_transform_tool(
                    document,
                    editor::GizmoMode::Scale);

            case ShortcutAction::ActivateUniversalTool:
                return activate_transform_tool(
                    document,
                    editor::GizmoMode::Universal);

            case ShortcutAction::Cancel:
                return dispatch_tool_event(
                    document,
                    editor::ToolEvent{
                        editor::ToolEventType::Cancel });

            case ShortcutAction::Undo:
            case ShortcutAction::Redo: {
                const editor::CommandResult result =
                    action == ShortcutAction::Redo
                ? document.history().redo(document.command_dispatcher())
                : document.history().undo(document.command_dispatcher());

                if (!result.success
                    && result.message != "Nothing to undo."
                    && result.message != "Nothing to redo.") {
                    return ApplicationError::make(
                        ApplicationErrorCode::RuntimeFailure,
                        result.message);
                }

                if (result.success
                    && editor::has_flag(
                        result.dirtyFlags,
                        editor::EditorDirtyFlags::Scene |
                        editor::EditorDirtyFlags::Mesh)) {
                    document.mark_dirty();
                }

                if (result.success) {
                    std::cout
                        << "[history] "
                        << (action == ShortcutAction::Redo
                            ? "redo"
                            : "undo")
                        << " success undo="
                        << document.history().undo_size()
                        << " redo="
                        << document.history().redo_size()
                        << '\n';
                }

                return {};
            }
            }

            return {};
        }

        [[nodiscard]] bool active_tool_is_transform(
            const DocumentSession& document)
        {
            return document.tool_manager().is_active(
                editor::ToolId{ editor::TransformTool::Id });
        }

        [[nodiscard]] bool active_tool_is_interacting(
            const DocumentSession& document)
        {
            const editor::ITool* tool =
                document.tool_manager().active_tool();

            return tool != nullptr
                && tool->state() == editor::ToolState::Interacting;
        }

        [[nodiscard]] ApplicationResult<void> route_press_to_select_tool(
            DocumentSession& document,
            const editor::ToolEvent& pressEvent)
        {
            const auto activateResult =
                activate_tool(
                    document,
                    editor::ToolId{ editor::SelectTool::Id });

            if (!activateResult) {
                return activateResult.error();
            }

            return dispatch_tool_event(document, pressEvent);
        }

        void apply_viewport_navigation(
            const InputRouteResult& route,
            EditorViewport& viewport)
        {
            if (route.captureBegan) {
                return;
            }

            switch (route.navigation) {
            case ViewportNavigationAction::Orbit:
                viewport.orbit_camera(
                    route.cursorDelta.x,
                    route.cursorDelta.y);
                break;

            case ViewportNavigationAction::Pan:
                viewport.pan_camera(
                    route.cursorDelta.x,
                    route.cursorDelta.y);
                break;

            case ViewportNavigationAction::Zoom:
                viewport.zoom_camera(route.scrollDelta);
                break;

            case ViewportNavigationAction::None:
                break;
            }
        }

    } // namespace

    ApplicationRuntime::~ApplicationRuntime()
    {
        shutdown();
    }

    ApplicationResult<void> ApplicationRuntime::initialize(
        const ApplicationConfig& config)
    {
        if (initialized()
            || state_.phase == ApplicationPhase::Initializing
            || state_.phase == ApplicationPhase::Stopping) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "ApplicationRuntime cannot initialize from its current state.");
        }

        configuration_ = config;
        state_ = {};
        state_.phase = ApplicationPhase::Initializing;

        frameClock_.set_maximum_delta(
            configuration_.maximumFrameDeltaSeconds);

        ApplicationResult<void> windowResult =
            window_.initialize(configuration_);

        if (!windowResult) {
            state_.phase = ApplicationPhase::Failed;
            state_.exitCode = 1;
            return windowResult.error();
        }
        append_startup_log("ApplicationRuntime: window initialized");

        window_.connect_input(inputState_);
        (void)documents_.create_document();
        append_startup_log("ApplicationRuntime: document created");

        ApplicationResult<void> viewportResult =
            editorViewport_.initialize(
                window_.framebuffer_width(),
                window_.framebuffer_height());

        if (!viewportResult) {
            append_startup_log("ApplicationRuntime: viewport initialization failed");
            documents_ = DocumentManager{};
            window_.shutdown();
            state_.phase = ApplicationPhase::Failed;
            state_.exitCode = 1;
            return viewportResult.error();
        }
        append_startup_log("ApplicationRuntime: viewport initialized");

        frameClock_.reset();
        state_.phase = ApplicationPhase::Running;
        append_startup_log("ApplicationRuntime: running");
        return {};
    }

    ApplicationResult<int> ApplicationRuntime::run()
    {
        if (!initialized()) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "ApplicationRuntime must be initialized before run().");
        }

        while (!state_.quitRequested && !window_.should_close()) {
            ApplicationResult<FrameContext> frameResult =
                run_frame();

            if (!frameResult) {
                state_.phase = ApplicationPhase::Failed;
                state_.exitCode = 1;
                const ApplicationError error = frameResult.error();
                shutdown();
                return error;
            }
        }

        if (window_.should_close()) {
            state_.quitRequested = true;
        }

        const int exitCode = state_.exitCode;
        shutdown();
        return exitCode;
    }

    ApplicationResult<FrameContext> ApplicationRuntime::run_frame()
    {
        if (!initialized()
            || state_.phase != ApplicationPhase::Running
            || state_.quitRequested) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "ApplicationRuntime is not ready to process a frame.");
        }

        inputState_.begin_frame();
        window_.process_events();
        append_startup_log("ApplicationRuntime: frame events processed");

        const FrameContext context = frameClock_.tick();
        state_.frameIndex = frameClock_.next_frame_index();

        DocumentSession* activeDocument = documents_.active_document();

        if (!activeDocument) {
            inputState_.end_frame();
            return ApplicationError::make(
                ApplicationErrorCode::InternalFailure,
                "ApplicationRuntime has no active document to render.");
        }

        editorViewport_.resize(
            window_.framebuffer_width(),
            window_.framebuffer_height());

        const InputRouteResult routeResult =
            inputRouter_.route(inputState_);

        apply_viewport_navigation(routeResult, editorViewport_);

        ShortcutContext shortcutContext{};
        shortcutContext.viewportFocused = inputState_.focused();
        shortcutContext.modalActive =
            active_tool_is_interacting(*activeDocument);
        shortcutContext.objectMode =
            activeDocument->editor().selection().scope()
            == editor::SelectionScope::Scene &&
            activeDocument->editor().selection().granularity()
            == editor::SelectionGranularity::Object;
        shortcutContext.vertexSelectionContext =
            activeDocument->editor().selection().scope()
            == editor::SelectionScope::ActiveMesh &&
            activeDocument->editor().selection().granularity()
            == editor::SelectionGranularity::Vertex &&
            activeDocument->editor().selection().mesh()
                .active_mesh().is_valid() &&
            !activeDocument->editor().selection().mesh()
                .vertices().empty();
        shortcutContext.faceSelectionContext =
            activeDocument->editor().selection().scope()
            == editor::SelectionScope::ActiveMesh &&
            activeDocument->editor().selection().granularity()
            == editor::SelectionGranularity::Face &&
            activeDocument->editor().selection().mesh()
                .active_mesh().is_valid() &&
            !activeDocument->editor().selection().mesh()
                .faces().empty();
        shortcutContext.edgeSelectionContext =
            activeDocument->editor().selection().scope()
            == editor::SelectionScope::ActiveMesh &&
            activeDocument->editor().selection().granularity()
            == editor::SelectionGranularity::Edge &&
            activeDocument->editor().selection().mesh()
                .active_mesh().is_valid() &&
            !activeDocument->editor().selection().mesh()
                .edges().empty();
        shortcutContext.transformSelectionContext =
            shortcutContext.objectMode ||
            shortcutContext.vertexSelectionContext ||
            shortcutContext.edgeSelectionContext ||
            shortcutContext.faceSelectionContext;
        shortcutContext.transformToolActive =
            active_tool_is_transform(*activeDocument);

        const ShortcutAction shortcutAction =
            shortcutManager_.resolve(
                inputState_,
                shortcutContext);

        ApplicationResult<void> shortcutResult =
            execute_shortcut_action(
                shortcutAction,
                *activeDocument,
                editorViewport_);

        if (!shortcutResult) {
            inputState_.end_frame();
            return shortcutResult.error();
        }

        if (shortcutAction == ShortcutAction::Cancel) {
            inputRouter_.reset();
        }

        ApplicationResult<void> renderResult =
            editorViewport_.render(*activeDocument);

        if (!renderResult) {
            append_startup_log("ApplicationRuntime: render failed");
            inputState_.end_frame();
            return renderResult.error();
        }
        append_startup_log("ApplicationRuntime: render completed");

        const bool cameraCaptured =
            routeResult.owner == InputCaptureOwner::ViewportCamera;
        const bool editorToolCaptured =
            routeResult.owner == InputCaptureOwner::EditorTool;

        apply_selection_depth_policy(
            *activeDocument,
            editorViewport_);

        const auto pickingResult = editorViewport_.update_hover(
            *activeDocument,
            inputState_.cursor_position(),
            window_.width(),
            window_.height(),
            inputState_.focused(),
            cameraCaptured,
            !editorToolCaptured);

        if (!pickingResult) {
            inputState_.end_frame();
            return pickingResult.error();
        }

        const ViewportPickingResult& picking =
            pickingResult.value();

        if (!cameraCaptured
            && !editorToolCaptured
            && can_route_hover_to_tool(picking)) {
            editor::ToolEvent hoverEvent =
                make_pointer_event(
                    editor::ToolEventType::PointerMove,
                    editor::ToolPointerButton::None,
                    inputState_,
                    picking);
            populate_camera_pointer_data(
                hoverEvent,
                editorViewport_);
            populate_transform_pointer_scale(
                hoverEvent,
                *activeDocument,
                editorViewport_);

            ApplicationResult<void> hoverToolResult =
                dispatch_tool_event(*activeDocument, hoverEvent);

            if (!hoverToolResult) {
                inputState_.end_frame();
                return hoverToolResult.error();
            }
        }

        if (routeResult.owner == InputCaptureOwner::EditorTool) {
            if (routeResult.editorPointerPress
                && can_route_pointer_to_tool(picking)) {
                const bool transformWasActive =
                    active_tool_is_transform(*activeDocument);

                editor::ToolEvent pressEvent =
                    make_pointer_event(
                        editor::ToolEventType::PointerPress,
                        editor::ToolPointerButton::Primary,
                        inputState_,
                        picking);
                populate_camera_pointer_data(
                    pressEvent,
                    editorViewport_);
                populate_transform_pointer_scale(
                    pressEvent,
                    *activeDocument,
                    editorViewport_);

                ApplicationResult<editor::ToolResult> pressToolResult =
                    dispatch_tool_event_result(
                        *activeDocument,
                        pressEvent);

                if (!pressToolResult) {
                    inputState_.end_frame();
                    return pressToolResult.error();
                }

                if (transformWasActive
                    && !pressToolResult.value().was_consumed()) {
                    ApplicationResult<void> selectFallbackResult =
                        route_press_to_select_tool(
                            *activeDocument,
                            pressEvent);

                    if (!selectFallbackResult) {
                        inputState_.end_frame();
                        return selectFallbackResult.error();
                    }
                }
            }

            if (routeResult.editorPointerMove) {
                editor::ToolEvent moveEvent =
                    make_pointer_event(
                        editor::ToolEventType::PointerMove,
                        editor::ToolPointerButton::None,
                        inputState_,
                        picking);
                populate_camera_pointer_data(
                    moveEvent,
                    editorViewport_);
                populate_transform_pointer_scale(
                    moveEvent,
                    *activeDocument,
                    editorViewport_);

                ApplicationResult<void> moveToolResult =
                    dispatch_tool_event(*activeDocument, moveEvent);

                if (!moveToolResult) {
                    inputState_.end_frame();
                    return moveToolResult.error();
                }
            }

            if (routeResult.editorPointerRelease) {
                editor::ToolEvent releaseEvent =
                    make_pointer_event(
                        editor::ToolEventType::PointerRelease,
                        editor::ToolPointerButton::Primary,
                        inputState_,
                        picking);
                populate_camera_pointer_data(
                    releaseEvent,
                    editorViewport_);
                populate_transform_pointer_scale(
                    releaseEvent,
                    *activeDocument,
                    editorViewport_);

                if (editor::SelectTool* selectTool =
                        active_select_tool(*activeDocument);
                    selectTool != nullptr &&
                    selectTool->is_box_selecting()) {
                    const auto regionIds =
                        editorViewport_.read_picking_region(
                            *activeDocument,
                            selectTool->selection_rect());

                    if (!regionIds) {
                        inputState_.end_frame();
                        return regionIds.error();
                    }

                    releaseEvent.pointer.regionalPickingIds =
                        regionIds.value();
                }

                ApplicationResult<void> releaseToolResult =
                    dispatch_tool_event(*activeDocument, releaseEvent);

                if (!releaseToolResult) {
                    inputState_.end_frame();
                    return releaseToolResult.error();
                }
            }
        }

        if (inputState_.focus_lost()) {
            ApplicationResult<void> focusToolResult =
                dispatch_tool_event(
                    *activeDocument,
                    editor::ToolEvent{
                        editor::ToolEventType::FocusLost });

            if (!focusToolResult) {
                inputState_.end_frame();
                return focusToolResult.error();
            }
        }

        if (editor::has_flag(
                activeDocument->editor().dirty_flags(),
                editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render)) {
            renderResult = editorViewport_.render(*activeDocument);

            if (!renderResult) {
                inputState_.end_frame();
                return renderResult.error();
            }

            if (routeResult.captureEnded &&
                routeResult.owner == InputCaptureOwner::EditorTool) {
                const auto refreshedPickingResult =
                    editorViewport_.update_hover(
                        *activeDocument,
                        inputState_.cursor_position(),
                        window_.width(),
                        window_.height(),
                        inputState_.focused(),
                        false,
                        false);

                if (!refreshedPickingResult) {
                    inputState_.end_frame();
                    return refreshedPickingResult.error();
                }
            }
        }

        window_.present();
        inputState_.end_frame();

        if (window_.should_close()) {
            state_.quitRequested = true;
        }

        return context;
    }

    void ApplicationRuntime::request_quit(int exitCode) noexcept
    {
        if (!initialized()) {
            return;
        }

        state_.quitRequested = true;
        state_.exitCode = exitCode;
        window_.request_close();
    }

    void ApplicationRuntime::shutdown()
    {
        const bool hasOwnedResources =
            editorViewport_.initialized()
            || !documents_.empty()
            || window_.initialized();

        if (!hasOwnedResources) {
            if (state_.phase == ApplicationPhase::Initializing
                || state_.phase == ApplicationPhase::Running
                || state_.phase == ApplicationPhase::Suspended
                || state_.phase == ApplicationPhase::Stopping
                || state_.phase == ApplicationPhase::Failed) {
                state_.phase = ApplicationPhase::Stopped;
            }

            return;
        }

        state_.phase = ApplicationPhase::Stopping;
        window_.disconnect_input();
        inputRouter_.reset();
        inputState_.reset();
        editorViewport_.shutdown();
        documents_ = DocumentManager{};
        window_.shutdown();
        state_.phase = ApplicationPhase::Stopped;
    }

    bool ApplicationRuntime::initialized() const noexcept
    {
        return window_.initialized()
            && editorViewport_.initialized()
            && (state_.phase == ApplicationPhase::Running
                || state_.phase == ApplicationPhase::Suspended);
    }

    const ApplicationConfig&
    ApplicationRuntime::configuration() const noexcept
    {
        return configuration_;
    }

    const ApplicationState& ApplicationRuntime::state() const noexcept
    {
        return state_;
    }

    ApplicationWindow& ApplicationRuntime::window() noexcept
    {
        return window_;
    }

    const ApplicationWindow& ApplicationRuntime::window() const noexcept
    {
        return window_;
    }

    DocumentManager& ApplicationRuntime::documents() noexcept
    {
        return documents_;
    }

    const DocumentManager& ApplicationRuntime::documents() const noexcept
    {
        return documents_;
    }

    EditorViewport& ApplicationRuntime::editor_viewport() noexcept
    {
        return editorViewport_;
    }

    const EditorViewport&
    ApplicationRuntime::editor_viewport() const noexcept
    {
        return editorViewport_;
    }

    InputState& ApplicationRuntime::input_state() noexcept
    {
        return inputState_;
    }

    const InputState& ApplicationRuntime::input_state() const noexcept
    {
        return inputState_;
    }

    InputRouter& ApplicationRuntime::input_router() noexcept
    {
        return inputRouter_;
    }

    const InputRouter& ApplicationRuntime::input_router() const noexcept
    {
        return inputRouter_;
    }

    ShortcutManager& ApplicationRuntime::shortcut_manager() noexcept
    {
        return shortcutManager_;
    }

    const ShortcutManager&
    ApplicationRuntime::shortcut_manager() const noexcept
    {
        return shortcutManager_;
    }

} // namespace locus::application
