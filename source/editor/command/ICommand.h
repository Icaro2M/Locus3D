/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/CommandContext.h"
#include "editor/command/CommandResult.h"

#include <string_view>

namespace locus::editor {

	/**
	 * @brief Interface implemented by reversible editor commands.
	 *
	 * Commands represent editor-level intent. They may call scene, selection, or
	 * kernel operations, but command history should remain above low-level geometry
	 * storage and GPU resources.
	 */
	class ICommand {
	public:
		/**
		 * @brief Destroys the command.
		 */
		virtual ~ICommand() = default;

		ICommand() = default;
		ICommand(const ICommand&) = delete;
		ICommand& operator=(const ICommand&) = delete;
		ICommand(ICommand&&) = default;
		ICommand& operator=(ICommand&&) = default;

		/**
		 * @brief Returns a human-readable command name.
		 *
		 * @return Command name.
		 */
		[[nodiscard]] virtual std::string_view name() const = 0;

		/**
		 * @brief Checks whether this command can be stored in undo history.
		 *
		 * @return True when undoable.
		 */
		[[nodiscard]] virtual bool is_undoable() const
		{
			return true;
		}

		/**
		 * @brief Executes the command.
		 *
		 * @param context Command execution context.
		 * @return Execution result.
		 */
		virtual CommandResult execute(CommandContext& context) = 0;

		/**
		 * @brief Reverts the command.
		 *
		 * @param context Command execution context.
		 * @return Undo result.
		 */
		virtual CommandResult undo(CommandContext& context) = 0;

		/**
		 * @brief Re-executes the command after an undo.
		 *
		 * @param context Command execution context.
		 * @return Redo result.
		 */
		virtual CommandResult redo(CommandContext& context)
		{
			return execute(context);
		}
	};

}