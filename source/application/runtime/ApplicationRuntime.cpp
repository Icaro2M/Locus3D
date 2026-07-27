/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/runtime/ApplicationRuntime.h"

namespace locus::application {

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
