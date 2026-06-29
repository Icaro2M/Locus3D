/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/EditorTypes.h"
#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionState.h"

namespace locus::editor {

    /**
     * @brief Aggregates persistent mutable state owned by the editor layer.
     */
    struct EditorState {
        /**
         * @brief Editable scene hierarchy and object data.
         */
        EditorScene scene;

        /**
         * @brief Object and mesh component selection state.
         */
        SelectionState selection;

        /**
         * @brief Current high-level editor interaction mode.
         */
        EditorMode mode = EditorMode::Object;

        /**
         * @brief Dirty subsystems waiting for synchronization.
         */
        EditorDirtyFlags dirtyFlags = EditorDirtyFlags::All;
    };

}