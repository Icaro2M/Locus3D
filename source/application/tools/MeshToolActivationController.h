/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/ApplicationResult.h"
#include "application/shortcut/Shortcut.h"

namespace locus::application {

    class DocumentSession;

    /**
     * @brief Activates built-in mesh tools from application shortcuts.
     *
     * This component keeps concrete mesh tool identifiers and UX preflight
     * checks out of ApplicationRuntime while leaving final validation inside
     * each tool.
     */
    class MeshToolActivationController final {
    public:
        /**
         * @brief Attempts to handle a shortcut as a mesh tool activation.
         *
         * @param action Shortcut action to inspect.
         * @param document Active document.
         * @return True when the action was a mesh tool action, false when the
         * runtime should continue generic dispatch, or an application error.
         */
        [[nodiscard]]
        ApplicationResult<bool> activate_shortcut(
            ShortcutAction action,
            DocumentSession& document) const;

        /**
         * @brief Checks whether the active tool should emit throttled preview logs.
         *
         * @param document Active document.
         * @return True for mesh tools with preview updates logged by runtime.
         */
        [[nodiscard]]
        bool is_logged_preview_tool(
            const DocumentSession& document) const;
    };

} // namespace locus::application
