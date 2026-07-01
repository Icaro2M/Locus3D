/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/ToggleObjectSelectionCommand.h"

#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionController.h"

namespace locus::editor {

    ToggleObjectSelectionCommand::ToggleObjectSelectionCommand(SceneNodeId id)
        : object_(id)
    {
    }

    std::string_view ToggleObjectSelectionCommand::name() const
    {
        return "Toggle Object Selection";
    }

    CommandResult ToggleObjectSelectionCommand::execute(CommandContext& context)
    {
        if (object_.is_invalid()) {
            return CommandResult::fail("Cannot toggle an invalid object.");
        }

        const SceneNode* node = context.scene().find_node(object_);
        if (!node) {
            return CommandResult::fail("Cannot toggle an object that does not exist.");
        }

        if (!node->is_selectable()) {
            return CommandResult::fail("Cannot toggle an object that is not selectable.");
        }

        previousSelection_.capture(context.selection());

        selectedAfterExecute_ = context.selection_controller().toggle_object(object_);
        executed_ = true;

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            selectedAfterExecute_ ? "Object added to selection." : "Object removed from selection.");
    }

    CommandResult ToggleObjectSelectionCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSelection_.is_valid()) {
            return CommandResult::fail("Cannot undo toggle object selection without a previous selection snapshot.");
        }

        previousSelection_.restore(context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            "Object selection restored.");
    }

} // namespace locus::editor