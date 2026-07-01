/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/scene/SceneNodeId.h"

#include <string>
#include <string_view>

namespace locus::editor {

	/**
	 * @brief Command that changes a scene node name.
	 */
	class RenameNodeCommand final : public ICommand {
	public:
		/**
		 * @brief Creates a command.
		 *
		 * @param id Node to rename.
		 * @param newName New node name.
		 */
		RenameNodeCommand(SceneNodeId id, std::string newName);

		/**
		 * @brief Returns the command name.
		 *
		 * @return Command name.
		 */
		[[nodiscard]] std::string_view name() const override;

		/**
		 * @brief Executes the command.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult execute(CommandContext& context) override;

		/**
		 * @brief Restores the previous node name.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult undo(CommandContext& context) override;

		/**
		 * @brief Reapplies the new node name.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult redo(CommandContext& context) override;

	private:
		CommandResult apply_name(CommandContext& context, const std::string& value, std::string_view message);

		SceneNodeId node_{};
		std::string newName_{};
		std::string previousName_{};
		bool captured_ = false;
	};

}