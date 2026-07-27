/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/topology/TopologyBuilder.h"

#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

using namespace locus;

[[nodiscard]] int fail(
    application::ApplicationRuntime& runtime,
    std::string_view message)
{
    std::cerr << "[picking integration] " << message << '\n';
    runtime.shutdown();
    return 1;
}

[[nodiscard]] bool run_at(
    application::ApplicationRuntime& runtime,
    application::InputVector2 cursor)
{
    runtime.input_state().initialize_cursor(cursor);
    const auto frameResult = runtime.run_frame();
    if (!frameResult) {
        std::cerr << frameResult.error().message << '\n';
        return false;
    }

    return true;
}

} // namespace

int main()
{
    application::ApplicationRuntime runtime{};
    const auto initializeResult = runtime.initialize();
    if (!initializeResult) {
        std::cerr << initializeResult.error().message << '\n';
        return 1;
    }

    application::DocumentSession* document =
        runtime.documents().active_document();
    if (!document) {
        return fail(runtime, "active document unavailable");
    }

    editor::Editor& editor = document->editor();
    const editor::SceneNodeId cubeId =
        editor.scene().create_mesh("Picking test cube");
    editor::MeshNode* cube = editor.scene().find_mesh(cubeId);
    if (!cube) {
        return fail(runtime, "failed to create the test cube");
    }

    const auto cubeResult =
        kernel::geometry::TopologyBuilder::build_box_into(cube->mesh());
    if (!cubeResult) {
        return fail(runtime, "failed to build the test cube");
    }

    editor.mark_dirty(
        editor::EditorDirtyFlags::Scene |
        editor::EditorDirtyFlags::Mesh |
        editor::EditorDirtyFlags::Render |
        editor::EditorDirtyFlags::Picking);

    const std::int32_t logicalWidth = runtime.window().width();
    const std::int32_t logicalHeight = runtime.window().height();
    const std::int32_t framebufferWidth =
        runtime.window().framebuffer_width();
    const std::int32_t framebufferHeight =
        runtime.window().framebuffer_height();
    const application::InputVector2 center{
        static_cast<double>(logicalWidth) * 0.5,
        static_cast<double>(logicalHeight) * 0.5
    };

    if (!run_at(runtime, { -1.0, -1.0 })
        || runtime.editor_viewport().last_picking_result().status
            != application::ViewportPickingStatus::OutsideViewport) {
        return fail(runtime, "outside-viewport check failed");
    }
    std::cout << "[picking] cursor outside viewport -> invalid hit\n";

    if (!run_at(runtime, { 2.0, 2.0 })
        || runtime.editor_viewport().last_picking_result().status
            != application::ViewportPickingStatus::Background) {
        return fail(runtime, "background check failed");
    }
    std::cout << "[picking] cursor over background -> invalid hit\n";

    if (!run_at(runtime, center)
        || !runtime.editor_viewport().last_picking_result().has_hit()
        || runtime.editor_viewport().last_picking_result().sceneNodeId
            != cubeId) {
        return fail(runtime, "graphics ID to cube mapping check failed");
    }

    const application::ViewportPickingResult hit =
        runtime.editor_viewport().last_picking_result();
    std::cout
        << "[picking] cursor over cube -> valid hit; graphics ID "
        << hit.pickingId.value
        << " -> scene node "
        << hit.sceneNodeId.value
        << '\n';

    runtime.editor_viewport().orbit_camera(8.0, 0.0);
    if (!run_at(runtime, center)
        || !runtime.editor_viewport()
            .last_picking_result()
            .bufferRendered) {
        return fail(runtime, "camera invalidation check failed");
    }
    std::cout << "[picking] camera movement -> buffer recalculated\n";

    runtime.editor_viewport().resize(
        framebufferWidth - 1,
        framebufferHeight - 1);
    if (!run_at(runtime, center)
        || !runtime.editor_viewport()
            .last_picking_result()
            .bufferRendered) {
        return fail(runtime, "resize invalidation check failed");
    }
    std::cout << "[picking] resize -> buffer recalculated\n";

    const auto focusResult = runtime.editor_viewport().update_hover(
        *document,
        center,
        logicalWidth,
        logicalHeight,
        false,
        false);
    if (!focusResult
        || focusResult.value().status
            != application::ViewportPickingStatus::FocusLost
        || !editor.selection().objects().hovered().is_invalid()) {
        return fail(runtime, "focus-loss hover clearing check failed");
    }
    std::cout << "[picking] window focus lost -> hover cleared\n";

    runtime.editor_viewport().resize(0, 0);
    const auto minimizedResult =
        runtime.editor_viewport().update_hover(
            *document,
            center,
            logicalWidth,
            logicalHeight,
            true,
            false);
    if (!minimizedResult
        || minimizedResult.value().status
            != application::ViewportPickingStatus::EmptyFramebuffer) {
        return fail(runtime, "zero-sized framebuffer check failed");
    }
    std::cout
        << "[picking] zero-sized framebuffer -> picking suspended\n";

    runtime.shutdown();
    return 0;
}
