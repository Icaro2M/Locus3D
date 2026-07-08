/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Active transform gizmo mode.
     */
    enum class GizmoMode {
        /**
         * @brief No transform gizmo is active.
         */
        None,

        /**
         * @brief Translation gizmo.
         */
        Translate,

        /**
         * @brief Rotation gizmo.
         */
        Rotate,

        /**
         * @brief Scale gizmo.
         */
        Scale,

        /**
         * @brief Combined translate, rotate, and scale gizmo.
         */
        Universal
    };

} // namespace locus::editor