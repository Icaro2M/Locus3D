/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/input/InputRouter.h"

#include "application/input/InputState.h"
#include "application/viewport/EditorViewport.h"

namespace locus::application {

    void InputRouter::route(
        const InputState& input,
        EditorViewport& viewport)
    {
        routedOwner_ = InputCaptureOwner::None;

        if (!input.focused() || input.focus_lost()) {
            reset();
            return;
        }

        const bool captureBegan =
            !capture_.active() && begin_capture(input);

        routedOwner_ = capture_.owner();

        if (capture_.owner() == InputCaptureOwner::ViewportCamera
            && !captureBegan) {
            route_camera_drag(input, viewport);
        }

        if (capture_.owner() != InputCaptureOwner::EditorTool
            && input.scroll_delta().y != 0.0) {
            viewport.zoom_camera(input.scroll_delta().y);
            routedOwner_ = InputCaptureOwner::ViewportCamera;
        }

        finish_released_capture(input);
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

    bool InputRouter::begin_capture(const InputState& input)
    {
        const bool alt =
            input.modifier_down(InputModifiers::Alt);

        if (alt && input.button_pressed(MouseButton::Left)) {
            if (capture_.acquire(
                    InputCaptureOwner::ViewportCamera,
                    MouseButton::Left)) {
                cameraDragMode_ = CameraDragMode::Orbit;
                return true;
            }
            return false;
        }

        if (alt && input.button_pressed(MouseButton::Middle)) {
            if (capture_.acquire(
                    InputCaptureOwner::ViewportCamera,
                    MouseButton::Middle)) {
                cameraDragMode_ = CameraDragMode::Pan;
                return true;
            }
            return false;
        }

        if (input.button_pressed(MouseButton::Left)) {
            return capture_.acquire(
                InputCaptureOwner::EditorTool,
                MouseButton::Left);
        }

        return false;
    }

    void InputRouter::route_camera_drag(
        const InputState& input,
        EditorViewport& viewport)
    {
        const InputVector2& delta = input.cursor_delta();

        switch (cameraDragMode_) {
        case CameraDragMode::Orbit:
            viewport.orbit_camera(delta.x, delta.y);
            break;

        case CameraDragMode::Pan:
            viewport.pan_camera(delta.x, delta.y);
            break;

        case CameraDragMode::None:
            break;
        }
    }

    void InputRouter::finish_released_capture(const InputState& input)
    {
        if (!capture_.active()) {
            return;
        }

        const MouseButton capturedButton = capture_.button();
        if (input.button_released(capturedButton)) {
            (void)capture_.release_for(capturedButton);
            cameraDragMode_ = CameraDragMode::None;
        }
    }

} // namespace locus::application
