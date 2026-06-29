/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/CommandRegistry.h"

#include <utility>

namespace locus::editor {

	bool CommandRegistry::register_command(std::string id, Factory factory)
	{
		if (id.empty() || !factory) {
			return false;
		}

		return factories_.emplace(std::move(id), std::move(factory)).second;
	}

	bool CommandRegistry::replace_command(std::string id, Factory factory)
	{
		if (id.empty() || !factory) {
			return false;
		}

		factories_[std::move(id)] = std::move(factory);
		return true;
	}

	bool CommandRegistry::unregister_command(const std::string& id)
	{
		return factories_.erase(id) > 0u;
	}

	bool CommandRegistry::contains(const std::string& id) const
	{
		return factories_.find(id) != factories_.end();
	}

	std::unique_ptr<ICommand> CommandRegistry::create(const std::string& id) const
	{
		const auto it = factories_.find(id);
		if (it == factories_.end()) {
			return nullptr;
		}

		return it->second();
	}

	std::vector<std::string> CommandRegistry::command_ids() const
	{
		std::vector<std::string> ids{};
		ids.reserve(factories_.size());

		for (const auto& [id, factory] : factories_) {
			(void)factory;
			ids.push_back(id);
		}

		return ids;
	}

	void CommandRegistry::clear()
	{
		factories_.clear();
	}

	std::size_t CommandRegistry::size() const
	{
		return factories_.size();
	}

	bool CommandRegistry::empty() const
	{
		return factories_.empty();
	}

}