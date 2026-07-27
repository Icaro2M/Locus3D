/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/input/InputRouter.h"
#include "application/input/InputState.h"
#include "application/viewport/EditorViewport.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <iostream>
#include <string_view>

namespace {

using namespace locus::application;

[[nodiscard]] InputEvent cursor(double x, double y)
{
    InputEvent event{};
    event.type = InputEventType::CursorMoved;
    event.cursorPosition = { x, y };
    return event;
}

[[nodiscard]] InputEvent button(
    InputEventType type,
    MouseButton mouseButton,
    InputModifiers modifiers = InputModifiers::None)
{
    InputEvent event{};
    event.type = type;
    event.mouseButton = mouseButton;
    event.modifiers = modifiers;
    return event;
}

[[nodiscard]] InputEvent scroll(double x, double y)
{
    InputEvent event{};
    event.type = InputEventType::Scrolled;
    event.scrollDelta = { x, y };
    return event;
}

[[nodiscard]] bool near(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 0.0001;
}

[[nodiscard]] bool test_input_state()
{
    InputState state{};
    state.reset();
    state.begin_frame();
    state.initialize_cursor({ 10.0, 20.0 });
    state.consume(cursor(15.0, 18.0));
    state.consume(cursor(20.0, 25.0));
    state.consume(button(
        InputEventType::MouseButtonPressed,
        MouseButton::Left,
        InputModifiers::Alt));

    InputEvent keyPress{};
    keyPress.type = InputEventType::KeyPressed;
    keyPress.key = 65;
    keyPress.modifiers = InputModifiers::Alt;
    state.consume(keyPress);
    state.consume(scroll(1.0, -2.0));

    if (!near(state.cursor_position().x, 20.0)
        || !near(state.cursor_position().y, 25.0)
        || !near(state.cursor_delta().x, 10.0)
        || !near(state.cursor_delta().y, 5.0)
        || !state.button_pressed(MouseButton::Left)
        || !state.button_down(MouseButton::Left)
        || !state.key_pressed(65)
        || !state.key_down(65)
        || !state.modifier_down(InputModifiers::Alt)
        || !near(state.scroll_delta().x, 1.0)
        || !near(state.scroll_delta().y, -2.0)) {
        return false;
    }

    state.end_frame();
    state.begin_frame();

    if (state.button_pressed(MouseButton::Left)
        || !state.button_down(MouseButton::Left)
        || state.key_pressed(65)
        || !state.key_down(65)
        || !near(state.cursor_delta().x, 0.0)
        || !near(state.scroll_delta().y, 0.0)) {
        return false;
    }

    state.consume(button(
        InputEventType::MouseButtonReleased,
        MouseButton::Left));

    InputEvent keyRelease{};
    keyRelease.type = InputEventType::KeyReleased;
    keyRelease.key = 65;
    state.consume(keyRelease);

    return state.button_released(MouseButton::Left)
        && !state.button_down(MouseButton::Left)
        && state.key_released(65)
        && !state.key_down(65);
}

[[nodiscard]] bool test_camera_routing_and_capture()
{
    InputState state{};
    InputRouter router{};
    EditorViewport viewport{};

    state.reset();
    state.initialize_cursor({ 100.0, 100.0 });
    state.begin_frame();
    const float initialYaw = viewport.orbit_rig().yaw_radians();
    const float initialPitch = viewport.orbit_rig().pitch_radians();
    state.consume(cursor(120.0, 100.0));
    state.consume(button(
        InputEventType::MouseButtonPressed,
        MouseButton::Left,
        InputModifiers::Alt));
    router.route(state, viewport);

    if (router.capture().owner()
            != InputCaptureOwner::ViewportCamera
        || router.capture().button() != MouseButton::Left
        || !near(viewport.orbit_rig().yaw_radians(), initialYaw)
        || !near(viewport.orbit_rig().pitch_radians(), initialPitch)) {
        return false;
    }

    state.end_frame();
    state.begin_frame();
    state.consume(cursor(130.0, 80.0));
    router.route(state, viewport);

    if (near(viewport.orbit_rig().yaw_radians(), initialYaw)
        || near(viewport.orbit_rig().pitch_radians(), initialPitch)
        || router.capture().owner()
            != InputCaptureOwner::ViewportCamera) {
        return false;
    }

    const float yawBeforeOutsideRelease =
        viewport.orbit_rig().yaw_radians();

    state.end_frame();
    state.begin_frame();
    state.consume(cursor(-25.0, 900.0));
    state.consume(button(
        InputEventType::MouseButtonReleased,
        MouseButton::Left));
    router.route(state, viewport);

    return !near(
            viewport.orbit_rig().yaw_radians(),
            yawBeforeOutsideRelease)
        && !router.capture().active();
}

[[nodiscard]] bool test_pan_zoom_and_invalid_gestures()
{
    InputState state{};
    InputRouter router{};
    EditorViewport viewport{};

    state.reset();
    state.initialize_cursor({ 50.0, 50.0 });
    state.begin_frame();
    state.consume(button(
        InputEventType::MouseButtonPressed,
        MouseButton::Middle,
        InputModifiers::Alt));
    router.route(state, viewport);

    const glm::vec3 targetBefore = viewport.orbit_rig().target();
    state.end_frame();
    state.begin_frame();
    state.consume(cursor(90.0, 70.0));
    router.route(state, viewport);

    if (glm::length(viewport.orbit_rig().target() - targetBefore)
        < 0.0001f) {
        return false;
    }

    router.reset();
    state.reset();
    state.begin_frame();
    const float distanceBefore = viewport.orbit_rig().distance();
    state.consume(scroll(0.0, 1.0));
    router.route(state, viewport);

    if (!(viewport.orbit_rig().distance() < distanceBefore)
        || router.capture().active()
        || router.routed_owner()
            != InputCaptureOwner::ViewportCamera) {
        return false;
    }

    router.reset();
    state.reset();
    state.initialize_cursor({ 0.0, 0.0 });
    state.begin_frame();
    const float yawBefore = viewport.orbit_rig().yaw_radians();
    const glm::vec3 targetBeforeInvalid =
        viewport.orbit_rig().target();
    state.consume(button(
        InputEventType::MouseButtonPressed,
        MouseButton::Right));
    router.route(state, viewport);
    state.end_frame();
    state.begin_frame();
    state.consume(cursor(100.0, 100.0));
    router.route(state, viewport);

    viewport.resize(800, 400);

    return near(viewport.orbit_rig().yaw_radians(), yawBefore)
        && glm::length(
            viewport.orbit_rig().target() - targetBeforeInvalid) < 0.0001f
        && !router.capture().active()
        && near(viewport.aspect_ratio(), 2.0);
}

[[nodiscard]] bool test_editor_capture_and_focus_loss()
{
    InputState state{};
    InputRouter router{};
    EditorViewport viewport{};

    state.reset();
    state.initialize_cursor({ 10.0, 10.0 });
    state.begin_frame();
    const float yawBefore = viewport.orbit_rig().yaw_radians();
    state.consume(button(
        InputEventType::MouseButtonPressed,
        MouseButton::Left));
    router.route(state, viewport);

    if (router.capture().owner() != InputCaptureOwner::EditorTool) {
        return false;
    }

    state.end_frame();
    state.begin_frame();
    state.consume(cursor(80.0, 40.0));
    router.route(state, viewport);

    if (!near(viewport.orbit_rig().yaw_radians(), yawBefore)
        || router.capture().owner() != InputCaptureOwner::EditorTool) {
        return false;
    }

    InputEvent focusLost{};
    focusLost.type = InputEventType::FocusLost;
    state.consume(focusLost);
    router.route(state, viewport);

    return !state.focused()
        && state.focus_lost()
        && state.button_released(MouseButton::Left)
        && !state.button_down(MouseButton::Left)
        && !router.capture().active();
}

} // namespace

int main()
{
    struct TestCase {
        std::string_view name;
        bool (*run)();
    };

    const TestCase tests[] = {
        { "InputState", &test_input_state },
        { "CameraRoutingAndCapture", &test_camera_routing_and_capture },
        { "PanZoomAndInvalidGestures", &test_pan_zoom_and_invalid_gestures },
        { "EditorCaptureAndFocusLoss", &test_editor_capture_and_focus_loss },
    };

    for (const TestCase& test : tests) {
        if (!test.run()) {
            std::cerr << test.name << ": failed\n";
            return 1;
        }
    }

    return 0;
}
