/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionId.h"

#include <string>
#include <utility>
#include <vector>

namespace locus::editor {

    /**
     * @brief Static metadata describing an immediate editor action.
     *
     * Action descriptors are presentation-independent records used by action
     * registries, menus, toolbars, keymaps, command palettes, context menus,
     * and diagnostics.
     */
    struct ActionDescriptor {
        /**
         * @brief Stable identifier used to reference the action.
         */
        ActionId id{};

        /**
         * @brief Human-readable action name.
         */
        std::string name{};

        /**
         * @brief Optional human-readable description.
         */
        std::string description{};

        /**
         * @brief High-level category used to organize the action.
         */
        ActionCategory category = ActionCategory::Utility;

        /**
         * @brief Optional search terms used by command palettes and menus.
         *
         * Keywords are presentation hints only and must not be used as stable
         * identifiers.
         */
        std::vector<std::string> keywords{};

        /**
         * @brief Creates an empty action descriptor.
         */
        ActionDescriptor() = default;

        /**
         * @brief Creates an action descriptor without additional keywords.
         *
         * @param id Stable action identifier.
         * @param name Human-readable action name.
         * @param description Human-readable action description.
         * @param category High-level action category.
         */
        ActionDescriptor(
            ActionId id,
            std::string name,
            std::string description,
            ActionCategory category)
            : id(std::move(id)),
            name(std::move(name)),
            description(std::move(description)),
            category(category) {
        }

        /**
         * @brief Creates a complete action descriptor.
         *
         * @param id Stable action identifier.
         * @param name Human-readable action name.
         * @param description Human-readable action description.
         * @param category High-level action category.
         * @param keywords Additional command palette search terms.
         */
        ActionDescriptor(
            ActionId id,
            std::string name,
            std::string description,
            ActionCategory category,
            std::vector<std::string> keywords)
            : id(std::move(id)),
            name(std::move(name)),
            description(std::move(description)),
            category(category),
            keywords(std::move(keywords)) {
        }

        /**
         * @brief Checks whether this descriptor can identify an action.
         *
         * @return True when the identifier and display name are valid.
         */
        [[nodiscard]] bool is_valid() const {
            return id.is_valid() && !name.empty();
        }
    };

} // namespace locus::editor