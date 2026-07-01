/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/SetNodeSelectableCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>

namespace locus::editor {

	SetNodeSelectableCommand::SetNodeSelectableCommand(SceneNodeId id, bool selectable)
		: node_(id)
		, selectable_(selectable)
	{
	}

	std::string_view SetNodeSelectableCommand::name() const
	{
		return "Set Node Selectable";
	}

	CommandResult SetNodeSelectableCommand::execute(CommandContext& context)
	{
		if (node_.is_invalid()) {
			return CommandResult::fail("Cannot change selectable state of an invalid node.");
		}

		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot change selectable state of a missing node.");
		}

		if (!captured_) {
			previousSelectable_ = node->metadata().selectable;
			captured_ = true;
		}

		return apply_selectable(context, selectable_, "Node selectable state changed.");
	}

	CommandResult SetNodeSelectableCommand::undo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot undo selectable state change without a previous value.");
		}

		return apply_selectable(context, previousSelectable_, "Node selectable state restored.");
	}

	CommandResult SetNodeSelectableCommand::redo(CommandContext& context)
	{
		if (!captured_) {
			return CommandResult::fail("Cannot redo selectable state change before execution.");
		}

		return apply_selectable(context, selectable_, "Node selectable state changed.");
	}

	CommandResult SetNodeSelectableCommand::apply_selectable(
		CommandContext& context,
		bool value,
		std::string_view message)
	{
		SceneNode* node = context.scene().find_node(node_);
		if (!node) {
			return CommandResult::fail("Cannot change selectable state of a missing node.");
		}

		node->metadata().selectable = value;
		node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			std::string(message));
	}

}