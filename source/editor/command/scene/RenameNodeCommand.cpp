/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/RenameNodeCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <utility>

namespace locus::editor {

	RenameNodeCommand::RenameNodeCommand(SceneNodeId id, std::string newName)
		: node_(id)
		, newName_(std::move(newName))
	{
	}

	std::string_view RenameNodeCommand::name() const
	{
		return "Rename Node";
	}

	CommandResult RenameNodeCommand::execute(CommandContext& context)
	{
		if (node_.is_invalid()) {
			return CommandResult::fail("Cannot rename an invalid node.");
		}

		if (newName_.empty()) {
			return CommandResult::fail("Cannot rename a node to an empty name.");
		}

		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot rename a missing node.");
		}

		if (!captured_) {
			previousName_ = node->metadata().name;
			captured_ = true;
		}

		return apply_name(context, newName_, "Node renamed.");
	}

	CommandResult RenameNodeCommand::undo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot undo node rename without a previous name.");
		}

		return apply_name(context, previousName_, "Node name restored.");
	}

	CommandResult RenameNodeCommand::redo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot redo node rename before execution.");
		}

		return apply_name(context, newName_, "Node renamed.");
	}

	CommandResult RenameNodeCommand::apply_name(
		CommandContext& context,
		const std::string& value,
		std::string_view message)
	{
		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot rename a missing node.");
		}

		node->metadata().name = value;
		node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			std::string(message));
	}

}