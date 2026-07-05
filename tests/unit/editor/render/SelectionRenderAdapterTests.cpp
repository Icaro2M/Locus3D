/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorRenderTestSuite.h"

#include "editor/render/SelectionRenderAdapter.h"
#include "kernel/geometry/mesh/LEMHandles.h"

namespace {

[[nodiscard]] locus::graphics::RenderObject make_object(locus::graphics::RenderObject::Id id)
{
    locus::graphics::RenderObject object;
    object.id = id;
    object.name = "Object";
    return object;
}

} // namespace

namespace locus::tests {

TestResult run_selection_render_adapter_tests()
{
    editor::SelectionState selection;
    const editor::SceneNodeId objectId{ 10 };
    const editor::SceneNodeId meshId{ 20 };

    selection.objects().set(objectId);
    selection.objects().set_hovered(meshId);
    selection.mesh().set_active_mesh(meshId);
    selection.mesh().add_vertex(kernel::geometry::VertexHandle{ 0 });
    selection.mesh().set_hovered_edge(kernel::geometry::EdgeHandle{ 1 });

    editor::SelectionRenderOptions options;
    options.wireframeSelectedObjects = true;

    editor::SelectionRenderObjectResult objectResult;
    const graphics::RenderObject selectedObject =
        editor::SelectionRenderAdapter::apply_selection_to_object(
            make_object(objectId.value),
            selection,
            options,
            &objectResult);

    if (!selectedObject.selected ||
        selectedObject.hovered ||
        !selectedObject.wireframe ||
        objectResult.objectId != objectId.value ||
        objectResult.wasSelected ||
        objectResult.wasHovered ||
        !objectResult.selected ||
        objectResult.hovered ||
        !objectResult.changed ||
        objectResult.message != "Render object selected.") {
        return TestResult::fail("SelectionRenderAdapter should apply object selection to render objects");
    }

    const graphics::RenderObject meshObject =
        editor::SelectionRenderAdapter::apply_selection_to_object(
            make_object(meshId.value),
            selection,
            options);

    if (!meshObject.selected ||
        !meshObject.hovered ||
        !meshObject.wireframe) {
        return TestResult::fail("SelectionRenderAdapter should apply active mesh selection and hover");
    }

    graphics::RenderObject preselected = make_object(30);
    preselected.selected = true;
    preselected.hovered = true;

    editor::SelectionRenderOptions preserveOptions;
    preserveOptions.clearExistingFlags = false;
    const graphics::RenderObject preserved =
        editor::SelectionRenderAdapter::apply_selection_to_object(
            preselected,
            editor::SelectionState{},
            preserveOptions);

    if (!preserved.selected ||
        !preserved.hovered) {
        return TestResult::fail("SelectionRenderAdapter should optionally preserve existing flags");
    }

    graphics::RenderScene scene;
    scene.add_object(make_object(objectId.value));
    scene.add_object(make_object(meshId.value));
    scene.add_object(make_object(30));

    editor::SelectionRenderResult sceneResult;
    const graphics::RenderScene selectedScene =
        editor::SelectionRenderAdapter::apply_selection(scene, selection, options, &sceneResult);

    if (selectedScene.object_count() != 3u ||
        !selectedScene.objects()[0].selected ||
        selectedScene.objects()[0].hovered ||
        !selectedScene.objects()[1].selected ||
        !selectedScene.objects()[1].hovered ||
        selectedScene.objects()[2].selected ||
        selectedScene.objects()[2].hovered ||
        sceneResult.visitedObjectCount != 3u ||
        sceneResult.selectedObjectCount != 2u ||
        sceneResult.hoveredObjectCount != 1u ||
        sceneResult.changedObjectCount != 2u ||
        !sceneResult.activeObjectApplied ||
        !sceneResult.hoveredObjectApplied ||
        !sceneResult.activeMeshApplied ||
        !sceneResult.activeMeshHoverApplied ||
        sceneResult.objects.size() != 3u) {
        return TestResult::fail("SelectionRenderAdapter should apply selection to render scene copies");
    }

    editor::SelectionRenderOptions disabledOptions;
    disabledOptions.applyObjectSelection = false;
    disabledOptions.applyActiveObject = false;
    disabledOptions.applyHoveredObject = false;
    disabledOptions.applyActiveMeshSelection = false;
    disabledOptions.applyActiveMeshHover = false;

    const graphics::RenderObject disabledObject =
        editor::SelectionRenderAdapter::apply_selection_to_object(
            make_object(objectId.value),
            selection,
            disabledOptions);

    if (disabledObject.selected ||
        disabledObject.hovered) {
        return TestResult::fail("SelectionRenderAdapter options should disable selection sources");
    }

    return TestResult::pass();
}

} // namespace locus::tests
