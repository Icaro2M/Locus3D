/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/overlay/renderers/PivotRenderer.h"

namespace locus::editor {

    class EditorScene;
    class PivotTool;
    class SelectionState;

    /**
     * @brief Converts editor object pivot state into graphics-only draw data.
     */
    class PivotRenderAdapter {
    public:
        /**
         * @brief Builds draw data for the active object pivot.
         *
         * @param scene Editor scene containing selected nodes.
         * @param selection Current editor selection state.
         * @param activePivotTool Optional active pivot tool state.
         * @return Graphics-only pivot marker draw data.
         */
        [[nodiscard]] static graphics::PivotDrawData build_active_object_pivot(
            const EditorScene& scene,
            const SelectionState& selection,
            const PivotTool* activePivotTool = nullptr);
    };

} // namespace locus::editor
