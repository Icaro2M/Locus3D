/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace locus::editor {

	/**
	 * @brief Registry for named command factories.
	 *
	 * The registry is useful for future menus, shortcuts, tools, and scripting
	 * layers that need to instantiate commands by stable identifiers.
	 */
	class CommandRegistry {
	public:
		/**
		 * @brief Factory used to create a command instance.
		 */
		using Factory = std::function<std::unique_ptr<ICommand>()>;

		/**
		 * @brief Registers a command factory.
		 *
		 * @param id Stable command identifier.
		 * @param factory Factory used to create the command.
		 * @return True when inserted, false when the id already existed or input was invalid.
		 */
		bool register_command(std::string id, Factory factory);

		/**
		 * @brief Replaces or inserts a command factory.
		 *
		 * @param id Stable command identifier.
		 * @param factory Factory used to create the command.
		 * @return True when the registry changed.
		 */
		bool replace_command(std::string id, Factory factory);

		/**
		 * @brief Removes a command factory.
		 *
		 * @param id Stable command identifier.
		 * @return True when a factory was removed.
		 */
		bool unregister_command(const std::string& id);

		/**
		 * @brief Checks whether a command identifier is registered.
		 *
		 * @param id Stable command identifier.
		 * @return True when registered.
		 */
		[[nodiscard]] bool contains(const std::string& id) const;

		/**
		 * @brief Creates a command instance from a registered factory.
		 *
		 * @param id Stable command identifier.
		 * @return New command instance, or null when not registered.
		 */
		[[nodiscard]] std::unique_ptr<ICommand> create(const std::string& id) const;

		/**
		 * @brief Returns all registered command identifiers.
		 *
		 * @return Command identifier list.
		 */
		[[nodiscard]] std::vector<std::string> command_ids() const;

		/**
		 * @brief Removes every registered factory.
		 */
		void clear();

		/**
		 * @brief Returns the number of registered factories.
		 *
		 * @return Registry size.
		 */
		[[nodiscard]] std::size_t size() const;

		/**
		 * @brief Checks whether the registry is empty.
		 *
		 * @return True when empty.
		 */
		[[nodiscard]] bool empty() const;

	private:
		std::unordered_map<std::string, Factory> factories_{};
	};

}