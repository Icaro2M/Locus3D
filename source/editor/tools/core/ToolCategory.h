/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief High-level category used to organize editor tools.
     */
    enum class ToolCategory {
        /**
         * @brief Selection and component targeting tools.
         */
        Selection,

        /**
         * @brief Object and mesh component transformation tools.
         */
        Transform,

        /**
         * @brief Interactive mesh modeling tools.
         */
        Mesh,

        /**
         * @brief Scene object and primitive creation tools.
         */
        Creation,

        /**
         * @brief Measurement, inspection, and other utility tools.
         */
        Utility
    };

} // namespace locus::editor