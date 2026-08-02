/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorRenderTestSuite.h"

#include "editor/render/SelectionRenderAdapter.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <vector>

namespace {

[[nodiscard]] locus::graphics::RenderObject make_object(locus::graphics::RenderObject::Id id)
{
    locus::graphics::RenderObject object;
    object.id = id;
    object.name = "Object";
    return object;
}

[[nodiscard]] locus::graphics::RenderObject make_highlightable_object(
    locus::graphics::RenderObject::Id id,
    const locus::graphics::GpuMesh& mesh)
{
    locus::graphics::RenderObject object = make_object(id);
    object.mesh = &mesh;
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

    graphics::GpuMesh mesh;
    graphics::RenderScene highlightScene;
    highlightScene.add_object(make_highlightable_object(10, mesh));
    highlightScene.add_object(make_highlightable_object(20, mesh));
    highlightScene.add_object(make_highlightable_object(30, mesh));

    editor::SelectionState noObjectSelection;
    graphics::ObjectHighlightBatch highlights =
        editor::SelectionRenderAdapter::build_object_highlights(
            highlightScene,
            noObjectSelection);

    if (!highlights.empty()) {
        return TestResult::fail("object highlight submission should be empty without object selection");
    }

    editor::SelectionState hoveredSelection;
    hoveredSelection.objects().set_hovered(editor::SceneNodeId{ 20 });
    highlights = editor::SelectionRenderAdapter::build_object_highlights(
        highlightScene,
        hoveredSelection);

    if (highlights.size() != 1u ||
        highlights.highlights[0].object->id != 20u ||
        highlights.highlights[0].maskId != 1u ||
        highlights.highlights[0].category != graphics::ObjectHighlightCategory::Hovered) {
        return TestResult::fail("object highlight submission should classify hovered objects");
    }

    editor::SelectionState selectedSelection;
    selectedSelection.objects().set(
        std::vector<editor::SceneNodeId>{
            editor::SceneNodeId{ 10 },
            editor::SceneNodeId{ 30 }
        });
    highlights = editor::SelectionRenderAdapter::build_object_highlights(
        highlightScene,
        selectedSelection);

    if (highlights.size() != 2u ||
        highlights.highlights[0].object->id != 10u ||
        highlights.highlights[0].maskId == highlights.highlights[1].maskId ||
        highlights.highlights[0].category != graphics::ObjectHighlightCategory::Selected ||
        highlights.highlights[1].category != graphics::ObjectHighlightCategory::Selected) {
        return TestResult::fail("object highlight submission should preserve multi-selection with distinct mask ids");
    }

    selectedSelection.objects().set_hovered(editor::SceneNodeId{ 10 });
    highlights = editor::SelectionRenderAdapter::build_object_highlights(
        highlightScene,
        selectedSelection);

    if (highlights.empty() ||
        highlights.highlights[0].category != graphics::ObjectHighlightCategory::Selected) {
        return TestResult::fail("selected object highlight should take precedence over hover");
    }

    editor::SelectionState staleSelection;
    staleSelection.objects().set(editor::SceneNodeId{ 999 });
    highlights = editor::SelectionRenderAdapter::build_object_highlights(
        highlightScene,
        staleSelection);

    if (!highlights.empty()) {
        return TestResult::fail("object highlight submission should not keep stale deleted object ids");
    }

    graphics::RenderScene hiddenScene;
    graphics::RenderObject hidden = make_highlightable_object(10, mesh);
    hidden.visibility.visible = false;
    hiddenScene.add_object(hidden);

    editor::SelectionState hiddenSelection;
    hiddenSelection.objects().set(editor::SceneNodeId{ 10 });
    highlights = editor::SelectionRenderAdapter::build_object_highlights(
        hiddenScene,
        hiddenSelection);

    if (!highlights.empty()) {
        return TestResult::fail("object highlight submission should skip invisible objects");
    }

    graphics::RenderScene unselectableScene;
    graphics::RenderObject unselectable = make_highlightable_object(10, mesh);
    unselectable.visibility.selectable = false;
    unselectableScene.add_object(unselectable);

    highlights = editor::SelectionRenderAdapter::build_object_highlights(
        unselectableScene,
        hiddenSelection);

    if (!highlights.empty()) {
        return TestResult::fail("object highlight submission should skip unselectable objects");
    }

    editor::SelectionState faceSelection;
    faceSelection.objects().set(editor::SceneNodeId{ 10 });
    faceSelection.set_scope(editor::SelectionScope::ActiveMesh);
    faceSelection.set_granularity(editor::SelectionGranularity::Face);

    highlights = editor::SelectionRenderAdapter::build_object_highlights(
        highlightScene,
        faceSelection);

    if (!highlights.empty()) {
        return TestResult::fail("object highlight submission should clear when leaving object selection granularity");
    }

    return TestResult::pass();
}

} // namespace locus::tests
