/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/mesh/ApplyMeshOperationCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"

#include <utility>

namespace locus::editor {

    ApplyMeshOperationCommand::ApplyMeshOperationCommand(
        SceneNodeId meshNode,
        MeshOperation operation,
        std::string label)
        : meshNode_(meshNode)
        , operation_(std::move(operation))
        , label_(std::move(label))
    {
        if (label_.empty()) {
            label_ = "Apply Mesh Operation";
        }
    }

    std::string_view ApplyMeshOperationCommand::name() const
    {
        return label_;
    }

    CommandResult ApplyMeshOperationCommand::execute(CommandContext& context)
    {
        if (executed_) {
            return redo(context);
        }

        if (meshNode_.is_invalid()) {
            return CommandResult::fail("Cannot apply mesh operation to an invalid node.");
        }

        if (!operation_) {
            return CommandResult::fail("Cannot apply mesh operation without an operation callback.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot apply mesh operation to a missing mesh node.");
        }

        previousSnapshot_.capture(*node, context.selection());

        kernel::geometry::LEMEditor editor(node->mesh());
        const bool success = operation_(editor);

        if (!success) {
            previousSnapshot_.restore(*node, context.selection());
            return CommandResult::fail("Mesh operation failed.");
        }

        context.selection().mesh().set_active_mesh(meshNode_);
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
            "Mesh operation applied.");
    }

    CommandResult ApplyMeshOperationCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot undo mesh operation before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot undo mesh operation because the mesh node is missing.");
        }

        previousSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh operation undone.");
    }

    CommandResult ApplyMeshOperationCommand::redo(CommandContext& context)
    {
        if (!executed_ || !nextSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot redo mesh operation before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot redo mesh operation because the mesh node is missing.");
        }

        nextSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh operation applied.");
    }

    MeshNode* ApplyMeshOperationCommand::find_target(CommandContext& context) const
    {
        return context.scene().find_mesh(meshNode_);
    }

} // namespace locus::editor    