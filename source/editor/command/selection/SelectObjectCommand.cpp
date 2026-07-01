/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/SelectObjectCommand.h"

#include "editor/selection/SelectionController.h"

namespace locus::editor {

	SelectObjectCommand::SelectObjectCommand(SceneNodeId id)
		: object_(id)
	{
	}

	std::string_view SelectObjectCommand::name() const
	{
		return "Select Object";
	}

	CommandResult SelectObjectCommand::execute(CommandContext& context)
	{
		if (object_.is_invalid()) {
			return CommandResult::fail("Cannot select an invalid object.");
		}

		previousSelection_.capture(context.selection());

		if (!context.selection_controller().select_object(object_)) {
			return CommandResult::fail("Failed to select object.");
		}

		executed_ = true;

		return CommandResult::ok(
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Object selected.");
	}

	CommandResult SelectObjectCommand::undo(CommandContext& context)
	{
		if (!executed_ || !previousSelection_.is_valid()) {
			return CommandResult::fail("Cannot undo object selection without a previous selection snapshot.");
		}

		previousSelection_.restore(context.selection());

		return CommandResult::ok(
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Object selection restored.");
	}

}