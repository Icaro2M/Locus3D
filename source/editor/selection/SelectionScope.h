/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Scope currently targeted by selection operations.
     */
    enum class SelectionScope {
        /**
         * @brief Selection targets the editor scene hierarchy.
         */
        Scene,

        /**
         * @brief Selection targets components inside the active mesh object.
         */
        ActiveMesh
    };

}