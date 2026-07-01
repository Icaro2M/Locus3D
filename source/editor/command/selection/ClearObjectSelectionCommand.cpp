/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/ClearObjectSelectionCommand.h"

#include "editor/selection/SelectionController.h"

namespace locus::editor {

	std::string_view ClearObjectSelectionCommand::name() const
	{
		return "Clear Object Selection";
	}

	CommandResult ClearObjectSelectionCommand::execute(CommandContext& context)
	{
		previousSelection_.capture(context.selection());

		context.selection_controller().clear_objects();
		executed_ = true;

		return CommandResult::ok(
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Object selection cleared.");
	}

	CommandResult ClearObjectSelectionCommand::undo(CommandContext& context)
	{
		if (!executed_ || !previousSelection_.is_valid()) {
			return CommandResult::fail("Cannot undo clear object selection without a previous selection snapshot.");
		}

		previousSelection_.restore(context.selection());

		return CommandResult::ok(
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Object selection restored.");
	}

}