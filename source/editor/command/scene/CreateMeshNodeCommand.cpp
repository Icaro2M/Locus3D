/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/CreateMeshNodeCommand.h"

#include "editor/scene/EditorScene.h"

#include <utility>

namespace locus::editor {

	CreateMeshNodeCommand::CreateMeshNodeCommand(std::string name)
		: nodeName_(std::move(name))
	{
		if (nodeName_.empty()) {
			nodeName_ = "Mesh";
		}
	}

	std::string_view CreateMeshNodeCommand::name() const
	{
		return "Create Mesh Node";
	}

	SceneNodeId CreateMeshNodeCommand::created_node() const
	{
		return createdNode_;
	}

	CommandResult CreateMeshNodeCommand::execute(CommandContext& context)
	{
		createdNode_ = context.scene().create_mesh(nodeName_);

		if (createdNode_.is_invalid()) {
			return CommandResult::fail("Failed to create mesh node.");
		}

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Mesh |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Mesh node created.");
	}

	CommandResult CreateMeshNodeCommand::undo(CommandContext& context)
	{
		if (createdNode_.is_invalid()) {
			return CommandResult::fail("Cannot undo mesh node creation without a valid node id.");
		}

		if (!context.scene().remove_node(createdNode_)) {
			return CommandResult::fail("Failed to remove created mesh node.");
		}

		createdNode_ = {};

		return CommandResult::ok(
			EditorDirtyFlags::Scene |
			EditorDirtyFlags::Selection |
			EditorDirtyFlags::Mesh |
			EditorDirtyFlags::Render |
			EditorDirtyFlags::Picking,
			"Mesh node creation undone.");
	}

}