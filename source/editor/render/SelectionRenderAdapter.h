/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionState.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <string>
#include <vector>

namespace locus::editor {

    /**
     * @brief Options used when applying editor selection to render objects.
     */
    struct SelectionRenderOptions {
        /**
         * @brief Clears existing selected/hovered flags before applying editor selection.
         */
        bool clearExistingFlags = true;

        /**
         * @brief Marks render objects selected when their id belongs to ObjectSelection.
         */
        bool applyObjectSelection = true;

        /**
         * @brief Marks the active object as selected even when it is not in the selected set.
         */
        bool applyActiveObject = true;

        /**
         * @brief Marks the hovered object as hovered.
         */
        bool applyHoveredObject = true;

        /**
         * @brief Marks the active mesh object selected when mesh component selection is active.
         */
        bool applyActiveMeshSelection = true;

        /**
         * @brief Marks the active mesh object hovered when a mesh component is hovered.
         */
        bool applyActiveMeshHover = true;

        /**
         * @brief Forces selected render objects into wireframe mode.
         */
        bool wireframeSelectedObjects = false;
    };

    /**
     * @brief Diagnostics for one render object processed by SelectionRenderAdapter.
     */
    struct SelectionRenderObjectResult {
        /**
         * @brief Render object identifier.
         */
        graphics::RenderObject::Id objectId = 0;

        /**
         * @brief True when the object was selected before applying selection.
         */
        bool wasSelected = false;

        /**
         * @brief True when the object was hovered before applying selection.
         */
        bool wasHovered = false;

        /**
         * @brief True when the object is selected after applying selection.
         */
        bool selected = false;

        /**
         * @brief True when the object is hovered after applying selection.
         */
        bool hovered = false;

        /**
         * @brief True when the object was changed by this adapter.
         */
        bool changed = false;

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message;
    };

    /**
     * @brief Diagnostics produced when applying editor selection to a render scene.
     */
    struct SelectionRenderResult {
        /**
         * @brief Number of render objects visited.
         */
        std::size_t visitedObjectCount = 0;

        /**
         * @brief Number of render objects marked selected.
         */
        std::size_t selectedObjectCount = 0;

        /**
         * @brief Number of render objects marked hovered.
         */
        std::size_t hoveredObjectCount = 0;

        /**
         * @brief Number of render objects changed by this adapter.
         */
        std::size_t changedObjectCount = 0;

        /**
         * @brief True when ObjectSelection active object was applied.
         */
        bool activeObjectApplied = false;

        /**
         * @brief True when ObjectSelection hovered object was applied.
         */
        bool hoveredObjectApplied = false;

        /**
         * @brief True when MeshSelection active mesh was applied.
         */
        bool activeMeshApplied = false;

        /**
         * @brief True when MeshSelection hovered component caused object hover.
         */
        bool activeMeshHoverApplied = false;

        /**
         * @brief Per-object diagnostics.
         */
        std::vector<SelectionRenderObjectResult> objects;
    };

    /**
     * @brief Applies editor selection state to graphics render scenes.
     *
     * SelectionRenderAdapter only maps editor selection semantics to render object
     * flags. It does not create overlay geometry for selected mesh components.
     */
    class SelectionRenderAdapter {
    public:
        /**
         * @brief Builds a copy of a render scene with selection flags applied.
         *
         * @param scene Source render scene.
         * @param selection Editor selection state.
         * @param options Selection conversion options.
         * @param result Optional diagnostic output.
         * @return Render scene copy with updated selection flags.
         */
        [[nodiscard]] static graphics::RenderScene apply_selection(
            const graphics::RenderScene& scene,
            const SelectionState& selection,
            const SelectionRenderOptions& options = {},
            SelectionRenderResult* result = nullptr
        );

        /**
         * @brief Applies selection flags to a single render object copy.
         *
         * @param object Source render object.
         * @param selection Editor selection state.
         * @param options Selection conversion options.
         * @param result Optional per-object diagnostic output.
         * @return Render object copy with updated selection flags.
         */
        [[nodiscard]] static graphics::RenderObject apply_selection_to_object(
            const graphics::RenderObject& object,
            const SelectionState& selection,
            const SelectionRenderOptions& options = {},
            SelectionRenderObjectResult* result = nullptr
        );

    private:
        /**
         * @brief Converts a render object id to an editor scene node id.
         *
         * @param objectId Render object identifier.
         * @return Scene node identifier.
         */
        [[nodiscard]] static SceneNodeId to_scene_node_id(graphics::RenderObject::Id objectId);

        /**
         * @brief Checks whether the object should be selected by object-level selection.
         *
         * @param id Scene node identifier.
         * @param selection Editor selection state.
         * @param options Selection conversion options.
         * @return True when selected.
         */
        [[nodiscard]] static bool is_object_selected(
            SceneNodeId id,
            const SelectionState& selection,
            const SelectionRenderOptions& options
        );

        /**
         * @brief Checks whether the object should be hovered by object-level selection.
         *
         * @param id Scene node identifier.
         * @param selection Editor selection state.
         * @param options Selection conversion options.
         * @return True when hovered.
         */
        [[nodiscard]] static bool is_object_hovered(
            SceneNodeId id,
            const SelectionState& selection,
            const SelectionRenderOptions& options
        );

        /**
         * @brief Checks whether active mesh component selection should mark this object selected.
         *
         * @param id Scene node identifier.
         * @param selection Editor selection state.
         * @param options Selection conversion options.
         * @return True when active mesh selection targets this object.
         */
        [[nodiscard]] static bool is_active_mesh_selected(
            SceneNodeId id,
            const SelectionState& selection,
            const SelectionRenderOptions& options
        );

        /**
         * @brief Checks whether hovered mesh component selection should mark this object hovered.
         *
         * @param id Scene node identifier.
         * @param selection Editor selection state.
         * @param options Selection conversion options.
         * @return True when active mesh hover targets this object.
         */
        [[nodiscard]] static bool is_active_mesh_hovered(
            SceneNodeId id,
            const SelectionState& selection,
            const SelectionRenderOptions& options
        );

        /**
         * @brief Checks whether any mesh component is currently hovered.
         *
         * @param selection Editor selection state.
         * @return True when any mesh component hover handle is valid.
         */
        [[nodiscard]] static bool has_hovered_mesh_component(const SelectionState& selection);

        /**
         * @brief Adds one object result to an optional aggregate result.
         *
         * @param result Optional aggregate result.
         * @param objectResult Object diagnostic to append.
         */
        static void push_object_result(
            SelectionRenderResult* result,
            SelectionRenderObjectResult objectResult
        );
    };

} // namespace locus::editor