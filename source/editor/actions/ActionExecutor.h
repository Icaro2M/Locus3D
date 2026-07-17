/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/core/ActionResult.h"

namespace locus::editor {

    /**
     * @brief Resolves and executes immediate editor actions.
     *
     * ActionExecutor does not own the registry or action instances. It
     * centralizes identifier validation, action lookup, availability checks,
     * execution, and dirty flag propagation.
     *
     * Menus, buttons, keyboard shortcuts, command palettes, and context menus
     * may all use this executor without duplicating action dispatch logic.
     */
    class ActionExecutor {
    public:
        /**
         * @brief Creates an executor using an action registry.
         *
         * @param registry Registry used to resolve action identifiers.
         */
        explicit ActionExecutor(
            ActionRegistry& registry);

        /**
         * @brief Checks whether an action can currently execute.
         *
         * @param context Current action context.
         * @param id Stable action identifier.
         * @return True when the action exists and accepts the current context.
         */
        [[nodiscard]] bool can_execute(
            const ActionContext& context,
            const ActionId& id) const;

        /**
         * @brief Executes a registered action immediately.
         *
         * The action is looked up by identifier and its availability is
         * validated immediately before execution. Dirty flags returned by the
         * action are propagated to the editor.
         *
         * @param context Current action context.
         * @param id Stable action identifier.
         * @return Action execution result.
         */
        ActionResult execute(
            ActionContext& context,
            const ActionId& id) const;

        /**
         * @brief Returns the action registry used by this executor.
         *
         * @return Mutable registry reference.
         */
        [[nodiscard]] ActionRegistry& registry();

        /**
         * @brief Returns the action registry used by this executor.
         *
         * @return Read-only registry reference.
         */
        [[nodiscard]] const ActionRegistry& registry() const;

    private:
        /**
         * @brief Applies common post-execution result handling.
         *
         * @param context Current action context.
         * @param result Result to process.
         * @return Processed result.
         */
        [[nodiscard]] ActionResult apply_result(
            ActionContext& context,
            ActionResult result) const;

        ActionRegistry* registry_ = nullptr;
    };

} // namespace locus::editor