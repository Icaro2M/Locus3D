/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/ReparentNodeCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>

namespace locus::editor {

    ReparentNodeCommand::ReparentNodeCommand(SceneNodeId child, SceneNodeId newParent)
        : child_(child)
        , newParent_(newParent) {
    }

    std::string_view ReparentNodeCommand::name() const {
        return "Reparent Node";
    }

    CommandResult ReparentNodeCommand::execute(CommandContext& context) {
        if (child_.is_invalid()) {
            return CommandResult::fail("Cannot reparent an invalid node.");
        }

        SceneNode* child = context.scene().find_node(child_);
        if (!child) {
            return CommandResult::fail("Cannot reparent a missing node.");
        }

        if (newParent_ == child_) {
            return CommandResult::fail("Cannot parent a node to itself.");
        }

        if (newParent_.is_valid() && !context.scene().find_node(newParent_)) {
            return CommandResult::fail("Cannot reparent a node to a missing parent.");
        }

        if (newParent_.is_valid() && context.scene().tree().is_ancestor(child_, newParent_)) {
            return CommandResult::fail("Cannot reparent a node to one of its descendants.");
        }

        if (!captured_) {
            previousParent_ = child->parent();
            captured_ = true;
        }

        if (previousParent_ == newParent_) {
            return CommandResult::ok(EditorDirtyFlags::None, "Node already has the requested parent.");
        }

        return apply_parent(context, newParent_, "Node reparented.");
    }

    CommandResult ReparentNodeCommand::undo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot undo node reparent before execution.");
        }

        return apply_parent(context, previousParent_, "Node parent restored.");
    }

    CommandResult ReparentNodeCommand::redo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot redo node reparent before execution.");
        }

        return apply_parent(context, newParent_, "Node reparented.");
    }

    CommandResult ReparentNodeCommand::apply_parent(
        CommandContext& context,
        SceneNodeId parent,
        std::string_view message) {
        if (child_.is_invalid()) {
            return CommandResult::fail("Cannot reparent an invalid node.");
        }

        if (!context.scene().find_node(child_)) {
            return CommandResult::fail("Cannot reparent a missing node.");
        }

        if (parent == child_) {
            return CommandResult::fail("Cannot parent a node to itself.");
        }

        if (parent.is_valid() && !context.scene().find_node(parent)) {
            return CommandResult::fail("Cannot reparent a node to a missing parent.");
        }

        if (parent.is_valid() && context.scene().tree().is_ancestor(child_, parent)) {
            return CommandResult::fail("Cannot reparent a node to one of its descendants.");
        }

        if (!context.scene().reparent(child_, parent)) {
            return CommandResult::fail("Failed to reparent node.");
        }

        if (SceneNode* child = context.scene().find_node(child_)) {
            child->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);
        }

        if (SceneNode* parentNode = context.scene().find_node(parent)) {
            parentNode->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);
        }

        if (SceneNode* previousParent = context.scene().find_node(previousParent_)) {
            previousParent->mark_dirty(EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking);
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string(message));
    }

}