/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/transform/RotateNodeCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <glm/geometric.hpp>

#include <string>

namespace locus::editor {

    RotateNodeCommand::RotateNodeCommand(SceneNodeId id, const glm::quat& delta)
        : node_(id)
        , delta_(delta) {
    }

    std::string_view RotateNodeCommand::name() const {
        return "Rotate Node";
    }

    CommandResult RotateNodeCommand::execute(CommandContext& context) {
        if (node_.is_invalid()) {
            return CommandResult::fail("Cannot rotate an invalid node.");
        }

        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot rotate a missing node.");
        }

        if (!captured_) {
            previousTransform_ = NodeTransformSnapshot::capture(node->transform());

            NodeTransform next = node->transform();
            next.set_rotation(glm::normalize(delta_ * next.rotation()));
            nextTransform_ = NodeTransformSnapshot::capture(next);

            captured_ = true;
        }

        return apply_transform(context, nextTransform_, "Node rotated.");
    }

    CommandResult RotateNodeCommand::undo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot undo node rotation before execution.");
        }

        return apply_transform(context, previousTransform_, "Node rotation undone.");
    }

    CommandResult RotateNodeCommand::redo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot redo node rotation before execution.");
        }

        return apply_transform(context, nextTransform_, "Node rotated.");
    }

    CommandResult RotateNodeCommand::apply_transform(
        CommandContext& context,
        const NodeTransformSnapshot& snapshot,
        std::string_view message) {
        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot rotate a missing node.");
        }

        snapshot.apply_to(node->transform());
        node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

        return CommandResult::ok(
            EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string(message));
    }

}