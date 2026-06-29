/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/CommandResult.h"
#include "editor/command/ICommand.h"

#include <memory>
#include <string>

namespace locus::editor {

	/**
	 * @brief Entry stored by the editor command history.
	 *
	 * A history entry owns one command instance and stores lightweight metadata
	 * about the last operation performed with it.
	 */
	class HistoryEntry {
	public:
		/**
		 * @brief Creates an empty history entry.
		 */
		HistoryEntry() = default;

		/**
		 * @brief Creates a history entry from an executed command.
		 *
		 * @param command Command owned by the entry.
		 * @param result Last command result.
		 */
		HistoryEntry(std::unique_ptr<ICommand> command, CommandResult result)
			: command_(std::move(command))
			, commandName_(command_ ? std::string(command_->name()) : std::string{})
			, lastResult_(std::move(result))
		{
		}

		HistoryEntry(const HistoryEntry&) = delete;
		HistoryEntry& operator=(const HistoryEntry&) = delete;
		HistoryEntry(HistoryEntry&&) noexcept = default;
		HistoryEntry& operator=(HistoryEntry&&) noexcept = default;

		/**
		 * @brief Destroys the history entry.
		 */
		~HistoryEntry() = default;

		/**
		 * @brief Checks whether this entry owns a command.
		 *
		 * @return True when valid.
		 */
		[[nodiscard]] bool is_valid() const
		{
			return command_ != nullptr;
		}

		/**
		 * @brief Returns the owned command.
		 *
		 * @return Mutable command reference.
		 */
		[[nodiscard]] ICommand& command()
		{
			return *command_;
		}

		/**
		 * @brief Returns the owned command.
		 *
		 * @return Read-only command reference.
		 */
		[[nodiscard]] const ICommand& command() const
		{
			return *command_;
		}

		/**
		 * @brief Releases the owned command.
		 *
		 * @return Owned command pointer.
		 */
		[[nodiscard]] std::unique_ptr<ICommand> take_command()
		{
			return std::move(command_);
		}

		/**
		 * @brief Returns the command name captured when the entry was created.
		 *
		 * @return Command name.
		 */
		[[nodiscard]] const std::string& command_name() const
		{
			return commandName_;
		}

		/**
		 * @brief Returns the last command result associated with this entry.
		 *
		 * @return Last command result.
		 */
		[[nodiscard]] const CommandResult& last_result() const
		{
			return lastResult_;
		}

		/**
		 * @brief Updates the last command result associated with this entry.
		 *
		 * @param result New last result.
		 */
		void set_last_result(CommandResult result)
		{
			lastResult_ = std::move(result);
		}

	private:
		std::unique_ptr<ICommand> command_{};
		std::string commandName_{};
		CommandResult lastResult_{};
	};

}