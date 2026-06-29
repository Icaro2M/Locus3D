/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/CommandContext.h"
#include "editor/command/CommandResult.h"

#include <memory>

namespace locus::editor {

	class ICommand;

	/**
	 * @brief Executes editor commands through a shared command context.
	 *
	 * The dispatcher is intentionally small for now. History integration can be
	 * added later without changing command implementations.
	 */
	class CommandDispatcher {
	public:
		/**
		 * @brief Creates a command dispatcher.
		 *
		 * @param editor Editor facade used by command execution.
		 */
		explicit CommandDispatcher(Editor& editor);

		/**
		 * @brief Returns the command context used by this dispatcher.
		 *
		 * @return Command context reference.
		 */
		[[nodiscard]] CommandContext& context();

		/**
		 * @brief Returns the command context used by this dispatcher.
		 *
		 * @return Read-only command context reference.
		 */
		[[nodiscard]] const CommandContext& context() const;

		/**
		 * @brief Executes a command.
		 *
		 * @param command Command to execute.
		 * @return Execution result.
		 */
		CommandResult execute(ICommand& command);

		/**
		 * @brief Executes an owned command.
		 *
		 * @param command Command to execute.
		 * @return Execution result.
		 */
		CommandResult execute(std::unique_ptr<ICommand> command);

		/**
		 * @brief Undoes a command.
		 *
		 * @param command Command to undo.
		 * @return Undo result.
		 */
		CommandResult undo(ICommand& command);

		/**
		 * @brief Redoes a command.
		 *
		 * @param command Command to redo.
		 * @return Redo result.
		 */
		CommandResult redo(ICommand& command);

		/**
		 * @brief Returns the last command result produced by this dispatcher.
		 *
		 * @return Last command result.
		 */
		[[nodiscard]] const CommandResult& last_result() const;

	private:
		void apply_result(const CommandResult& result);

		CommandContext context_;
		CommandResult lastResult_{};
	};

}