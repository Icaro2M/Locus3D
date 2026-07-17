/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

namespace locus::editor {

    /**
     * @brief Stable textual identifier used to reference editor actions.
     *
     * Action identifiers are independent from presentation labels and may be
     * used by registries, menus, keymaps, toolbars, command palettes, context
     * menus, and serialized editor preferences.
     */
    struct ActionId {
        /**
         * @brief Stable textual identifier value.
         */
        std::string value{};

        /**
         * @brief Creates an invalid action identifier.
         */
        ActionId() = default;

        /**
         * @brief Creates an action identifier from a textual value.
         *
         * @param value Stable identifier value.
         */
        explicit ActionId(std::string value)
            : value(std::move(value)) {
        }

        /**
         * @brief Checks whether this identifier contains a usable value.
         *
         * @return True when the identifier is not empty.
         */
        [[nodiscard]] bool is_valid() const {
            return !value.empty();
        }

        /**
         * @brief Checks whether this identifier is invalid.
         *
         * @return True when the identifier is empty.
         */
        [[nodiscard]] bool is_invalid() const {
            return !is_valid();
        }

        /**
         * @brief Compares two action identifiers for equality.
         *
         * @param lhs Left-hand identifier.
         * @param rhs Right-hand identifier.
         * @return True when both identifiers contain the same value.
         */
        friend bool operator==(const ActionId& lhs, const ActionId& rhs) {
            return lhs.value == rhs.value;
        }

        /**
         * @brief Compares two action identifiers for inequality.
         *
         * @param lhs Left-hand identifier.
         * @param rhs Right-hand identifier.
         * @return True when the identifiers contain different values.
         */
        friend bool operator!=(const ActionId& lhs, const ActionId& rhs) {
            return !(lhs == rhs);
        }
    };

} // namespace locus::editor

namespace std {

    /**
     * @brief Hash functor for editor action identifiers.
     */
    template <>
    struct hash<locus::editor::ActionId> {
        /**
         * @brief Hashes an action identifier.
         *
         * @param id Identifier to hash.
         * @return Hash value.
         */
        std::size_t operator()(
            const locus::editor::ActionId& id) const noexcept {
            return std::hash<std::string>{}(id.value);
        }
    };

} // namespace std