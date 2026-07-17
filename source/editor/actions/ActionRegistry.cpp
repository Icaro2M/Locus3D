/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/ActionRegistry.h"

#include <utility>

namespace locus::editor {

    bool ActionRegistry::register_action(
        std::unique_ptr<IEditorAction> action) {
        if (!action) {
            return false;
        }

        const ActionDescriptor& descriptor =
            action->descriptor();

        if (!descriptor.is_valid()) {
            return false;
        }

        const ActionId id = descriptor.id;

        return actions_
            .emplace(id, std::move(action))
            .second;
    }

    bool ActionRegistry::replace_action(
        std::unique_ptr<IEditorAction> action) {
        if (!action) {
            return false;
        }

        const ActionDescriptor& descriptor =
            action->descriptor();

        if (!descriptor.is_valid()) {
            return false;
        }

        const ActionId id = descriptor.id;
        actions_[id] = std::move(action);
        return true;
    }

    bool ActionRegistry::unregister_action(
        const ActionId& id) {
        if (id.is_invalid()) {
            return false;
        }

        return actions_.erase(id) > 0u;
    }

    bool ActionRegistry::contains(
        const ActionId& id) const {
        if (id.is_invalid()) {
            return false;
        }

        return actions_.find(id) != actions_.end();
    }

    IEditorAction* ActionRegistry::find(
        const ActionId& id) {
        const auto it = actions_.find(id);

        if (it == actions_.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    const IEditorAction* ActionRegistry::find(
        const ActionId& id) const {
        const auto it = actions_.find(id);

        if (it == actions_.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    const ActionDescriptor* ActionRegistry::descriptor(
        const ActionId& id) const {
        const IEditorAction* action = find(id);

        if (!action) {
            return nullptr;
        }

        return &action->descriptor();
    }

    std::vector<ActionId>
        ActionRegistry::action_ids() const {
        std::vector<ActionId> ids{};
        ids.reserve(actions_.size());

        for (const auto& [id, action] : actions_) {
            (void)action;
            ids.push_back(id);
        }

        return ids;
    }

    std::vector<const ActionDescriptor*>
        ActionRegistry::descriptors_by_category(
            ActionCategory category) const {
        std::vector<const ActionDescriptor*> descriptors{};

        for (const auto& [id, action] : actions_) {
            (void)id;

            if (action
                && action->descriptor().category == category) {
                descriptors.push_back(
                    &action->descriptor());
            }
        }

        return descriptors;
    }

    void ActionRegistry::clear() {
        actions_.clear();
    }

    std::size_t ActionRegistry::size() const {
        return actions_.size();
    }

    bool ActionRegistry::empty() const {
        return actions_.empty();
    }

} // namespace locus::editor