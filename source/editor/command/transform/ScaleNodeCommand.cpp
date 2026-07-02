/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/transform/ScaleNodeCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>

namespace locus::editor {

    ScaleNodeCommand::ScaleNodeCommand(SceneNodeId id, const glm::vec3& factor)
        : node_(id)
        , factor_(factor) {
    }

    std::string_view ScaleNodeCommand::name() const {
        return "Scale Node";
    }

    CommandResult ScaleNodeCommand::execute(CommandContext& context) {
        if (node_.is_invalid()) {
            return CommandResult::fail("Cannot scale an invalid node.");
        }

        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot scale a missing node.");
        }

        if (!captured_) {
            previousTransform_ = NodeTransformSnapshot::capture(node->transform());

            NodeTransform next = node->transform();
            next.set_scale(next.scale() * factor_);
            nextTransform_ = NodeTransformSnapshot::capture(next);

            captured_ = true;
        }

        return apply_transform(context, nextTransform_, "Node scaled.");
    }

    CommandResult ScaleNodeCommand::undo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot undo node scale before execution.");
        }

        return apply_transform(context, previousTransform_, "Node scale undone.");
    }

    CommandResult ScaleNodeCommand::redo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot redo node scale before execution.");
        }

        return apply_transform(context, nextTransform_, "Node scaled.");
    }

    CommandResult ScaleNodeCommand::apply_transform(
        CommandContext& context,
        const NodeTransformSnapshot& snapshot,
        std::string_view message) {
        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot scale a missing node.");
        }

        snapshot.apply_to(node->transform());
        node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

        return CommandResult::ok(
            EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string(message));
    }

}