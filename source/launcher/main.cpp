/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"
#include "editor/EditorTypes.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/topology/TopologyBuilder.h"

#include <iostream>

namespace {

    [[nodiscard]] bool seed_demo_scene(
        locus::application::ApplicationRuntime& runtime)
    {
        locus::application::DocumentSession* document =
            runtime.documents().active_document();

        if (document == nullptr) {
            std::cerr << "No active document available.\n";
            return false;
        }

        locus::editor::Editor& editor = document->editor();
        const locus::editor::SceneNodeId cubeId =
            editor.scene().create_mesh("Demo cube");

        locus::editor::MeshNode* cube =
            editor.scene().find_mesh(cubeId);

        if (cube == nullptr) {
            std::cerr << "Failed to create demo cube.\n";
            return false;
        }

        const auto cubeResult =
            locus::kernel::geometry::TopologyBuilder::build_box_into(
                cube->mesh());

        if (!cubeResult) {
            std::cerr << "Failed to build demo cube.\n";
            return false;
        }

        editor.mark_dirty(
            locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking);

        std::cout
            << "Demo cube created with scene node "
            << cubeId.value
            << ". Click it to select, then use W/E/R.\n";

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

    if (!seed_demo_scene(runtime)) {
        runtime.shutdown();
        return 1;
    }

    const locus::application::ApplicationResult<int> runResult =
        runtime.run();

    if (!runResult) {
        std::cerr << runResult.error().message << '\n';
        return 1;
    }

    return runResult.value();
}
