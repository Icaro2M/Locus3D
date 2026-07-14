/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ToolCapabilities.h"
#include "editor/tools/core/ToolCategory.h"
#include "editor/tools/core/ToolId.h"

#include <string>
#include <utility>

namespace locus::editor {

    /**
     * @brief Static metadata describing an editor tool.
     *
     * Tool descriptors are presentation-independent records used by registries,
     * toolbars, keymaps, command palettes, and diagnostics. The stable identifier
     * must not depend on the user-visible label.
     */
    struct ToolDescriptor {
        /**
         * @brief Stable identifier used to reference the tool.
         */
        ToolId id{};

        /**
         * @brief Human-readable tool name.
         */
        std::string name{};

        /**
         * @brief Optional human-readable description.
         */
        std::string description{};

        /**
         * @brief High-level category used to organize the tool.
         */
        ToolCategory category = ToolCategory::Utility;

        /**
         * @brief Capability and requirement flags exposed by the tool.
         */
        ToolCapabilities capabilities = ToolCapabilities::None;

        /**
         * @brief Creates an empty descriptor.
         */
        ToolDescriptor() = default;

        /**
         * @brief Creates a complete tool descriptor.
         *
         * @param id Stable tool identifier.
         * @param name Human-readable tool name.
         * @param description Human-readable tool description.
         * @param category High-level tool category.
         * @param capabilities Capability mask.
         */
        ToolDescriptor(
            ToolId id,
            std::string name,
            std::string description,
            ToolCategory category,
            ToolCapabilities capabilities)
            : id(std::move(id)),
            name(std::move(name)),
            description(std::move(description)),
            category(category),
            capabilities(capabilities) {
        }

        /**
         * @brief Checks whether this descriptor can identify a tool.
         *
         * @return True when the identifier and display name are valid.
         */
        [[nodiscard]] bool is_valid() const {
            return id.is_valid() && !name.empty();
        }

        /**
         * @brief Checks whether the descriptor exposes a capability.
         *
         * @param capability Capability to test.
         * @return True when the capability is present.
         */
        [[nodiscard]] bool has_capability(
            ToolCapabilities capability) const {

            return locus::editor::has_capability(
                capabilities,
                capability);
        }
    };

} // namespace locus::editor