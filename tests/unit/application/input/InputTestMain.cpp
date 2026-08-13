/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/input/InputRouter.h"
#include "application/input/InputState.h"
#include "application/document/DocumentSession.h"
#include "application/shortcut/ShortcutManager.h"
#include "application/tools/MeshToolActivationController.h"
#include "application/viewport/EditorViewport.h"
#include "graphics/renderer/Renderer.h"
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/mesh/edge/EdgeSlideTool.h"
#include "editor/tools/mesh/edge/BevelTool.h"
#include "editor/tools/mesh/face/ExtrudeFaceTool.h"
#include "editor/tools/mesh/face/InsetFaceTool.h"
#include "editor/tools/mesh/face/SolidifyTool.h"
#include "editor/tools/mesh/topology/LoopCutTool.h"
#include "editor/tools/mesh/vertex/ShrinkFattenTool.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/transform/TransformTool.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyBuilder.h"

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

[[nodiscard]] bool near_vec3(
    const glm::vec3& lhs,
    const glm::vec3& rhs)
{
    return glm::length(lhs - rhs) < 0.0001f;
}

[[nodiscard]] glm::vec3 safe_normalize(
    const glm::vec3& value,
    const glm::vec3& fallback)
{
    const float length = glm::length(value);

    if (length <= 0.000001f) {
        return fallback;
    }

    return value / length;
}

[[nodiscard]] glm::vec3 vertex_normal(
    const locus::kernel::geometry::LEM& mesh,
    locus::kernel::geometry::VertexHandle vertex)
{
    glm::vec3 normal{ 0.0f };

    for (const locus::kernel::geometry::FaceHandle face :
        locus::kernel::geometry::TopologyTraversal::vertex_faces(
            mesh,
            vertex)) {
        if (mesh.is_valid(face)) {
            normal +=
                locus::kernel::geometry::NormalBuilder::face_normal(
                    mesh,
                    face);
        }
    }

    return safe_normalize(
        normal,
        glm::vec3{ 0.0f, 1.0f, 0.0f });
}

[[nodiscard]] locus::editor::ToolEvent shrink_fatten_pointer_event(
    locus::editor::ToolEventType type,
    locus::editor::ToolPointerButton button,
    glm::vec2 position,
    const glm::vec3& normal)
{
    locus::editor::ToolEvent event{};
    event.type = type;
    event.button = button;
    event.pointer.viewportPosition = position;
    event.pointer.worldRay.direction = normal;
    event.pointer.viewRight = normal;
    event.pointer.viewUp = glm::vec3{ 0.0f, 1.0f, 0.0f };
    event.pointer.visualScale = 1.0f;
    return event;
}

[[nodiscard]] const char* shortcut_action_name(
    ShortcutAction action)
{
    switch (action) {
    case ShortcutAction::None:
        return "None";
    case ShortcutAction::ActivateSelectTool:
        return "ActivateSelectTool";
    case ShortcutAction::ActivateTranslateTool:
        return "ActivateTranslateTool";
    case ShortcutAction::ActivateRotateTool:
        return "ActivateRotateTool";
    case ShortcutAction::ActivateScaleTool:
        return "ActivateScaleTool";
    case ShortcutAction::ActivateUniversalTool:
        return "ActivateUniversalTool";
    case ShortcutAction::ActivateExtrudeFaceTool:
        return "ActivateExtrudeFaceTool";
    case ShortcutAction::ActivateInsetFaceTool:
        return "ActivateInsetFaceTool";
    case ShortcutAction::ActivateSolidifyTool:
        return "ActivateSolidifyTool";
    case ShortcutAction::ActivateShrinkFattenTool:
        return "ActivateShrinkFattenTool";
    case ShortcutAction::ActivateEdgeSlideTool:
        return "ActivateEdgeSlideTool";
    case ShortcutAction::ActivateBevelTool:
        return "ActivateBevelTool";
    case ShortcutAction::ActivateLoopCutTool:
        return "ActivateLoopCutTool";
    case ShortcutAction::ExecuteBridgeEdgeAction:
        return "ExecuteBridgeEdgeAction";
    case ShortcutAction::ExecuteFillHoleAction:
        return "ExecuteFillHoleAction";
    case ShortcutAction::ExecuteFlipFacesAction:
        return "ExecuteFlipFacesAction";
    case ShortcutAction::ExecuteDissolveAction:
        return "ExecuteDissolveAction";
    case ShortcutAction::SetObjectGranularity:
        return "SetObjectGranularity";
    case ShortcutAction::SetVertexGranularity:
        return "SetVertexGranularity";
    case ShortcutAction::SetEdgeGranularity:
        return "SetEdgeGranularity";
    case ShortcutAction::SetFaceGranularity:
        return "SetFaceGranularity";
    case ShortcutAction::ToggleProjection:
        return "ToggleProjection";
    case ShortcutAction::FrontView:
        return "FrontView";
    case ShortcutAction::BackView:
        return "BackView";
    case ShortcutAction::LeftView:
        return "LeftView";
    case ShortcutAction::RightView:
        return "RightView";
    case ShortcutAction::TopView:
        return "TopView";
    case ShortcutAction::BottomView:
        return "BottomView";
    case ShortcutAction::ToggleViewportShading:
        return "ToggleViewportShading";
    case ShortcutAction::ToggleFaceOrientation:
        return "ToggleFaceOrientation";
    case ShortcutAction::Undo:
        return "Undo";
    case ShortcutAction::Redo:
        return "Redo";
    case ShortcutAction::Save:
        return "Save";
    case ShortcutAction::SaveAs:
        return "SaveAs";
    case ShortcutAction::Open:
        return "Open";
    case ShortcutAction::DeleteSelection:
        return "DeleteSelection";
    case ShortcutAction::Cancel:
        return "Cancel";
    }

    return "Unknown";
}

[[nodiscard]] bool expect_shortcut(
    const ShortcutManager& shortcuts,
    const InputState& state,
    const ShortcutContext& context,
    ShortcutAction expected,
    const char* label)
{
    const ShortcutAction actual =
        shortcuts.resolve(state, context);

    if (actual == expected) {
        return true;
    }

    std::cerr
        << label
        << ": expected " << shortcut_action_name(expected)
        << ", got " << shortcut_action_name(actual)
        << '\n';

    return false;
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

[[nodiscard]] bool test_orthographic_viewport_navigation()
{
    EditorViewport viewport{};
    viewport.resize(1600, 800);

    viewport.orbit_rig().set_distance(6.0f);
    viewport.viewport().camera().projection().set_perspective(
        0.78539816339f,
        viewport.aspect_ratio(),
        0.01f,
        1000.0f);

    viewport.set_projection_mode(
        locus::graphics::ProjectionType::Orthographic);

    if (viewport.projection_mode()
            != locus::graphics::ProjectionType::Orthographic
        || viewport.viewport().camera().projection().orthographic_height()
            <= 0.0f) {
        return false;
    }

    const float expectedHeight =
        2.0f * viewport.orbit_rig().distance()
        * std::tan(0.78539816339f * 0.5f);

    if (!near(
            viewport.viewport().camera().projection()
                .orthographic_height(),
            expectedHeight)) {
        return false;
    }

    const float distanceBeforeZoom =
        viewport.orbit_rig().distance();
    const float heightBeforeZoom =
        viewport.viewport().camera().projection().orthographic_height();
    viewport.zoom_camera(1.0);

    if (!near(viewport.orbit_rig().distance(), distanceBeforeZoom)
        || !(viewport.viewport().camera().projection()
            .orthographic_height() < heightBeforeZoom)) {
        return false;
    }

    const glm::vec3 targetBeforePan =
        viewport.orbit_rig().target();
    viewport.pan_camera(10.0, -20.0);

    if (glm::length(viewport.orbit_rig().target() - targetBeforePan)
        <= 0.0001f) {
        return false;
    }

    viewport.orbit_camera(8.0, 4.0);
    if (viewport.projection_mode()
            != locus::graphics::ProjectionType::Orthographic
        || viewport.view_orientation() != ViewOrientation::User) {
        return false;
    }

    const glm::vec3 pivotBeforeView =
        viewport.orbit_rig().target();
    viewport.set_view_orientation(ViewOrientation::Top);

    return viewport.projection_mode()
            == locus::graphics::ProjectionType::Orthographic
        && viewport.view_orientation() == ViewOrientation::Top
        && glm::length(
            viewport.orbit_rig().target() - pivotBeforeView)
            <= 0.0001f
        && glm::dot(
            viewport.viewport().camera().forward(),
            glm::vec3{ 0.0f, -1.0f, 0.0f }) > 0.999f
        && glm::dot(
            viewport.viewport().camera().up(),
            glm::vec3{ 0.0f, 0.0f, -1.0f }) > 0.999f;
}

[[nodiscard]] bool test_viewport_visual_scale_tracks_world_point()
{
    EditorViewport viewport{};
    viewport.resize(1000, 500);
    viewport.viewport().camera().look_at(
        glm::vec3{ 0.0f, 0.0f, 10.0f },
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 1.0f, 0.0f });
    viewport.viewport().camera().projection().set_perspective(
        0.78539816339f,
        viewport.aspect_ratio(),
        0.01f,
        1000.0f);

    const float originScale =
        viewport.visual_scale_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const float fartherScale =
        viewport.visual_scale_at(glm::vec3{ 0.0f, 0.0f, -10.0f });

    if (!(fartherScale > originScale * 1.9f)) {
        std::cerr
            << "Perspective visual scale should follow the target point depth\n";
        return false;
    }

    viewport.viewport().camera().projection().set_orthographic(
        8.0f,
        viewport.aspect_ratio(),
        0.01f,
        1000.0f);

    const float orthoOriginScale =
        viewport.visual_scale_at(glm::vec3{ 0.0f, 0.0f, 0.0f });
    const float orthoFartherScale =
        viewport.visual_scale_at(glm::vec3{ 0.0f, 0.0f, -10.0f });

    return near(orthoOriginScale, orthoFartherScale);
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

[[nodiscard]] bool test_viewport_shading_state()
{
    EditorViewport viewport{};

    if (viewport.shading_mode() != ViewportShadingMode::Solid) {
        std::cerr << "Viewport shading should default to Solid\n";
        return false;
    }

    if (viewport.face_orientation_enabled()) {
        std::cerr << "Face Orientation should default to disabled\n";
        return false;
    }

    viewport.set_face_orientation_enabled(true);
    if (!viewport.face_orientation_enabled()) {
        std::cerr << "Viewport should enable Face Orientation\n";
        return false;
    }

    viewport.toggle_face_orientation();
    if (viewport.face_orientation_enabled()) {
        std::cerr << "Face Orientation toggle should disable\n";
        return false;
    }

    viewport.toggle_face_orientation();
    if (!viewport.face_orientation_enabled()) {
        std::cerr << "Face Orientation toggle should enable\n";
        return false;
    }

    viewport.set_face_orientation_enabled(false);

    viewport.set_shading_mode(ViewportShadingMode::Wireframe);
    if (viewport.shading_mode() != ViewportShadingMode::Wireframe) {
        std::cerr << "Viewport should accept Wireframe shading\n";
        return false;
    }

    viewport.toggle_shading_mode();
    if (viewport.shading_mode() != ViewportShadingMode::Solid) {
        std::cerr << "Wireframe should toggle back to Solid\n";
        return false;
    }

    viewport.toggle_shading_mode();
    if (viewport.shading_mode() != ViewportShadingMode::Wireframe) {
        std::cerr << "Solid should toggle to Wireframe\n";
        return false;
    }

    const ViewportShadingFrameConfig solid =
        viewport_shading_frame_config(ViewportShadingMode::Solid);
    const ViewportShadingFrameConfig wireframe =
        viewport_shading_frame_config(ViewportShadingMode::Wireframe);
    const ViewportDisplaySettings wireframeOrientation{
        ViewportShadingMode::Wireframe,
        true
    };
    const ViewportShadingFrameConfig wireframeWithOrientation =
        viewport_shading_frame_config(wireframeOrientation);

    if (!solid.surfaceColorPass ||
        solid.surfaceDepthPrepass ||
        !solid.topologyVisibleEdges ||
        solid.topologyOccludedEdges ||
        !solid.topologySurfaceOverlays ||
        solid.pointSelectionDepthMode !=
            locus::editor::SelectionDepthMode::VisibleOnly ||
        solid.regionSelectionDepthMode !=
            locus::editor::SelectionDepthMode::VisibleOnly ||
        !solid.topologyDepthTest ||
        !solid.attenuateOccludedGizmo) {
        std::cerr << "Solid shading frame config regressed\n";
        return false;
    }

    if (wireframe.surfaceColorPass ||
        wireframe.surfaceDepthPrepass ||
        !wireframe.topologyVisibleEdges ||
        wireframe.topologyOccludedEdges ||
        wireframe.topologySurfaceOverlays ||
        wireframe.pointSelectionDepthMode !=
            locus::editor::SelectionDepthMode::Through ||
        wireframe.regionSelectionDepthMode !=
            locus::editor::SelectionDepthMode::Through ||
        wireframe.topologyDepthTest ||
        wireframe.attenuateOccludedGizmo) {
        std::cerr
            << "Wireframe shading frame config should draw topology through without invisible surface depth\n";
        return false;
    }

    if (!wireframeWithOrientation.surfaceColorPass ||
        wireframeWithOrientation.surfaceDepthPrepass ||
        !wireframeWithOrientation.topologyVisibleEdges ||
        wireframeWithOrientation.topologyOccludedEdges ||
        !wireframeWithOrientation.topologySurfaceOverlays ||
        wireframeWithOrientation.pointSelectionDepthMode !=
            locus::editor::SelectionDepthMode::VisibleOnly ||
        wireframeWithOrientation.regionSelectionDepthMode !=
            locus::editor::SelectionDepthMode::VisibleOnly ||
        !wireframeWithOrientation.topologyDepthTest ||
        !wireframeWithOrientation.attenuateOccludedGizmo) {
        std::cerr
            << "Face Orientation should use solid visibility policy with diagnostic colors\n";
        return false;
    }

    const locus::graphics::RendererSurfaceState depthOnly =
        locus::graphics::Renderer::depth_only_surface_state();

    const locus::graphics::RendererSurfaceState foreground =
        locus::graphics::Renderer::foreground_overlay_state();

    locus::graphics::Renderer renderer{};
    if (renderer.face_orientation_display().enabled) {
        std::cerr << "Renderer Face Orientation should default to disabled\n";
        return false;
    }

    locus::graphics::FaceOrientationDisplay orientationDisplay{};
    orientationDisplay.enabled = true;
    renderer.set_face_orientation_display(orientationDisplay);
    if (!renderer.face_orientation_display().enabled) {
        std::cerr << "Renderer should accept Face Orientation display state\n";
        return false;
    }

    renderer.set_face_orientation_display({});
    if (renderer.face_orientation_display().enabled) {
        std::cerr << "Renderer Face Orientation display should reset\n";
        return false;
    }

    return depthOnly.depthTest &&
        depthOnly.depthWrite &&
        depthOnly.depthFunc == locus::graphics::DepthFunc::Less &&
        !depthOnly.colorWrite &&
        !depthOnly.blend &&
        near(depthOnly.vertexAlphaMultiplier, 1.0f) &&
        !depthOnly.cullFace &&
        depthOnly.polygonMode == locus::graphics::RenderPolygonMode::Fill &&
        !foreground.depthTest &&
        !foreground.depthWrite &&
        foreground.colorWrite &&
        !foreground.blend &&
        near(foreground.vertexAlphaMultiplier, 1.0f) &&
        foreground.polygonMode == locus::graphics::RenderPolygonMode::Fill;
}

[[nodiscard]] bool test_shortcut_resolution()
{
    ShortcutManager shortcuts{};
    ShortcutContext context{};
    InputState state{};

    context.transformSelectionContext = true;

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Z, InputModifiers::Control));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::Undo,
            "Undo shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(
        Key::Z,
        InputModifiers::Control | InputModifiers::Shift));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::Redo,
            "Redo shortcut")) {
        return false;
    }

    context.vertexSelectionContext = true;

    state.reset();
    state.begin_frame();
    state.consume(key(Key::X));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ExecuteDissolveAction,
            "Dissolve shortcut")) {
        return false;
    }

    context.vertexSelectionContext = false;

    state.reset();
    state.begin_frame();
    state.consume(key(Key::W));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateTranslateTool,
            "Translate shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::E));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateRotateTool,
            "Rotate shortcut")) {
        return false;
    }

    context.objectMode = false;
    context.transformSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Transform shortcut blocked without transform target")) {
        return false;
    }

    context.objectMode = true;
    context.transformSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::T));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateUniversalTool,
            "Universal transform shortcut")) {
        return false;
    }

    context.objectMode = false;
    context.faceSelectionContext = true;
    context.transformSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::E));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateExtrudeFaceTool,
            "Extrude shortcut in face context")) {
        return false;
    }

    context.faceSelectionContext = false;
    context.transformSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Extrude shortcut blocked without selected face")) {
        return false;
    }

    context.faceSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::I));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateInsetFaceTool,
            "Inset shortcut in face context")) {
        return false;
    }

    context.faceSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Inset shortcut blocked without selected face")) {
        return false;
    }

    context.faceSelectionContext = true;
    context.transformSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::F));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateSolidifyTool,
            "Solidify shortcut in face context")) {
        return false;
    }

    context.faceSelectionContext = false;
    context.transformSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Solidify shortcut blocked without selected face")) {
        return false;
    }

    context.faceSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::F, InputModifiers::Alt));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ExecuteFlipFacesAction,
            "Flip Faces shortcut in face context")) {
        return false;
    }

    context.faceSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Flip Faces shortcut blocked without selected face")) {
        return false;
    }

    context.vertexSelectionContext = true;
    context.transformSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::S, InputModifiers::Alt));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateShrinkFattenTool,
            "Shrink/fatten shortcut in vertex context")) {
        return false;
    }

    context.vertexSelectionContext = false;
    context.transformSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Shrink/fatten shortcut blocked without selected vertex")) {
        return false;
    }

    context.edgeSelectionContext = true;
    context.transformSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::G));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateEdgeSlideTool,
            "Edge slide shortcut in edge context")) {
        return false;
    }

    context.edgeSelectionContext = false;
    context.transformSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Edge slide shortcut blocked without selected edge")) {
        return false;
    }

    context.edgeSelectionContext = true;
    context.objectMode = false;
    context.transformSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::R));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ActivateLoopCutTool,
            "Loop cut shortcut in edge context")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::J));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ExecuteBridgeEdgeAction,
            "Bridge Edge shortcut in edge context")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::F));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ExecuteFillHoleAction,
            "Fill Hole shortcut in edge context")) {
        return false;
    }

    context.edgeSelectionContext = false;
    context.transformSelectionContext = false;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Bridge Edge shortcut blocked without selected edge")) {
        return false;
    }

    context.objectMode = true;
    context.transformSelectionContext = true;
    state.reset();
    state.begin_frame();
    state.consume(key(Key::Num1));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::SetObjectGranularity,
            "Object granularity shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Num2));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::SetVertexGranularity,
            "Vertex granularity shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Num3));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::SetEdgeGranularity,
            "Edge granularity shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Num4));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::SetFaceGranularity,
            "Face granularity shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Num5));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ToggleProjection,
            "Projection toggle shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Num1, InputModifiers::Alt));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::FrontView,
            "Front view shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(Key::Num5, InputModifiers::Alt));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::TopView,
            "Top view shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(
        Key::W,
        InputModifiers::Control | InputModifiers::Alt));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ToggleViewportShading,
            "Viewport shading shortcut")) {
        return false;
    }

    state.reset();
    state.begin_frame();
    state.consume(key(
        Key::N,
        InputModifiers::Control | InputModifiers::Alt));

    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::ToggleFaceOrientation,
            "Face Orientation shortcut")) {
        return false;
    }

    context.modalActive = true;
    if (!expect_shortcut(
            shortcuts,
            state,
            context,
            ShortcutAction::None,
            "Granularity shortcut blocked during modal interaction")) {
        return false;
    }

    context.modalActive = false;
    context.textInputActive = true;
    return expect_shortcut(
        shortcuts,
        state,
        context,
        ShortcutAction::None,
        "Shortcut blocked while text input is active");
}

[[nodiscard]] bool test_mesh_tool_registration_and_activation()
{
    DocumentSession document{ DocumentId{ 42u } };

    const locus::editor::ToolId extrudeId{
        std::string{ locus::editor::ExtrudeFaceTool::Id }
    };
    const locus::editor::ToolId insetId{
        std::string{ locus::editor::InsetFaceTool::Id }
    };
    const locus::editor::ToolId solidifyId{
        std::string{ locus::editor::SolidifyTool::Id }
    };
    const locus::editor::ToolId shrinkFattenId{
        std::string{ locus::editor::ShrinkFattenTool::Id }
    };
    const locus::editor::ToolId edgeSlideId{
        std::string{ locus::editor::EdgeSlideTool::Id }
    };
    const locus::editor::ToolId bevelId{
        std::string{ locus::editor::BevelTool::Id }
    };
    const locus::editor::ToolId loopCutId{
        std::string{ locus::editor::LoopCutTool::Id }
    };

    if (!document.tool_registry().contains(extrudeId) ||
        !document.tool_registry().contains(insetId) ||
        !document.tool_registry().contains(solidifyId) ||
        !document.tool_registry().contains(shrinkFattenId) ||
        !document.tool_registry().contains(edgeSlideId) ||
        !document.tool_registry().contains(bevelId) ||
        !document.tool_registry().contains(loopCutId)) {
        std::cerr
            << "DocumentSession should register built-in mesh tools\n";
        return false;
    }

    if (document.tool_registry().create(extrudeId) == nullptr ||
        document.tool_registry().create(insetId) == nullptr ||
        document.tool_registry().create(solidifyId) == nullptr ||
        document.tool_registry().create(shrinkFattenId) == nullptr ||
        document.tool_registry().create(edgeSlideId) == nullptr ||
        document.tool_registry().create(bevelId) == nullptr ||
        document.tool_registry().create(loopCutId) == nullptr) {
        std::cerr
            << "Built-in mesh tool factories should create tools\n";
        return false;
    }

    const locus::editor::SceneNodeId meshId =
        document.editor().scene().create_mesh("Mesh");
    locus::editor::MeshNode* node =
        document.editor().scene().find_mesh(meshId);

    if (node == nullptr) {
        return false;
    }

    const auto buildResult =
        locus::kernel::geometry::TopologyBuilder::build_box_into(
            node->mesh());

    if (!buildResult ||
        buildResult.faces.empty() ||
        buildResult.edges.empty()) {
        return false;
    }

    if (!document.editor().selection_controller().enter_mesh_context(
            meshId,
            locus::editor::SelectionGranularity::Face) ||
        !document.editor().selection_controller().select_face(
            buildResult.faces.front())) {
        return false;
    }

    MeshToolActivationController activation{};

    const ApplicationResult<bool> extrudeResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateExtrudeFaceTool,
            document);

    if (!extrudeResult ||
        !extrudeResult.value() ||
        !document.tool_manager().is_active(extrudeId)) {
        std::cerr << "Extrude activation should stay functional\n";
        return false;
    }

    const ApplicationResult<bool> insetResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateInsetFaceTool,
            document);

    if (!insetResult ||
        !insetResult.value() ||
        !document.tool_manager().is_active(insetId)) {
        std::cerr << "Inset activation should stay functional\n";
        return false;
    }

    const locus::editor::ActionId bridgeActionId{
        std::string{ locus::editor::edge_actions::BridgeId }
    };

    if (!document.action_registry().contains(bridgeActionId)) {
        std::cerr
            << "DocumentSession should register built-in mesh actions\n";
        return false;
    }

    const ApplicationResult<bool> solidifyResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateSolidifyTool,
            document);

    if (!solidifyResult ||
        !solidifyResult.value() ||
        !document.tool_manager().is_active(solidifyId)) {
        std::cerr << "Solidify activation should stay functional\n";
        return false;
    }

    if (!document.editor().selection_controller().enter_mesh_context(
            meshId,
            locus::editor::SelectionGranularity::Vertex) ||
        !document.editor().selection_controller().select_vertex(
            buildResult.vertices.front())) {
        return false;
    }

    const ApplicationResult<bool> shrinkFattenResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateShrinkFattenTool,
            document);

    if (!shrinkFattenResult ||
        !shrinkFattenResult.value() ||
        !document.tool_manager().is_active(shrinkFattenId)) {
        std::cerr << "Shrink/fatten activation should stay functional\n";
        return false;
    }

    if (!document.editor().selection_controller().enter_mesh_context(
            meshId,
            locus::editor::SelectionGranularity::Edge) ||
        !document.editor().selection_controller().select_edge(
            buildResult.edges.front())) {
        return false;
    }

    const ApplicationResult<bool> edgeSlideResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateEdgeSlideTool,
            document);

    if (!edgeSlideResult ||
        !edgeSlideResult.value() ||
        !document.tool_manager().is_active(edgeSlideId)) {
        std::cerr << "Edge slide activation should stay functional\n";
        return false;
    }

    const ApplicationResult<bool> bevelResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateBevelTool,
            document);

    if (!bevelResult ||
        !bevelResult.value() ||
        !document.tool_manager().is_active(bevelId)) {
        std::cerr << "Bevel activation should stay functional\n";
        return false;
    }

    const ApplicationResult<bool> loopCutResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateLoopCutTool,
            document);

    const ApplicationResult<bool> bridgeToolResult =
        activation.activate_shortcut(
            ShortcutAction::ExecuteBridgeEdgeAction,
            document);

    return loopCutResult &&
        loopCutResult.value() &&
        document.tool_manager().is_active(loopCutId) &&
        activation.is_logged_preview_tool(document) &&
        bridgeToolResult &&
        !bridgeToolResult.value() &&
        edgeSlideResult &&
        edgeSlideResult.value() &&
        !document.tool_manager().is_active(edgeSlideId);
}

[[nodiscard]] bool test_transform_tool_activation_from_selection_contexts()
{
    DocumentSession document{ DocumentId{ 44u } };

    const locus::editor::SceneNodeId meshId =
        document.editor().scene().create_mesh("Mesh");
    locus::editor::MeshNode* node =
        document.editor().scene().find_mesh(meshId);

    if (node == nullptr) {
        return false;
    }

    const auto buildResult =
        locus::kernel::geometry::TopologyBuilder::build_box_into(
            node->mesh());

    if (!buildResult ||
        buildResult.vertices.empty() ||
        buildResult.edges.empty() ||
        buildResult.faces.empty()) {
        return false;
    }

    const locus::editor::ToolId transformId{
        std::string{ locus::editor::TransformTool::Id }
    };

    locus::editor::ToolContext toolContext{
        document.editor(),
        document.command_dispatcher(),
        document.history(),
        document.editor_sync().picking_sync()
    };

    document.editor().selection_controller().select_object(meshId);
    locus::editor::ToolResult result =
        document.tool_manager().activate_tool(
            toolContext,
            transformId);

    if (result.failed() ||
        !document.tool_manager().is_active(transformId)) {
        std::cerr << "Transform should activate for object selection\n";
        return false;
    }

    auto* transform =
        dynamic_cast<locus::editor::TransformTool*>(
            document.tool_manager().active_tool());

    if (transform == nullptr ||
        !transform->set_mode(locus::editor::GizmoMode::Translate)) {
        return false;
    }

    transform->refresh_gizmo_state(toolContext);
    if (!transform->gizmo_state().visible) {
        std::cerr << "Object transform should present a gizmo\n";
        return false;
    }

    const struct ComponentCase {
        locus::editor::SelectionGranularity granularity;
        locus::kernel::geometry::VertexHandle vertex;
        locus::kernel::geometry::EdgeHandle edge;
        locus::kernel::geometry::FaceHandle face;
    } cases[] = {
        {
            locus::editor::SelectionGranularity::Vertex,
            buildResult.vertices.front(),
            {},
            {}
        },
        {
            locus::editor::SelectionGranularity::Edge,
            {},
            buildResult.edges.front(),
            {}
        },
        {
            locus::editor::SelectionGranularity::Face,
            {},
            {},
            buildResult.faces.front()
        },
    };

    for (const ComponentCase& testCase : cases) {
        if (!document.editor().selection_controller().enter_mesh_context(
                meshId,
                testCase.granularity)) {
            return false;
        }

        switch (testCase.granularity) {
        case locus::editor::SelectionGranularity::Vertex:
            if (!document.editor().selection_controller().select_vertex(
                    testCase.vertex)) {
                return false;
            }
            break;
        case locus::editor::SelectionGranularity::Edge:
            if (!document.editor().selection_controller().select_edge(
                    testCase.edge)) {
                return false;
            }
            break;
        case locus::editor::SelectionGranularity::Face:
            if (!document.editor().selection_controller().select_face(
                    testCase.face)) {
                return false;
            }
            break;
        default:
            return false;
        }

        result = document.tool_manager().activate_tool(
            toolContext,
            transformId);

        if (result.failed() ||
            !document.tool_manager().is_active(transformId)) {
            std::cerr << "Transform should activate for component selection\n";
            return false;
        }

        transform =
            dynamic_cast<locus::editor::TransformTool*>(
                document.tool_manager().active_tool());

        if (transform == nullptr ||
            !transform->set_mode(locus::editor::GizmoMode::Translate)) {
            return false;
        }

        transform->refresh_gizmo_state(toolContext);
        const bool gizmoVisible = transform->gizmo_state().visible;
        if (!gizmoVisible) {
            std::cerr << "Component transform should present a gizmo\n";
            return false;
        }

        const locus::editor::ToolResult deactivateResult =
            document.tool_manager().deactivate_tool(toolContext);
        if (deactivateResult.failed()) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool test_shrink_fatten_tool_interaction()
{
    DocumentSession document{ DocumentId{ 43u } };

    const locus::editor::SceneNodeId meshId =
        document.editor().scene().create_mesh("Mesh");
    locus::editor::MeshNode* node =
        document.editor().scene().find_mesh(meshId);

    if (node == nullptr) {
        return false;
    }

    const auto buildResult =
        locus::kernel::geometry::TopologyBuilder::build_box_into(
            node->mesh());

    if (!buildResult ||
        buildResult.vertices.size() < 2u) {
        return false;
    }

    const locus::kernel::geometry::VertexHandle firstVertex =
        buildResult.vertices.front();
    const locus::kernel::geometry::VertexHandle secondVertex =
        buildResult.vertices[1u];

    if (!document.editor().selection_controller().enter_mesh_context(
            meshId,
            locus::editor::SelectionGranularity::Vertex) ||
        !document.editor().selection_controller().select_vertex(
            firstVertex) ||
        !document.editor().selection_controller().toggle_vertex(
            secondVertex)) {
        return false;
    }

    const glm::vec3 originalPosition =
        node->mesh().vertex(firstVertex).position;
    const glm::vec3 normal =
        safe_normalize(
            vertex_normal(
                node->mesh(),
                firstVertex) +
            vertex_normal(
                node->mesh(),
                secondVertex),
            glm::vec3{ 0.0f, 1.0f, 0.0f });

    MeshToolActivationController activation{};
    const ApplicationResult<bool> activationResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateShrinkFattenTool,
            document);

    const locus::editor::ToolId shrinkFattenId{
        std::string{ locus::editor::ShrinkFattenTool::Id }
    };

    if (!activationResult ||
        !activationResult.value() ||
        !document.tool_manager().is_active(shrinkFattenId)) {
        std::cerr << "Shrink/fatten should activate in vertex context\n";
        return false;
    }

    locus::editor::ToolContext toolContext{
        document.editor(),
        document.command_dispatcher(),
        document.history(),
        document.editor_sync().picking_sync()
    };

    const locus::editor::ToolResult pressResult =
        document.tool_manager().handle_event(
            toolContext,
            shrink_fatten_pointer_event(
                locus::editor::ToolEventType::PointerPress,
                locus::editor::ToolPointerButton::Primary,
                glm::vec2{ 0.0f, 0.0f },
                normal));

    if (pressResult.failed() ||
        !pressResult.was_consumed() ||
        !near_vec3(
            node->mesh().vertex(firstVertex).position,
            originalPosition)) {
        std::cerr << "Shrink/fatten press should start without mutation\n";
        return false;
    }

    const locus::editor::ToolResult moveResult =
        document.tool_manager().handle_event(
            toolContext,
            shrink_fatten_pointer_event(
                locus::editor::ToolEventType::PointerMove,
                locus::editor::ToolPointerButton::None,
                glm::vec2{ 20.0f, 0.0f },
                normal));

    auto* shrinkFattenTool =
        dynamic_cast<locus::editor::ShrinkFattenTool*>(
            document.tool_manager().active_tool());

    if (moveResult.failed() ||
        shrinkFattenTool == nullptr ||
        !shrinkFattenTool->has_operation_preview() ||
        std::abs(shrinkFattenTool->distance()) <= 0.000001f ||
        !near_vec3(
            node->mesh().vertex(firstVertex).position,
            originalPosition)) {
        std::cerr << "Shrink/fatten move should build preview only\n";
        return false;
    }

    const locus::editor::ToolResult releaseResult =
        document.tool_manager().handle_event(
            toolContext,
            shrink_fatten_pointer_event(
                locus::editor::ToolEventType::PointerRelease,
                locus::editor::ToolPointerButton::Primary,
                glm::vec2{ 20.0f, 0.0f },
                normal));

    const glm::vec3 committedPosition =
        node->mesh().vertex(firstVertex).position;

    if (releaseResult.failed() ||
        document.history().undo_size() != 1u ||
        near_vec3(committedPosition, originalPosition) ||
        !document.editor().selection().mesh().vertices().contains(
            firstVertex) ||
        !document.editor().selection().mesh().vertices().contains(
            secondVertex)) {
        std::cerr
            << "Shrink/fatten release should commit once and preserve "
            << "selection\n";
        return false;
    }

    const locus::editor::CommandResult undoResult =
        document.history().undo(
            document.command_dispatcher());

    if (!undoResult.success ||
        !near_vec3(
            node->mesh().vertex(firstVertex).position,
            originalPosition)) {
        std::cerr << "Shrink/fatten undo should restore the mesh\n";
        return false;
    }

    const locus::editor::CommandResult redoResult =
        document.history().redo(
            document.command_dispatcher());

    if (!redoResult.success ||
        !near_vec3(
            node->mesh().vertex(firstVertex).position,
            committedPosition)) {
        std::cerr << "Shrink/fatten redo should restore the commit\n";
        return false;
    }

    const std::size_t undoSizeBeforeCancel =
        document.history().undo_size();
    const glm::vec3 positionBeforeCancel =
        node->mesh().vertex(firstVertex).position;

    const ApplicationResult<bool> secondActivationResult =
        activation.activate_shortcut(
            ShortcutAction::ActivateShrinkFattenTool,
            document);

    if (!secondActivationResult ||
        !secondActivationResult.value()) {
        return false;
    }

    document.tool_manager().handle_event(
        toolContext,
        shrink_fatten_pointer_event(
            locus::editor::ToolEventType::PointerPress,
            locus::editor::ToolPointerButton::Primary,
            glm::vec2{ 0.0f, 0.0f },
            normal));
    document.tool_manager().handle_event(
        toolContext,
        shrink_fatten_pointer_event(
            locus::editor::ToolEventType::PointerMove,
            locus::editor::ToolPointerButton::None,
            glm::vec2{ -20.0f, 0.0f },
            normal));

    const locus::editor::ToolResult cancelResult =
        document.tool_manager().handle_event(
            toolContext,
            locus::editor::ToolEvent{
                locus::editor::ToolEventType::Cancel });

    return !cancelResult.failed() &&
        document.history().undo_size() == undoSizeBeforeCancel &&
        near_vec3(
            node->mesh().vertex(firstVertex).position,
            positionBeforeCancel);
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
        { "OrthographicViewportNavigation",
            &test_orthographic_viewport_navigation },
        { "ViewportVisualScaleTracksWorldPoint",
            &test_viewport_visual_scale_tracks_world_point },
        { "EditorCaptureAndFocusLoss", &test_editor_capture_and_focus_loss },
        { "ViewportShadingState", &test_viewport_shading_state },
        { "ShortcutResolution", &test_shortcut_resolution },
        { "MeshToolRegistrationAndActivation",
            &test_mesh_tool_registration_and_activation },
        { "TransformToolActivationFromSelectionContexts",
            &test_transform_tool_activation_from_selection_contexts },
        { "ShrinkFattenToolInteraction",
            &test_shrink_fatten_tool_interaction },
    };

    for (const TestCase& test : tests) {
        if (!test.run()) {
            std::cerr << test.name << ": failed\n";
            return 1;
        }
    }

    return 0;
}
