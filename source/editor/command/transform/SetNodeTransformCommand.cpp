/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/transform/SetNodeTransformCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>

namespace locus::editor {

    SetNodeTransformCommand::SetNodeTransformCommand(SceneNodeId id, const NodeTransform& transform)
        : node_(id)
        , nextTransform_(NodeTransformSnapshot::capture(transform)) {
    }

    std::string_view SetNodeTransformCommand::name() const {
        return "Set Node Transform";
    }

    CommandResult SetNodeTransformCommand::execute(CommandContext& context) {
        if (node_.is_invalid()) {
            return CommandResult::fail("Cannot set transform of an invalid node.");
        }

        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot set transform of a missing node.");
        }

        if (!captured_) {
            previousTransform_ = NodeTransformSnapshot::capture(node->transform());
            captured_ = true;
        }

        return apply_transform(context, nextTransform_.transform, "Node transform changed.");
    }

    CommandResult SetNodeTransformCommand::undo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot undo transform change without a previous value.");
        }

        return apply_transform(context, previousTransform_.transform, "Node transform restored.");
    }

    CommandResult SetNodeTransformCommand::redo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot redo transform change before execution.");
        }

        return apply_transform(context, nextTransform_.transform, "Node transform changed.");
    }

    CommandResult SetNodeTransformCommand::apply_transform(
        CommandContext& context,
        const NodeTransform& transform,
        std::string_view message) {
        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot set transform of a missing node.");
        }

        node->transform() = transform;
        node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

        return CommandResult::ok(
            EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string(message));
    }

}