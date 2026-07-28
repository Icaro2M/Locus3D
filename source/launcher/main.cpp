/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/ApplicationResult.h"
#include "application/document/DocumentSession.h"
#include "application/runtime/ApplicationRuntime.h"
#include "editor/EditorTypes.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/topology/TopologyBuilder.h"

#include <cstddef>
#include <iostream>

namespace {

    [[nodiscard]] const char* picking_status_name(
        locus::application::ViewportPickingStatus status) noexcept
    {
        switch (status) {
        case locus::application::ViewportPickingStatus::Unavailable:
            return "unavailable";
        case locus::application::ViewportPickingStatus::OutsideViewport:
            return "outside";
        case locus::application::ViewportPickingStatus::Background:
            return "background";
        case locus::application::ViewportPickingStatus::Hit:
            return "hit";
        case locus::application::ViewportPickingStatus::FocusLost:
            return "focus-lost";
        case locus::application::ViewportPickingStatus::CameraCapture:
            return "camera-capture";
        case locus::application::ViewportPickingStatus::EmptyFramebuffer:
            return "empty-framebuffer";
        }

        return "unknown";
    }

    void print_runtime_selection_state(
        const locus::application::ApplicationRuntime& runtime)
    {
        const locus::application::DocumentSession* document =
            runtime.documents().active_document();

        if (!document) {
            return;
        }

        const locus::application::ViewportPickingResult& picking =
            runtime.editor_viewport().last_picking_result();
        const locus::editor::ObjectSelection& objects =
            document->editor().selection().objects();

        std::cout
            << "[selection] picking="
            << picking_status_name(picking.status)
            << " hitNode="
            << picking.sceneNodeId.value
            << " hovered="
            << objects.hovered().value
            << " active="
            << objects.active().value
            << " selectedCount="
            << objects.size()
            << '\n';
    }

    [[nodiscard]] bool selection_state_changed(
        const locus::application::ApplicationRuntime& runtime)
    {
        struct Snapshot {
            locus::application::ViewportPickingStatus pickingStatus =
                locus::application::ViewportPickingStatus::Unavailable;
            locus::editor::SceneNodeId hit{};
            locus::editor::SceneNodeId hovered{};
            locus::editor::SceneNodeId active{};
            std::size_t selectedCount = 0;
        };

        static Snapshot previous{};
        static bool initialized = false;

        const locus::application::DocumentSession* document =
            runtime.documents().active_document();

        if (!document) {
            return false;
        }

        const locus::application::ViewportPickingResult& picking =
            runtime.editor_viewport().last_picking_result();
        const locus::editor::ObjectSelection& objects =
            document->editor().selection().objects();

        const Snapshot current{
            picking.status,
            picking.sceneNodeId,
            objects.hovered(),
            objects.active(),
            objects.size()
        };

        const bool changed =
            !initialized
            || previous.pickingStatus != current.pickingStatus
            || previous.hit != current.hit
            || previous.hovered != current.hovered
            || previous.active != current.active
            || previous.selectedCount != current.selectedCount;

        previous = current;
        initialized = true;
        return changed;
    }

    [[nodiscard]] bool seed_selection_test_scene(
        locus::application::ApplicationRuntime& runtime)
    {
        locus::application::DocumentSession* document =
            runtime.documents().active_document();

        if (!document) {
            std::cerr << "No active document available.\n";
            return false;
        }

        locus::editor::Editor& editor = document->editor();
        const locus::editor::SceneNodeId cubeId =
            editor.scene().create_mesh("Selection test cube");

        locus::editor::MeshNode* cube =
            editor.scene().find_mesh(cubeId);

        if (!cube) {
            std::cerr << "Failed to create selection test cube.\n";
            return false;
        }

        const auto cubeResult =
            locus::kernel::geometry::TopologyBuilder::build_box_into(
                cube->mesh());

        if (!cubeResult) {
            std::cerr << "Failed to build selection test cube.\n";
            return false;
        }

        editor.mark_dirty(
            locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking);

        std::cout
            << "[selection] test cube created with scene node "
            << cubeId.value
            << '\n';

        return true;
    }

} // namespace

int main()
{
    locus::application::ApplicationRuntime runtime{};

    const locus::application::ApplicationResult<void> initializeResult =
        runtime.initialize();

    if (!initializeResult) {
        std::cerr << initializeResult.error().message << '\n';
        return 1;
    }

    if (!seed_selection_test_scene(runtime)) {
        runtime.shutdown();
        return 1;
    }

    while (!runtime.state().quitRequested
        && !runtime.window().should_close()) {
        const locus::application::ApplicationResult<
            locus::application::FrameContext> frameResult =
            runtime.run_frame();

        if (!frameResult) {
            std::cerr << frameResult.error().message << '\n';
            runtime.shutdown();
            return 1;
        }

        if (selection_state_changed(runtime)) {
            print_runtime_selection_state(runtime);
        }
    }

    const int exitCode = runtime.state().exitCode;
    runtime.shutdown();
    return exitCode;
}
