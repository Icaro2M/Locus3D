/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/CreateEmptyNodeCommand.h"

#include "editor/scene/EditorScene.h"

#include <utility>

namespace locus::editor {

	CreateEmptyNodeCommand::CreateEmptyNodeCommand(std::string name)
		: nodeName_(std::move(name))
	{
		if (nodeName_.empty()) {
			nodeName_ = "Empty";
		}
	}

	std::string_view CreateEmptyNodeCommand::name() const
	{
		return "Create Empty Node";
	}

	SceneNodeId CreateEmptyNodeCommand::created_node() const
	{
		return createdNode_;
	}

	CommandResult CreateEmptyNodeCommand::execute(CommandContext& context)
	{
		createdNode_ = context.scene().create_empty(nodeName_);

		if (createdNode_.is_invalid()) {
			return CommandResult::fail("Failed to create empty node.");
		}

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Empty node created.");
	}

	CommandResult CreateEmptyNodeCommand::undo(CommandContext& context)
	{
		if (createdNode_.is_invalid()) {
			return CommandResult::fail("Cannot undo empty node creation without a valid node id.");
		}

		if (!context.scene().remove_node(createdNode_)) {
			return CommandResult::fail("Failed to remove created empty node.");
		}

		createdNode_ = {};

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Empty node creation undone.");
	}

}