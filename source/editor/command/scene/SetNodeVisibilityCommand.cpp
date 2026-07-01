/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/SetNodeVisibilityCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>

namespace locus::editor {

	SetNodeVisibilityCommand::SetNodeVisibilityCommand(SceneNodeId id, bool visible)
		: node_(id)
		, visible_(visible)
	{
	}

	std::string_view SetNodeVisibilityCommand::name() const
	{
		return "Set Node Visibility";
	}

	CommandResult SetNodeVisibilityCommand::execute(CommandContext& context)
	{
		if (node_.is_invalid()) {
			return CommandResult::fail("Cannot change visibility of an invalid node.");
		}

		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot change visibility of a missing node.");
		}

		if (!captured_) {
			previousVisible_ = node->metadata().visible;
			captured_ = true;
		}

		return apply_visibility(context, visible_, "Node visibility changed.");
	}

	CommandResult SetNodeVisibilityCommand::undo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot undo node visibility change without a previous value.");
		}

		return apply_visibility(context, previousVisible_, "Node visibility restored.");
	}

	CommandResult SetNodeVisibilityCommand::redo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot redo node visibility change before execution.");
		}

		return apply_visibility(context, visible_, "Node visibility changed.");
	}

	CommandResult SetNodeVisibilityCommand::apply_visibility(
		CommandContext& context,
		bool value,
		std::string_view message)
	{
		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot change visibility of a missing node.");
		}

		node->metadata().visible = value;
		node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			std::string(message));
	}

}