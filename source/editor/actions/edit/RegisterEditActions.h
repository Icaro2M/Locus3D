/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Stable identifiers of built-in edit actions.
     */
    namespace edit_actions {

        /**
         * @brief Contextual Delete intent.
         */
        inline constexpr std::string_view DeleteId =
            "edit.delete";

    } // namespace edit_actions

    /**
     * @brief Registers built-in edit actions.
     *
     * @param registry Registry that owns created actions.
     * @return True when every edit action was registered.
     */
    bool register_edit_actions(ActionRegistry& registry);

} // namespace locus::editor
