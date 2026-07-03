/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/mesh/EditMeshSelectionCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"

#include <utility>

namespace locus::editor {

    EditMeshSelectionCommand::EditMeshSelectionCommand(
        SceneNodeId meshNode,
        MeshSelectionEdit edit,
        std::string label)
        : meshNode_(meshNode)
        , edit_(std::move(edit))
        , label_(std::move(label))
    {
        if (label_.empty()) {
            label_ = "Edit Mesh Selection";
        }
    }

    std::string_view EditMeshSelectionCommand::name() const
    {
        return label_;
    }

    CommandResult EditMeshSelectionCommand::execute(CommandContext& context)
    {
        if (executed_) {
            return redo(context);
        }

        if (meshNode_.is_invalid()) {
            return CommandResult::fail("Cannot edit mesh selection on an invalid node.");
        }

        if (!edit_) {
            return CommandResult::fail("Cannot edit mesh selection without an edit callback.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot edit mesh selection on a missing mesh node.");
        }

        previousSnapshot_.capture(*node, context.selection());

        context.selection().mesh().set_active_mesh(meshNode_);

        const bool success = edit_(node->mesh(), context.selection());
        if (!success) {
            previousSnapshot_.restore(*node, context.selection());
            return CommandResult::fail("Mesh selection edit failed.");
        }

        context.selection().mark_dirty();

        node->mark_dirty(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking);

        nextSnapshot_.capture(*node, context.selection());
        executed_ = true;

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh selection edited.");
    }

    CommandResult EditMeshSelectionCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot undo mesh selection edit before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot undo mesh selection edit because the mesh node is missing.");
        }

        previousSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh selection edit undone.");
    }

    CommandResult EditMeshSelectionCommand::redo(CommandContext& context)
    {
        if (!executed_ || !nextSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot redo mesh selection edit before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot redo mesh selection edit because the mesh node is missing.");
        }

        nextSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh selection edited.");
    }

    MeshNode* EditMeshSelectionCommand::find_target(CommandContext& context) const
    {
        return context.scene().find_mesh(meshNode_);
    }

} // namespace locus::editor