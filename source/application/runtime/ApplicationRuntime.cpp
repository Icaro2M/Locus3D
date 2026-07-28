/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/runtime/ApplicationRuntime.h"

#include "editor/EditorTypes.h"
#include "editor/command/CommandResult.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/selection/SelectTool.h"
#include "editor/tools/transform/TransformTool.h"
#include "graphics/camera/CameraRayBuilder.h"

#include <glm/vec2.hpp>

namespace locus::application {

    namespace {

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
            event.pointer.visualScale = 1.0f;

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
            editor::ToolContext toolContext = make_tool_context(document);
            const editor::ToolResult result =
                document.tool_manager().handle_event(
                    toolContext,
                    event);

            if (result.failed()) {
                return tool_failure(result).error();
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

            document.editor().mark_dirty(
                editor::EditorDirtyFlags::Render);
            return {};
        }

        [[nodiscard]] ApplicationResult<void> execute_shortcut_action(
            ShortcutAction action,
            DocumentSession& document)
        {
            switch (action) {
            case ShortcutAction::None:
            case ShortcutAction::Save:
            case ShortcutAction::Open:
            case ShortcutAction::DeleteSelection:
                return {};

            case ShortcutAction::ActivateSelectTool:
                return activate_tool(
                    document,
                    editor::ToolId{ editor::SelectTool::Id });

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

        window_.connect_input(inputState_);
        (void)documents_.create_document();

        ApplicationResult<void> viewportResult =
            editorViewport_.initialize(
                window_.framebuffer_width(),
                window_.framebuffer_height());

        if (!viewportResult) {
            documents_ = DocumentManager{};
            window_.shutdown();
            state_.phase = ApplicationPhase::Failed;
            state_.exitCode = 1;
            return viewportResult.error();
        }

        frameClock_.reset();
        state_.phase = ApplicationPhase::Running;
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
        shortcutContext.objectMode =
            activeDocument->editor().mode()
            == editor::EditorMode::Object;

        const ShortcutAction shortcutAction =
            shortcutManager_.resolve(
                inputState_,
                shortcutContext);

        ApplicationResult<void> shortcutResult =
            execute_shortcut_action(shortcutAction, *activeDocument);

        if (!shortcutResult) {
            inputState_.end_frame();
            return shortcutResult.error();
        }

        ApplicationResult<void> renderResult =
            editorViewport_.render(*activeDocument);

        if (!renderResult) {
            inputState_.end_frame();
            return renderResult.error();
        }

        const bool cameraCaptured =
            routeResult.owner == InputCaptureOwner::ViewportCamera;

        const auto pickingResult = editorViewport_.update_hover(
            *activeDocument,
            inputState_.cursor_position(),
            window_.width(),
            window_.height(),
            inputState_.focused(),
            cameraCaptured);

        if (!pickingResult) {
            inputState_.end_frame();
            return pickingResult.error();
        }

        const ViewportPickingResult& picking =
            pickingResult.value();

        if (!cameraCaptured
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

            if (routeResult.editorPointerMove
                && !can_route_hover_to_tool(picking)) {
                editor::ToolEvent moveEvent =
                    make_pointer_event(
                        editor::ToolEventType::PointerMove,
                        editor::ToolPointerButton::None,
                        inputState_,
                        picking);
                populate_camera_pointer_data(
                    moveEvent,
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
