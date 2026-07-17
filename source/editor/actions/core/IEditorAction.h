/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/actions/core/ActionContext.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionResult.h"

namespace locus::editor {

    /**
     * @brief Interface implemented by immediate editor actions.
     *
     * Actions represent instantaneous operations triggered by menus, buttons,
     * shortcuts, command palettes, and context menus.
     *
     * Actions do not become the active editor tool, receive continuous pointer
     * events, or retain modal interaction state. Interactive and preview-based
     * workflows belong to the tools subsystem.
     */
    class IEditorAction {
    public:
        /**
         * @brief Destroys the action.
         */
        virtual ~IEditorAction() = default;

        IEditorAction() = default;
        IEditorAction(const IEditorAction&) = delete;
        IEditorAction& operator=(const IEditorAction&) = delete;
        IEditorAction(IEditorAction&&) = default;
        IEditorAction& operator=(IEditorAction&&) = default;

        /**
         * @brief Returns static metadata describing the action.
         *
         * @return Action descriptor.
         */
        [[nodiscard]] virtual const ActionDescriptor&
            descriptor() const = 0;

        /**
         * @brief Checks whether this action can execute in the current context.
         *
         * This operation must not mutate editor state.
         *
         * @param context Current action context.
         * @return True when the action is currently available.
         */
        [[nodiscard]] virtual bool can_execute(
            const ActionContext& context) const = 0;

        /**
         * @brief Executes the action immediately.
         *
         * Persistent and undoable changes should be performed through commands
         * dispatched by ActionContext rather than by mutating editor data
         * directly.
         *
         * Implementations must still validate the context during execution,
         * because editor state may change after a previous can_execute() call.
         *
         * @param context Current action context.
         * @return Action execution result.
         */
        virtual ActionResult execute(
            ActionContext& context) = 0;
    };

} // namespace locus::editor