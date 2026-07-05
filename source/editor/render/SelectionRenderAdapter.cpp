/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/SelectionRenderAdapter.h"

#include <utility>

namespace locus::editor {

    graphics::RenderScene SelectionRenderAdapter::apply_selection(
        const graphics::RenderScene& scene,
        const SelectionState& selection,
        const SelectionRenderOptions& options,
        SelectionRenderResult* result
    ) {
        if (result) {
            *result = {};
        }

        graphics::RenderScene output;
        output.reserve(scene.object_count());

        for (const graphics::RenderObject& object : scene.objects()) {
            if (result) {
                ++result->visitedObjectCount;
            }

            SelectionRenderObjectResult objectResult{};
            graphics::RenderObject selectedObject =
                apply_selection_to_object(object, selection, options, &objectResult);

            if (objectResult.selected && result) {
                ++result->selectedObjectCount;
            }

            if (objectResult.hovered && result) {
                ++result->hoveredObjectCount;
            }

            if (objectResult.changed && result) {
                ++result->changedObjectCount;
            }

            const SceneNodeId id = to_scene_node_id(object.id);

            if (result && options.applyActiveObject && selection.objects().active() == id) {
                result->activeObjectApplied = true;
            }

            if (result && options.applyHoveredObject && selection.objects().hovered() == id) {
                result->hoveredObjectApplied = true;
            }

            if (result && is_active_mesh_selected(id, selection, options)) {
                result->activeMeshApplied = true;
            }

            if (result && is_active_mesh_hovered(id, selection, options)) {
                result->activeMeshHoverApplied = true;
            }

            push_object_result(result, std::move(objectResult));
            output.add_object(std::move(selectedObject));
        }

        return output;
    }

    graphics::RenderObject SelectionRenderAdapter::apply_selection_to_object(
        const graphics::RenderObject& object,
        const SelectionState& selection,
        const SelectionRenderOptions& options,
        SelectionRenderObjectResult* result
    ) {
        graphics::RenderObject output = object;

        const bool previousSelected = output.selected;
        const bool previousHovered = output.hovered;

        if (options.clearExistingFlags) {
            output.selected = false;
            output.hovered = false;
        }

        const SceneNodeId id = to_scene_node_id(object.id);

        const bool selectedByObject = is_object_selected(id, selection, options);
        const bool selectedByMesh = is_active_mesh_selected(id, selection, options);
        const bool hoveredByObject = is_object_hovered(id, selection, options);
        const bool hoveredByMesh = is_active_mesh_hovered(id, selection, options);

        output.selected = output.selected || selectedByObject || selectedByMesh;
        output.hovered = output.hovered || hoveredByObject || hoveredByMesh;

        if (options.wireframeSelectedObjects && output.selected) {
            output.wireframe = true;
        }

        if (result) {
            result->objectId = object.id;
            result->wasSelected = previousSelected;
            result->wasHovered = previousHovered;
            result->selected = output.selected;
            result->hovered = output.hovered;
            result->changed =
                previousSelected != output.selected ||
                previousHovered != output.hovered ||
                object.wireframe != output.wireframe;

            if (output.selected && output.hovered) {
                result->message = "Render object selected and hovered.";
            }
            else if (output.selected) {
                result->message = "Render object selected.";
            }
            else if (output.hovered) {
                result->message = "Render object hovered.";
            }
            else {
                result->message = "Render object not targeted by selection.";
            }
        }

        return output;
    }

    SceneNodeId SelectionRenderAdapter::to_scene_node_id(graphics::RenderObject::Id objectId)
    {
        return SceneNodeId{ static_cast<SceneNodeIdValue>(objectId) };
    }

    bool SelectionRenderAdapter::is_object_selected(
        SceneNodeId id,
        const SelectionState& selection,
        const SelectionRenderOptions& options
    ) {
        if (!id.is_valid()) {
            return false;
        }

        if (options.applyObjectSelection && selection.objects().contains(id)) {
            return true;
        }

        if (options.applyActiveObject && selection.objects().active() == id) {
            return true;
        }

        return false;
    }

    bool SelectionRenderAdapter::is_object_hovered(
        SceneNodeId id,
        const SelectionState& selection,
        const SelectionRenderOptions& options
    ) {
        if (!id.is_valid() || !options.applyHoveredObject) {
            return false;
        }

        return selection.objects().hovered() == id;
    }

    bool SelectionRenderAdapter::is_active_mesh_selected(
        SceneNodeId id,
        const SelectionState& selection,
        const SelectionRenderOptions& options
    ) {
        if (!id.is_valid() || !options.applyActiveMeshSelection) {
            return false;
        }

        if (selection.mesh().active_mesh() != id) {
            return false;
        }

        return !selection.mesh().empty();
    }

    bool SelectionRenderAdapter::is_active_mesh_hovered(
        SceneNodeId id,
        const SelectionState& selection,
        const SelectionRenderOptions& options
    ) {
        if (!id.is_valid() || !options.applyActiveMeshHover) {
            return false;
        }

        if (selection.mesh().active_mesh() != id) {
            return false;
        }

        return has_hovered_mesh_component(selection);
    }

    bool SelectionRenderAdapter::has_hovered_mesh_component(const SelectionState& selection)
    {
        return selection.mesh().hovered_vertex().is_valid() ||
            selection.mesh().hovered_edge().is_valid() ||
            selection.mesh().hovered_loop().is_valid() ||
            selection.mesh().hovered_face().is_valid();
    }

    void SelectionRenderAdapter::push_object_result(
        SelectionRenderResult* result,
        SelectionRenderObjectResult objectResult
    ) {
        if (!result) {
            return;
        }

        result->objects.push_back(std::move(objectResult));
    }

} // namespace locus::editor