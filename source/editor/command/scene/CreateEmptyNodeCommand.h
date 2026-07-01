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
	 * @brief Command that creates an empty transform node in the editor scene.
	 */
	class CreateEmptyNodeCommand final : public ICommand {
	public:
		/**
		 * @brief Creates a command.
		 *
		 * @param name Name assigned to the created node.
		 */
		explicit CreateEmptyNodeCommand(std::string name = "Empty");

		/**
		 * @brief Returns the command name.
		 *
		 * @return Command name.
		 */
		[[nodiscard]] std::string_view name() const override;

		/**
		 * @brief Returns the last created node identifier.
		 *
		 * The id can change after redo because the current scene tree owns id
		 * allocation.
		 *
		 * @return Created node id, or invalid before execution.
		 */
		[[nodiscard]] SceneNodeId created_node() const;

		/**
		 * @brief Executes the command.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult execute(CommandContext& context) override;

		/**
		 * @brief Removes the created node.
		 *
		 * @param context Command context.
		 * @return Command result.
		 */
		CommandResult undo(CommandContext& context) override;

	private:
		std::string nodeName_{};
		SceneNodeId createdNode_{};
	};

}