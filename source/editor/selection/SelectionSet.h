/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace locus::editor {

    /**
     * @brief Ordered unique set used by editor selection containers.
     *
     * @tparam ItemT Selected item type.
     */
    template <typename ItemT>
    class SelectionSet {
    public:
        /**
         * @brief Adds an item to the selection.
         *
         * @param item Item to add.
         * @return True when the item was added.
         */
        bool add(ItemT item)
        {
            if (contains(item)) {
                return false;
            }

            items_.push_back(item);
            return true;
        }

        /**
         * @brief Removes an item from the selection.
         *
         * @param item Item to remove.
         * @return True when the item was removed.
         */
        bool remove(ItemT item)
        {
            const auto it = std::remove(items_.begin(), items_.end(), item);
            if (it == items_.end()) {
                return false;
            }

            items_.erase(it, items_.end());
            return true;
        }

        /**
         * @brief Toggles the presence of an item in the selection.
         *
         * @param item Item to toggle.
         * @return True when the item is selected after the operation.
         */
        bool toggle(ItemT item)
        {
            if (contains(item)) {
                remove(item);
                return false;
            }

            add(item);
            return true;
        }

        /**
         * @brief Replaces the selection with a single item.
         *
         * @param item Item to select.
         */
        void set(ItemT item)
        {
            items_.clear();
            items_.push_back(item);
        }

        /**
         * @brief Replaces the selection with a list of items.
         *
         * Duplicate items are ignored while preserving first occurrence order.
         *
         * @param items Items to select.
         */
        void set(const std::vector<ItemT>& items)
        {
            items_.clear();

            for (ItemT item : items) {
                add(item);
            }
        }

        /**
         * @brief Checks whether an item is selected.
         *
         * @param item Item to inspect.
         * @return True when the item is selected.
         */
        [[nodiscard]] bool contains(ItemT item) const
        {
            return std::find(items_.begin(), items_.end(), item) != items_.end();
        }

        /**
         * @brief Removes all selected items.
         */
        void clear()
        {
            items_.clear();
        }

        /**
         * @brief Returns all selected items.
         *
         * @return Ordered selected item list.
         */
        [[nodiscard]] const std::vector<ItemT>& items() const
        {
            return items_;
        }

        /**
         * @brief Returns the number of selected items.
         *
         * @return Selection count.
         */
        [[nodiscard]] std::size_t size() const
        {
            return items_.size();
        }

        /**
         * @brief Checks whether no items are selected.
         *
         * @return True when the set is empty.
         */
        [[nodiscard]] bool empty() const
        {
            return items_.empty();
        }

    private:
        std::vector<ItemT> items_{};
    };

    /**
     * @brief Translation unit anchor for selection set templates.
     */
    void selection_set_anchor();

}