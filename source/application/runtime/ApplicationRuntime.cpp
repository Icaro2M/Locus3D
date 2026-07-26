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

        window_.process_events();

        const FrameContext context = frameClock_.tick();
        state_.frameIndex = frameClock_.next_frame_index();

        DocumentSession* activeDocument = documents_.active_document();

        if (!activeDocument) {
            return ApplicationError::make(
                ApplicationErrorCode::InternalFailure,
                "ApplicationRuntime has no active document to render.");
        }

        editorViewport_.resize(
            window_.framebuffer_width(),
            window_.framebuffer_height());

        ApplicationResult<void> renderResult =
            editorViewport_.render(*activeDocument);

        if (!renderResult) {
            return renderResult.error();
        }

        window_.present();

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

} // namespace locus::application
