/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/SetNodeLockCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>

namespace locus::editor {

	SetNodeLockCommand::SetNodeLockCommand(SceneNodeId id, bool locked)
		: node_(id)
		, locked_(locked)
	{
	}

	std::string_view SetNodeLockCommand::name() const
	{
		return "Set Node Lock";
	}

	CommandResult SetNodeLockCommand::execute(CommandContext& context)
	{
		if (node_.is_invalid()) {
			return CommandResult::fail("Cannot change lock state of an invalid node.");
		}

		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot change lock state of a missing node.");
		}

		if (!captured_) {
			previousLocked_ = node->metadata().locked;
			captured_ = true;
		}

		return apply_locked(context, locked_, "Node lock state changed.");
	}

	CommandResult SetNodeLockCommand::undo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot undo lock state change without a previous value.");
		}

		return apply_locked(context, previousLocked_, "Node lock state restored.");
	}

	CommandResult SetNodeLockCommand::redo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot redo lock state change before execution.");
		}

		return apply_locked(context, locked_, "Node lock state changed.");
	}

	CommandResult SetNodeLockCommand::apply_locked(
		CommandContext& context,
		bool value,
		std::string_view message)
	{
		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot change lock state of a missing node.");
		}

		node->metadata().locked = value;
		node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			std::string(message));
	}

}