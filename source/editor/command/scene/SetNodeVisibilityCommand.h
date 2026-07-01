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
	 * @brief Command that changes a scene node visibility flag.
	 */
	class SetNodeVisibilityCommand final : public ICommand {
	public:
		/**
		 * @brief Creates a command.
		 *
		 * @param id Node to update.
		 * @param visible New visibility state.
		 */
		SetNodeVisibilityCommand(SceneNodeId id, bool visible);

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
		 * @brief Restores the previous visibility state.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult undo(CommandContext& context) override;

		/**
		 * @brief Reapplies the visibility state.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult redo(CommandContext& context) override;

	private:
		CommandResult apply_visibility(CommandContext& context, bool value, std::string_view message);

		SceneNodeId node_{};
		bool visible_ = true;
		bool previousVisible_ = true;
		bool captured_ = false;
	};

}