/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Reason why an active tool interaction was cancelled.
     */
    enum class ToolCancelReason {
        /**
         * @brief Cancellation was explicitly requested by the user.
         */
        UserRequest,

        /**
         * @brief The application or active viewport lost focus.
         */
        FocusLost,

        /**
         * @brief Another editor tool is being activated.
         */
        ToolSwitch,

        /**
         * @brief The active tool is being deactivated.
         */
        ToolDeactivated,

        /**
         * @brief The interaction target or editor state became invalid.
         */
        InvalidState
    };

} // namespace locus::editor