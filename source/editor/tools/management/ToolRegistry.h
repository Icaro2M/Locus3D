/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ITool.h"
#include "editor/tools/core/ToolDescriptor.h"
#include "editor/tools/core/ToolId.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace locus::editor {

    /**
     * @brief Registry for editor tool descriptors and factories.
     *
     * The registry stores metadata independently from tool instances. A fresh tool
     * instance is created whenever the tool manager activates a registered tool.
     */
    class ToolRegistry {
    public:
        /**
         * @brief Factory used to create an editor tool instance.
         */
        using Factory = std::function<std::unique_ptr<ITool>()>;

        /**
         * @brief Registers a tool descriptor and factory.
         *
         * The descriptor identifier must be valid and unique.
         *
         * @param descriptor Static tool metadata.
         * @param factory Factory used to create tool instances.
         * @return True when the registration was inserted.
         */
        bool register_tool(
            ToolDescriptor descriptor,
            Factory factory);

        /**
         * @brief Replaces or inserts a tool registration.
         *
         * @param descriptor Static tool metadata.
         * @param factory Factory used to create tool instances.
         * @return True when the supplied registration was valid.
         */
        bool replace_tool(
            ToolDescriptor descriptor,
            Factory factory);

        /**
         * @brief Removes a registered tool.
         *
         * @param id Stable tool identifier.
         * @return True when a registration was removed.
         */
        bool unregister_tool(const ToolId& id);

        /**
         * @brief Checks whether a tool identifier is registered.
         *
         * @param id Stable tool identifier.
         * @return True when the identifier is registered.
         */
        [[nodiscard]] bool contains(const ToolId& id) const;

        /**
         * @brief Returns the descriptor of a registered tool.
         *
         * @param id Stable tool identifier.
         * @return Descriptor pointer, or null when not registered.
         */
        [[nodiscard]] const ToolDescriptor* descriptor(
            const ToolId& id) const;

        /**
         * @brief Creates a fresh tool instance.
         *
         * The created instance is validated against the registered identifier.
         * Factories returning null or a tool with a mismatched descriptor are
         * rejected.
         *
         * @param id Stable tool identifier.
         * @return New tool instance, or null when creation failed.
         */
        [[nodiscard]] std::unique_ptr<ITool> create(
            const ToolId& id) const;

        /**
         * @brief Returns all registered tool identifiers.
         *
         * @return Tool identifier list.
         */
        [[nodiscard]] std::vector<ToolId> tool_ids() const;

        /**
         * @brief Returns descriptors belonging to a category.
         *
         * @param category Category to query.
         * @return Pointers to matching descriptors.
         */
        [[nodiscard]] std::vector<const ToolDescriptor*>
            descriptors_by_category(ToolCategory category) const;

        /**
         * @brief Removes every tool registration.
         */
        void clear();

        /**
         * @brief Returns the number of registered tools.
         *
         * @return Registry size.
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Checks whether the registry is empty.
         *
         * @return True when no tools are registered.
         */
        [[nodiscard]] bool empty() const;

    private:
        /**
         * @brief Complete registry entry.
         */
        struct Registration {
            ToolDescriptor descriptor{};
            Factory factory{};
        };

        std::unordered_map<ToolId, Registration> registrations_{};
    };

} // namespace locus::editor