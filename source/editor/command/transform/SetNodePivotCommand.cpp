/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/transform/SetNodePivotCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>

namespace locus::editor {

    SetNodePivotCommand::SetNodePivotCommand(SceneNodeId id, const NodePivot& pivot)
        : node_(id)
        , nextPivot_(pivot) {
    }

    SetNodePivotCommand::SetNodePivotCommand(SceneNodeId id, const glm::vec3& offset, bool custom)
        : node_(id) {
        nextPivot_.offset = offset;
        nextPivot_.custom = custom;
    }

    std::string_view SetNodePivotCommand::name() const {
        return "Set Node Pivot";
    }

    CommandResult SetNodePivotCommand::execute(CommandContext& context) {
        if (node_.is_invalid()) {
            return CommandResult::fail("Cannot set pivot of an invalid node.");
        }

        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot set pivot of a missing node.");
        }

        if (!captured_) {
            previousPivot_ = node->pivot();
            captured_ = true;
        }

        return apply_pivot(context, nextPivot_, "Node pivot changed.");
    }

    CommandResult SetNodePivotCommand::undo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot undo pivot change before execution.");
        }

        return apply_pivot(context, previousPivot_, "Node pivot restored.");
    }

    CommandResult SetNodePivotCommand::redo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot redo pivot change before execution.");
        }

        return apply_pivot(context, nextPivot_, "Node pivot changed.");
    }

    CommandResult SetNodePivotCommand::apply_pivot(
        CommandContext& context,
        const NodePivot& pivot,
        std::string_view message) {
        SceneNode* node = context.scene().find_node(node_);
        if (!node) {
            return CommandResult::fail("Cannot set pivot of a missing node.");
        }

        node->pivot() = pivot;
        node->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);

        return CommandResult::ok(
            EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string(message));
    }

}