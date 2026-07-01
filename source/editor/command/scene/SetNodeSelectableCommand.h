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
	 * @brief Command that changes whether a scene node can be selected.
	 */
	class SetNodeSelectableCommand final : public ICommand {
	public:
		/**
		 * @brief Creates a command.
		 *
		 * @param id Node to update.
		 * @param selectable New selectable state.
		 */
		SetNodeSelectableCommand(SceneNodeId id, bool selectable);

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
		 * @brief Restores the previous selectable state.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult undo(CommandContext& context) override;

		/**
		 * @brief Reapplies the selectable state.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult redo(CommandContext& context) override;

	private:
		CommandResult apply_selectable(CommandContext& context, bool value, std::string_view message);

		SceneNodeId node_{};
		bool selectable_ = true;
		bool previousSelectable_ = true;
		bool captured_ = false;
	};

}