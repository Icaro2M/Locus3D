/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief High-level category used to organize editor actions.
     */
    enum class ActionCategory {
        /**
         * @brief Scene hierarchy and scene object actions.
         */
        Scene,

        /**
         * @brief Object and mesh component selection actions.
         */
        Selection,

        /**
         * @brief Object and mesh component transformation actions.
         */
        Transform,

        /**
         * @brief Mesh geometry, topology, and attribute actions.
         */
        Mesh,

        /**
         * @brief Document lifecycle, import, and export actions.
         */
        Document,

        /**
         * @brief Inspection, diagnostics, and miscellaneous editor actions.
         */
        Utility
    };

} // namespace locus::editor