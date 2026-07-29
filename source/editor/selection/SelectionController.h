/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/queries/SelectionHit.h"

namespace locus::editor {

    /**
     * @brief High-level API used to mutate editor selection consistently.
     */
    class SelectionController {
    public:
        /**
         * @brief Creates a selection controller.
         *
         * @param scene Scene used for validating object selections.
         * @param state Selection state to mutate.
         */
        SelectionController(EditorScene& scene, SelectionState& state);

        /**
         * @brief Selects one object and clears previous object selection.
         *
         * @param id Object identifier.
         * @return True when the object was selected.
         */
        bool select_object(SceneNodeId id);

        /**
         * @brief Adds an object to the current selection.
         *
         * @param id Object identifier.
         * @return True when the object was added.
         */
        bool add_object(SceneNodeId id);

        /**
         * @brief Removes an object from the current selection.
         *
         * @param id Object identifier.
         * @return True when the object was removed.
         */
        bool remove_object(SceneNodeId id);

        /**
         * @brief Toggles an object in the current selection.
         *
         * @param id Object identifier.
         * @return True when the object is selected after the operation.
         */
        bool toggle_object(SceneNodeId id);

        /**
         * @brief Changes the active object.
         *
         * @param id Object identifier.
         * @return True when the active object was changed.
         */
        bool set_active_object(SceneNodeId id);

        /**
         * @brief Sets the hovered object.
         *
         * @param id Object identifier.
         * @return True when the hover state was changed.
         */
        bool set_hovered_object(SceneNodeId id);

        /**
         * @brief Clears object selection.
         */
        void clear_objects();

        /**
         * @brief Sets the active mesh for component selection.
         *
         * @param id Mesh scene node identifier.
         * @return True when the active mesh was changed.
         */
        bool set_active_mesh(SceneNodeId id);

        /**
         * @brief Selects one mesh vertex.
         *
         * @param handle Vertex handle.
         * @return True when selected.
         */
        bool select_vertex(kernel::geometry::VertexHandle handle);

        /**
         * @brief Selects one mesh edge.
         *
         * @param handle Edge handle.
         * @return True when selected.
         */
        bool select_edge(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Selects one mesh loop.
         *
         * @param handle Loop handle.
         * @return True when selected.
         */
        bool select_loop(kernel::geometry::LoopHandle handle);

        /**
         * @brief Selects one mesh face.
         *
         * @param handle Face handle.
         * @return True when selected.
         */
        bool select_face(kernel::geometry::FaceHandle handle);

        /**
         * @brief Toggles a mesh vertex.
         *
         * @param handle Vertex handle.
         * @return True when selected after the operation.
         */
        bool toggle_vertex(kernel::geometry::VertexHandle handle);

        /**
         * @brief Toggles a mesh edge.
         *
         * @param handle Edge handle.
         * @return True when selected after the operation.
         */
        bool toggle_edge(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Toggles a mesh loop.
         *
         * @param handle Loop handle.
         * @return True when selected after the operation.
         */
        bool toggle_loop(kernel::geometry::LoopHandle handle);

        /**
         * @brief Toggles a mesh face.
         *
         * @param handle Face handle.
         * @return True when selected after the operation.
         */
        bool toggle_face(kernel::geometry::FaceHandle handle);

        /**
         * @brief Sets the hovered mesh component according to the active granularity.
         *
         * @param hit Component hit resolved from editor picking context.
         * @return True when hover state changed.
         */
        bool set_hovered_mesh_component(const kernel::geometry::SelectionHit& hit);

        /**
         * @brief Clears all hovered mesh components.
         *
         * @return True when hover state changed.
         */
        bool clear_hovered_mesh_component();

        /**
         * @brief Clears all mesh component selections.
         */
        void clear_mesh_components();

        /**
         * @brief Changes the current selection granularity.
         *
         * @param granularity New granularity.
         */
        void set_granularity(SelectionGranularity granularity);

    private:
        [[nodiscard]] bool is_valid_selectable_object(SceneNodeId id) const;
        [[nodiscard]] bool is_valid_mesh(SceneNodeId id) const;
        void enter_object_mode();
        void enter_mesh_mode(SelectionGranularity granularity);

        EditorScene* scene_ = nullptr;
        SelectionState* state_ = nullptr;
    };

}
