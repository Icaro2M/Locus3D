/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/ActionExecutor.h"

#include <utility>

namespace locus::editor {

    ActionExecutor::ActionExecutor(
        ActionRegistry& registry)
        : registry_(&registry) {
    }

    bool ActionExecutor::can_execute(
        const ActionContext& context,
        const ActionId& id) const {
        if (id.is_invalid()) {
            return false;
        }

        const IEditorAction* action =
            registry_->find(id);

        return action
            && action->can_execute(context);
    }

    ActionResult ActionExecutor::execute(
        ActionContext& context,
        const ActionId& id) const {
        if (id.is_invalid()) {
            return ActionResult::fail(
                "Cannot execute an action with an invalid identifier.");
        }

        IEditorAction* action =
            registry_->find(id);

        if (!action) {
            return ActionResult::fail(
                "The requested action is not registered.");
        }

        if (!action->can_execute(context)) {
            return ActionResult::unavailable(
                "The requested action is not available in the current "
                "editor state.");
        }

        return apply_result(
            context,
            action->execute(context));
    }

    ActionRegistry& ActionExecutor::registry() {
        return *registry_;
    }

    const ActionRegistry&
        ActionExecutor::registry() const {
        return *registry_;
    }

    ActionResult ActionExecutor::apply_result(
        ActionContext& context,
        ActionResult result) const {
        if (result.dirtyFlags != EditorDirtyFlags::None) {
            context.mark_dirty(result.dirtyFlags);
        }

        return result;
    }

} // namespace locus::editor