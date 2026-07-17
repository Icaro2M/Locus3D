/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/core/IEditorAction.h"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace locus::editor {

    /**
     * @brief Registry owning immediate editor action instances.
     *
     * Actions are stored as persistent instances because immediate actions do
     * not retain modal interaction state. Interactive workflows requiring a
     * fresh stateful instance belong to the tools subsystem.
     *
     * Registered action identifiers must be valid and unique. The identifier
     * exposed by each action descriptor is used as the registry key.
     */
    class ActionRegistry {
    public:
        /**
         * @brief Registers an action instance.
         *
         * The action and its descriptor must be valid. Registration fails when
         * another action already uses the same identifier.
         *
         * @param action Owned action instance.
         * @return True when the action was registered.
         */
        bool register_action(
            std::unique_ptr<IEditorAction> action);

        /**
         * @brief Replaces or inserts an action instance.
         *
         * The action and its descriptor must be valid.
         *
         * @param action Owned action instance.
         * @return True when the supplied action was valid.
         */
        bool replace_action(
            std::unique_ptr<IEditorAction> action);

        /**
         * @brief Removes a registered action.
         *
         * @param id Stable action identifier.
         * @return True when an action was removed.
         */
        bool unregister_action(const ActionId& id);

        /**
         * @brief Checks whether an action identifier is registered.
         *
         * @param id Stable action identifier.
         * @return True when the action is registered.
         */
        [[nodiscard]] bool contains(
            const ActionId& id) const;

        /**
         * @brief Returns a mutable registered action.
         *
         * @param id Stable action identifier.
         * @return Action pointer, or null when not registered.
         */
        [[nodiscard]] IEditorAction* find(
            const ActionId& id);

        /**
         * @brief Returns a read-only registered action.
         *
         * @param id Stable action identifier.
         * @return Action pointer, or null when not registered.
         */
        [[nodiscard]] const IEditorAction* find(
            const ActionId& id) const;

        /**
         * @brief Returns the descriptor of a registered action.
         *
         * @param id Stable action identifier.
         * @return Descriptor pointer, or null when not registered.
         */
        [[nodiscard]] const ActionDescriptor* descriptor(
            const ActionId& id) const;

        /**
         * @brief Returns all registered action identifiers.
         *
         * No ordering guarantee is provided.
         *
         * @return Action identifier list.
         */
        [[nodiscard]] std::vector<ActionId>
            action_ids() const;

        /**
         * @brief Returns descriptors belonging to one category.
         *
         * The returned pointers remain valid until the corresponding actions
         * are removed or replaced.
         *
         * @param category Category to query.
         * @return Pointers to matching descriptors.
         */
        [[nodiscard]] std::vector<const ActionDescriptor*>
            descriptors_by_category(
                ActionCategory category) const;

        /**
         * @brief Removes every registered action.
         */
        void clear();

        /**
         * @brief Returns the number of registered actions.
         *
         * @return Registry size.
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Checks whether the registry is empty.
         *
         * @return True when no actions are registered.
         */
        [[nodiscard]] bool empty() const;

    private:
        std::unordered_map<
            ActionId,
            std::unique_ptr<IEditorAction>>
            actions_{};
    };

} // namespace locus::editor