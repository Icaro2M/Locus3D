/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/mesh/ReplaceMeshCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"

namespace locus::editor {

    ReplaceMeshCommand::ReplaceMeshCommand(
        SceneNodeId meshNode,
        const kernel::geometry::LEM& mesh,
        bool clearComponentSelection)
        : meshNode_(meshNode)
        , replacementMesh_(mesh)
        , clearComponentSelection_(clearComponentSelection)
    {
    }

    std::string_view ReplaceMeshCommand::name() const
    {
        return "Replace Mesh";
    }

    CommandResult ReplaceMeshCommand::execute(CommandContext& context)
    {
        if (meshNode_.is_invalid()) {
            return CommandResult::fail("Cannot replace mesh on an invalid node.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot replace mesh on a missing mesh node.");
        }

        previousSnapshot_.capture(*node, context.selection());

        node->mesh() = replacementMesh_;
        context.selection().mesh().set_active_mesh(meshNode_);

        if (clearComponentSelection_) {
            context.selection().mesh().clear_components();
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
            "Mesh replaced.");
    }

    CommandResult ReplaceMeshCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot undo mesh replacement before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot undo mesh replacement because the mesh node is missing.");
        }

        previousSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh replacement undone.");
    }

    CommandResult ReplaceMeshCommand::redo(CommandContext& context)
    {
        if (!executed_ || !nextSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot redo mesh replacement before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot redo mesh replacement because the mesh node is missing.");
        }

        nextSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh replaced.");
    }

    MeshNode* ReplaceMeshCommand::find_target(CommandContext& context) const
    {
        return context.scene().find_mesh(meshNode_);
    }

} // namespace locus::editor