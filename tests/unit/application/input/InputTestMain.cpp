/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/input/InputRouter.h"
#include "application/input/InputState.h"
#include "application/shortcut/ShortcutManager.h"
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

[[nodiscard]] InputEvent key(
    Key keyValue,
    InputModifiers modifiers = InputModifiers::None)
{
    InputEvent event{};
    event.type = InputEventType::KeyPressed;
    event.key = keyValue;
    event.modifiers = modifiers;
    return event;
}

[[nodiscard]] bool near(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 0.0001;
}

void apply_route(
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
    keyPress.key = Key::A;
    keyPress.modifiers = InputModifiers::Alt;
    state.consume(keyPress);
    state.consume(scroll(1.0, -2.0));

    if (!near(state.cursor_position().x, 20.0)
        || !near(state.cursor_position().y, 25.0)
        || !near(state.cursor_delta().x, 10.0)
        || !near(state.cursor_delta().y, 5.0)
        || !state.button_pressed(MouseButton::Left)
        || !state.button_down(MouseButton::Left)
        || !state.key_pressed(Key::A)
        || !state.key_down(Key::A)
        || !state.modifier_down(InputModifiers::Alt)
        || !near(state.scroll_delta().x, 1.0)
        || !near(state.scroll_delta().y, -2.0)) {
        return false;
    }

    state.end_frame();
    state.begin_frame();

    if (state.button_pressed(MouseButton::Left)
        || !state.button_down(MouseButton::Left)
        || state.key_pressed(Key::A)
        || !state.key_down(Key::A)
        || !near(state.cursor_delta().x, 0.0)
        || !near(state.scroll_delta().y, 0.0)) {
        return false;
    }

    state.consume(button(
        InputEventType::MouseButtonReleased,
        MouseButton::Left));

    InputEvent keyRelease{};
    keyRelease.type = InputEventType::KeyReleased;
    keyRelease.key = Key::A;
    state.consume(keyRelease);

    return state.button_released(MouseButton::Left)
        && !state.button_down(MouseButton::Left)
        && state.key_released(Key::A)
        && !state.key_down(Key::A);
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
        MouseButton::Middle));
    const InputRouteResult pressRoute = router.route(state);
    apply_route(pressRoute, viewport);

    if (router.capture().owner()
            != InputCaptureOwner::ViewportCamera
        || router.capture().button() != MouseButton::Middle
        || pressRoute.destination
            != InputRouteDestination::ViewportCamera
        || !near(viewport.orbit_rig().yaw_radians(), initialYaw)
        || !near(viewport.orbit_rig().pitch_radians(), initialPitch)) {
        return false;
    }

    state.end_frame();
    state.begin_frame();
    state.consume(cursor(130.0, 80.0));
    const InputRouteResult dragRoute = router.route(state);
    apply_route(dragRoute, viewport);

    if (near(viewport.orbit_rig().yaw_radians(), initialYaw)
        || near(viewport.orbit_rig().pitch_radians(), initialPitch)
        || dragRoute.navigation != ViewportNavigationAction::Orbit
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
        MouseButton::Middle));
    const InputRouteResult releaseRoute = router.route(state);
    apply_route(releaseRoute, viewport);

    return !near(
            viewport.orbit_rig().yaw_radians(),
            yawBeforeOutsideRelease)
        && releaseRoute.captureEnded
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
        InputModifiers::Shift));
    const InputRouteResult pressRoute = router.route(state);
    apply_route(pressRoute, viewport);

    const glm::vec3 targetBefore = viewport.orbit_rig().target();
    state.end_frame();
    state.begin_frame();
    state.consume(cursor(90.0, 70.0));
    const InputRouteResult panRoute = router.route(state);
    apply_route(panRoute, viewport);

    if (glm::length(viewport.orbit_rig().target() - targetBefore)
        < 0.0001f
        || panRoute.navigation != ViewportNavigationAction::Pan) {
        return false;
    }

    router.reset();
    state.reset();
    state.begin_frame();
    const float distanceBefore = viewport.orbit_rig().distance();
    state.consume(scroll(0.0, 1.0));
    const InputRouteResult zoomRoute = router.route(state);
    apply_route(zoomRoute, viewport);

    if (!(viewport.orbit_rig().distance() < distanceBefore)
        || router.capture().active()
        || zoomRoute.destination
            != InputRouteDestination::ViewportCamera
        || zoomRoute.navigation != ViewportNavigationAction::Zoom) {
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
    apply_route(router.route(state), viewport);
    state.end_frame();
    state.begin_frame();
    state.consume(cursor(100.0, 100.0));
    apply_route(router.route(state), viewport);

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
    const InputRouteResult pressRoute = router.route(state);

    if (router.capture().owner() != InputCaptureOwner::EditorTool
        || pressRoute.destination != InputRouteDestination::Editor
        || !pressRoute.editorPointerPress) {
        return false;
    }

    state.end_frame();
    state.begin_frame();
    state.consume(cursor(80.0, 40.0));
    const InputRouteResult moveRoute = router.route(state);

    if (!near(viewport.orbit_rig().yaw_radians(), yawBefore)
        || !moveRoute.editorPointerMove
        || router.capture().owner() != InputCaptureOwner::EditorTool) {
        return false;
    }

    InputEvent focusLost{};
    focusLost.type = InputEventType::FocusLost;
    state.consume(focusLost);
    const InputRouteResult focusRoute = router.route(state);

    return !state.focused()
        && state.focus_lost()
        && state.button_released(MouseButton::Left)
        && !state.button_down(MouseButton::Left)
        && focusRoute.captureEnded
        && !router.capture().active();
}

[[nodiscard]] bool test_shortcut_resolution()
{
    ShortcutManager shortcuts{};
    ShortcutContext context{};
    InputState state{};

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Z, InputModifiers::Control));

    if (shortcuts.resolve(state, context) != ShortcutAction::Undo) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(
        Key::Z,
        InputModifiers::Control | InputModifiers::Shift));

    if (shortcuts.resolve(state, context) != ShortcutAction::Redo) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::W));

    if (shortcuts.resolve(state, context)
        != ShortcutAction::ActivateTranslateTool) {
        return false;
    }

    context.objectMode = false;
    if (shortcuts.resolve(state, context) != ShortcutAction::None) {
        return false;
    }

    context.objectMode = true;
    context.textInputActive = true;
    return shortcuts.resolve(state, context) == ShortcutAction::None;
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
        { "ShortcutResolution", &test_shortcut_resolution },
    };

    for (const TestCase& test : tests) {
        if (!test.run()) {
            std::cerr << test.name << ": failed\n";
            return 1;
        }
    }

    return 0;
}
