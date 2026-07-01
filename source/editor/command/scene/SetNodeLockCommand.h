/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/scene/SceneNodeId.h"

#include <string_view>

namespace locus::editor {

	/**
	 * @brief Command that changes a scene node lock flag.
	 */
	class SetNodeLockCommand final : public ICommand {
	public:
		/**
		 * @brief Creates a command.
		 *
		 * @param id Node to update.
		 * @param locked New lock state.
		 */
		SetNodeLockCommand(SceneNodeId id, bool locked);

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
		 * @brief Restores the previous lock state.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult undo(CommandContext& context) override;

		/**
		 * @brief Reapplies the lock state.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult redo(CommandContext& context) override;

	private:
		CommandResult apply_locked(CommandContext& context, bool value, std::string_view message);

		SceneNodeId node_{};
		bool locked_ = false;
		bool previousLocked_ = false;
		bool captured_ = false;
	};

}