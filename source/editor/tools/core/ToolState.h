/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Runtime lifecycle state of an editor tool.
     */
    enum class ToolState {
        /**
         * @brief Tool is not currently active.
         */
        Inactive,

        /**
         * @brief Tool is active and waiting for user interaction.
         */
        Ready,

        /**
         * @brief Tool owns an active interaction or temporary session.
         */
        Interacting,

        /**
         * @brief Tool is temporarily suspended without losing its session.
         */
        Suspended
    };

} // namespace locus::editor