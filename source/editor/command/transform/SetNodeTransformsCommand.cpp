/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/transform/SetNodeTransformsCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <string>
#include <utility>

namespace locus::editor {

    SetNodeTransformsCommand::SetNodeTransformsCommand(
        std::vector<NodeTransformChange> changes)
        : changes_(std::move(changes)) {
    }

    std::string_view SetNodeTransformsCommand::name() const {
        return "Set Node Transforms";
    }

    CommandResult SetNodeTransformsCommand::execute(
        CommandContext& context) {

        return apply(
            context,
            ApplySide::Next,
            "Node transforms changed.");
    }

    CommandResult SetNodeTransformsCommand::undo(
        CommandContext& context) {

        return apply(
            context,
            ApplySide::Previous,
            "Node transforms restored.");
    }

    CommandResult SetNodeTransformsCommand::redo(
        CommandContext& context) {

        return apply(
            context,
            ApplySide::Next,
            "Node transforms changed.");
    }

    const std::vector<NodeTransformChange>&
        SetNodeTransformsCommand::changes() const {

        return changes_;
    }

    CommandResult SetNodeTransformsCommand::validate_targets(
        CommandContext& context) const {

        if (changes_.empty()) {
            return CommandResult::fail(
                "Cannot apply an empty node transform batch.");
        }

        for (const NodeTransformChange& change : changes_) {
            if (!change.is_valid()) {
                return CommandResult::fail(
                    "Cannot transform an invalid scene node.");
            }

            if (!context.scene().find_node(change.node)) {
                return CommandResult::fail(
                    "Cannot transform a missing scene node.");
            }
        }

        return CommandResult::ok();
    }

    CommandResult SetNodeTransformsCommand::apply(
        CommandContext& context,
        ApplySide side,
        std::string_view message) {

        const CommandResult validation =
            validate_targets(context);

        if (!validation.success) {
            return validation;
        }

        for (const NodeTransformChange& change : changes_) {
            SceneNode* node =
                context.scene().find_node(change.node);

            const NodeTransformSnapshot& snapshot =
                side == ApplySide::Previous
                ? change.previous
                : change.next;

            snapshot.apply_to(node->transform());

            node->mark_dirty(
                EditorDirtyFlags::Scene |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking);
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            std::string(message));
    }

} // namespace locus::editor