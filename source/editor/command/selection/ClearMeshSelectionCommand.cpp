/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/ClearMeshSelectionCommand.h"

#include "editor/selection/SelectionController.h"

namespace locus::editor {

    std::string_view ClearMeshSelectionCommand::name() const
    {
        return "Clear Mesh Selection";
    }

    CommandResult ClearMeshSelectionCommand::execute(CommandContext& context)
    {
        previousSelection_.capture(context.selection());

        context.selection_controller().clear_mesh_components();
        executed_ = true;

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            "Mesh component selection cleared.");
    }

    CommandResult ClearMeshSelectionCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSelection_.is_valid()) {
            return CommandResult::fail("Cannot undo clear mesh selection without a previous selection snapshot.");
        }

        previousSelection_.restore(context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            "Mesh component selection restored.");
    }

} // namespace locus::editor