/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/runtime/ApplicationRuntime.h"

#include "editor/EditorTypes.h"
#include "editor/command/CommandResult.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"

#include <glm/vec2.hpp>

namespace locus::application {

    namespace {

        constexpr KeyCode KeyZ = 90;
        constexpr KeyCode KeyY = 89;

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

        [[nodiscard]] ApplicationResult<void> tool_failure(
            const editor::ToolResult& result)
        {
            return ApplicationError::make(
                ApplicationErrorCode::RuntimeFailure,
                result.message.empty()
                    ? "Editor tool input failed."
                    : result.message);
        }

        [[nodiscard]] ApplicationResult<void> dispatch_tool_event(
            DocumentSession& document,
            const editor::ToolEvent& event)
        {
            editor::ToolContext toolContext = make_tool_context(document);
            const editor::ToolResult result =
                document.tool_manager().handle_event(
                    toolContext,
                    event);

            if (result.failed()) {
                return tool_failure(result);
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

        [[nodiscard]] ApplicationResult<void> route_editor_shortcuts(
            const InputState& input,
            DocumentSession& document)
        {
            if (!input.modifier_down(InputModifiers::Control)) {
                return {};
            }

            const bool redo =
                input.key_pressed(KeyY)
                || (input.key_pressed(KeyZ)
                    && input.modifier_down(InputModifiers::Shift));

            const bool undo =
                input.key_pressed(KeyZ) && !redo;

            if (!undo && !redo) {
                return {};
            }

            const editor::CommandResult result =
                redo
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

        inputRouter_.route(inputState_, editorViewport_);

        ApplicationResult<void> shortcutResult =
            route_editor_shortcuts(inputState_, *activeDocument);

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
            inputRouter_.capture().owner()
            == InputCaptureOwner::ViewportCamera;

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
            ApplicationResult<void> hoverToolResult =
                dispatch_tool_event(
                    *activeDocument,
                    make_pointer_event(
                        editor::ToolEventType::PointerMove,
                        editor::ToolPointerButton::None,
                        inputState_,
                        picking));

            if (!hoverToolResult) {
                inputState_.end_frame();
                return hoverToolResult.error();
            }
        }

        if (inputRouter_.capture().owner()
                == InputCaptureOwner::EditorTool
            && inputState_.button_pressed(MouseButton::Left)
            && can_route_pointer_to_tool(picking)) {
            ApplicationResult<void> pressToolResult =
                dispatch_tool_event(
                    *activeDocument,
                    make_pointer_event(
                        editor::ToolEventType::PointerPress,
                        editor::ToolPointerButton::Primary,
                        inputState_,
                        picking));

            if (!pressToolResult) {
                inputState_.end_frame();
                return pressToolResult.error();
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

} // namespace locus::application
