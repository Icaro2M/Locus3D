/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/input/InputRouter.h"

#include "application/input/InputState.h"

namespace locus::application {

    InputRouteResult InputRouter::route(const InputState& input)
    {
        InputRouteResult result{};
        routedOwner_ = InputCaptureOwner::None;

        if (!input.focused() || input.focus_lost()) {
            reset();
            result.captureEnded = true;
            return result;
        }

        if (!capture_.active()) {
            result.captureBegan = begin_capture(input, result);
        }

        route_active_capture(input, result);
        route_scroll(input, result);
        finish_released_capture(input, result);

        routedOwner_ = result.owner;
        return result;
    }

    void InputRouter::reset() noexcept
    {
        capture_.cancel();
        cameraDragMode_ = CameraDragMode::None;
        routedOwner_ = InputCaptureOwner::None;
    }

    const InputCapture& InputRouter::capture() const noexcept
    {
        return capture_;
    }

    InputCaptureOwner InputRouter::routed_owner() const noexcept
    {
        return routedOwner_;
    }

    bool InputRouter::begin_capture(
        const InputState& input,
        InputRouteResult& result)
    {
        const bool shift =
            input.modifier_down(InputModifiers::Shift);

        if (input.button_pressed(MouseButton::Middle)) {
            if (capture_.acquire(
                    InputCaptureOwner::ViewportCamera,
                    MouseButton::Middle)) {
                cameraDragMode_ = shift
                    ? CameraDragMode::Pan
                    : CameraDragMode::Orbit;
                result.destination =
                    InputRouteDestination::ViewportCamera;
                result.owner = InputCaptureOwner::ViewportCamera;
                return true;
            }

            return false;
        }

        if (input.button_pressed(MouseButton::Left)) {
            if (capture_.acquire(
                    InputCaptureOwner::EditorTool,
                    MouseButton::Left)) {
                result.destination = InputRouteDestination::Editor;
                result.owner = InputCaptureOwner::EditorTool;
                result.editorPointerPress = true;
                return true;
            }
        }

        return false;
    }

    void InputRouter::route_active_capture(
        const InputState& input,
        InputRouteResult& result) const
    {
        if (!capture_.active()) {
            if (input.cursor_delta().x != 0.0
                || input.cursor_delta().y != 0.0) {
                result.destination = InputRouteDestination::Editor;
                result.owner = InputCaptureOwner::EditorTool;
                result.editorPointerMove = true;
            }

            return;
        }

        result.owner = capture_.owner();

        if (capture_.owner() == InputCaptureOwner::ViewportCamera) {
            result.destination = InputRouteDestination::ViewportCamera;
            result.cursorDelta = input.cursor_delta();

            switch (cameraDragMode_) {
            case CameraDragMode::Orbit:
                result.navigation =
                    ViewportNavigationAction::Orbit;
                break;

            case CameraDragMode::Pan:
                result.navigation =
                    ViewportNavigationAction::Pan;
                break;

            case CameraDragMode::None:
                break;
            }

            return;
        }

        if (capture_.owner() == InputCaptureOwner::EditorTool) {
            result.destination = InputRouteDestination::Editor;
            result.editorPointerMove =
                input.cursor_delta().x != 0.0
                || input.cursor_delta().y != 0.0;
            result.editorPointerPress =
                input.button_pressed(capture_.button());
            result.editorPointerRelease =
                input.button_released(capture_.button());
        }
    }

    void InputRouter::route_scroll(
        const InputState& input,
        InputRouteResult& result) const
    {
        if (input.scroll_delta().y == 0.0
            || capture_.owner() == InputCaptureOwner::EditorTool) {
            return;
        }

        result.destination = InputRouteDestination::ViewportCamera;
        result.owner = InputCaptureOwner::ViewportCamera;
        result.navigation = ViewportNavigationAction::Zoom;
        result.scrollDelta = input.scroll_delta().y;
    }

    void InputRouter::finish_released_capture(
        const InputState& input,
        InputRouteResult& result)
    {
        if (!capture_.active()) {
            return;
        }

        const MouseButton capturedButton = capture_.button();
        if (input.button_released(capturedButton)) {
            result.captureEnded =
                capture_.release_for(capturedButton);
            cameraDragMode_ = CameraDragMode::None;
        }
    }

} // namespace locus::application
