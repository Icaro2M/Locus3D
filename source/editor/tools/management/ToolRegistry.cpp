/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/management/ToolRegistry.h"

#include <utility>

namespace locus::editor {

    bool ToolRegistry::register_tool(
        ToolDescriptor descriptor,
        Factory factory) {

        if (!descriptor.is_valid() || !factory) {
            return false;
        }

        const ToolId id = descriptor.id;

        Registration registration{};
        registration.descriptor = std::move(descriptor);
        registration.factory = std::move(factory);

        return registrations_
            .emplace(id, std::move(registration))
            .second;
    }

    bool ToolRegistry::replace_tool(
        ToolDescriptor descriptor,
        Factory factory) {

        if (!descriptor.is_valid() || !factory) {
            return false;
        }

        const ToolId id = descriptor.id;

        Registration registration{};
        registration.descriptor = std::move(descriptor);
        registration.factory = std::move(factory);

        registrations_[id] = std::move(registration);
        return true;
    }

    bool ToolRegistry::unregister_tool(const ToolId& id) {
        if (id.is_invalid()) {
            return false;
        }

        return registrations_.erase(id) > 0u;
    }

    bool ToolRegistry::contains(const ToolId& id) const {
        if (id.is_invalid()) {
            return false;
        }

        return registrations_.find(id) != registrations_.end();
    }

    const ToolDescriptor* ToolRegistry::descriptor(
        const ToolId& id) const {

        const auto it = registrations_.find(id);
        if (it == registrations_.end()) {
            return nullptr;
        }

        return &it->second.descriptor;
    }

    std::unique_ptr<ITool> ToolRegistry::create(
        const ToolId& id) const {

        const auto it = registrations_.find(id);
        if (it == registrations_.end() || !it->second.factory) {
            return nullptr;
        }

        std::unique_ptr<ITool> tool = it->second.factory();
        if (!tool) {
            return nullptr;
        }

        const ToolDescriptor& createdDescriptor = tool->descriptor();

        if (!createdDescriptor.is_valid() ||
            createdDescriptor.id != it->second.descriptor.id) {

            return nullptr;
        }

        return tool;
    }

    std::vector<ToolId> ToolRegistry::tool_ids() const {
        std::vector<ToolId> ids{};
        ids.reserve(registrations_.size());

        for (const auto& [id, registration] : registrations_) {
            (void)registration;
            ids.push_back(id);
        }

        return ids;
    }

    std::vector<const ToolDescriptor*>
        ToolRegistry::descriptors_by_category(
            ToolCategory category) const {

        std::vector<const ToolDescriptor*> descriptors{};

        for (const auto& [id, registration] : registrations_) {
            (void)id;

            if (registration.descriptor.category == category) {
                descriptors.push_back(&registration.descriptor);
            }
        }

        return descriptors;
    }

    void ToolRegistry::clear() {
        registrations_.clear();
    }

    std::size_t ToolRegistry::size() const {
        return registrations_.size();
    }

    bool ToolRegistry::empty() const {
        return registrations_.empty();
    }

} // namespace locus::editor