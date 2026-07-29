/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/document/ImportMeshCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/selection/SelectionState.h"

#include <memory>
#include <utility>

namespace locus::editor {

    ImportMeshCommand::ImportMeshCommand(
        const kernel::geometry::LEM& mesh,
        std::string name,
        SceneNodeId parent,
        bool selectImported)
        : mesh_(mesh)
        , nodeName_(std::move(name))
        , parent_(parent)
        , selectImported_(selectImported)
    {
        if (nodeName_.empty()) {
            nodeName_ = "Imported Mesh";
        }

        metadata_.name = nodeName_;
    }

    std::string_view ImportMeshCommand::name() const
    {
        return "Import Mesh";
    }

    SceneNodeId ImportMeshCommand::imported_node() const
    {
        return importedNode_;
    }

    CommandResult ImportMeshCommand::execute(CommandContext& context)
    {
        if (parent_.is_valid() && !context.scene().find_node(parent_)) {
            return CommandResult::fail("Cannot import mesh under a missing parent node.");
        }

        const bool created = hasExecuted_
            ? restore_imported_node(context)
            : create_imported_node(context);

        if (!created) {
            return CommandResult::fail("Failed to import mesh.");
        }

        if (selectImported_) {
            select_imported_node(context);
        }

        if (!hasExecuted_) {
            capture_imported_node(context);
            hasExecuted_ = true;
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking |
            EditorDirtyFlags::Manufacturing,
            "Mesh imported.");
    }

    CommandResult ImportMeshCommand::undo(CommandContext& context)
    {
        if (!hasExecuted_ || importedNode_.is_invalid()) {
            return CommandResult::fail("Cannot undo mesh import before execution.");
        }

        if (!context.scene().find_node(importedNode_)) {
            return CommandResult::fail("Cannot undo mesh import because the imported node is missing.");
        }

        cleanup_selection(context);

        if (!context.scene().remove_node(importedNode_)) {
            return CommandResult::fail("Failed to remove imported mesh node.");
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking |
            EditorDirtyFlags::Manufacturing,
            "Mesh import undone.");
    }

    CommandResult ImportMeshCommand::redo(CommandContext& context)
    {
        if (!hasExecuted_) {
            return CommandResult::fail("Cannot redo mesh import before execution.");
        }

        if (context.scene().find_node(importedNode_)) {
            return CommandResult::fail("Cannot redo mesh import because the imported node already exists.");
        }

        if (!restore_imported_node(context)) {
            return CommandResult::fail("Failed to restore imported mesh node.");
        }

        if (selectImported_) {
            select_imported_node(context);
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking |
            EditorDirtyFlags::Manufacturing,
            "Mesh imported.");
    }

    bool ImportMeshCommand::create_imported_node(CommandContext& context)
    {
        importedNode_ = context.scene().create_mesh(nodeName_);

        if (importedNode_.is_invalid()) {
            return false;
        }

        MeshNode* node = context.scene().find_mesh(importedNode_);
        if (!node) {
            return false;
        }

        node->mesh() = mesh_;
        node->bump_mesh_revision();

        if (parent_.is_valid()) {
            if (!context.scene().reparent(importedNode_, parent_)) {
                context.scene().remove_node(importedNode_);
                importedNode_ = {};
                return false;
            }
        }

        node->mark_dirty(
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking |
            EditorDirtyFlags::Manufacturing);

        return true;
    }

    bool ImportMeshCommand::restore_imported_node(CommandContext& context)
    {
        if (importedNode_.is_invalid()) {
            return false;
        }

        if (parent_.is_valid() && !context.scene().find_node(parent_)) {
            return false;
        }

        auto meshNode = std::make_unique<MeshNode>(importedNode_, metadata_.name);
        meshNode->mesh() = mesh_;
        meshNode->bump_mesh_revision();
        meshNode->transform() = transform_;
        meshNode->pivot() = pivot_;
        meshNode->metadata() = metadata_;

        if (context.scene().tree().insert_node(std::move(meshNode)).is_invalid()) {
            return false;
        }

        if (parent_.is_valid()) {
            if (!context.scene().reparent(importedNode_, parent_)) {
                context.scene().remove_node(importedNode_);
                return false;
            }
        }

        if (SceneNode* node = context.scene().find_node(importedNode_)) {
            node->mark_dirty(
                EditorDirtyFlags::Scene |
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking |
                EditorDirtyFlags::Manufacturing);
        }

        return true;
    }

    void ImportMeshCommand::capture_imported_node(CommandContext& context)
    {
        const MeshNode* node = context.scene().find_mesh(importedNode_);
        if (!node) {
            return;
        }

        mesh_ = node->mesh();
        transform_ = node->transform();
        pivot_ = node->pivot();
        metadata_ = node->metadata();
    }

    void ImportMeshCommand::select_imported_node(CommandContext& context)
    {
        context.selection().objects().set(importedNode_);
        context.selection().mesh().set_active_mesh(importedNode_);
        context.selection().set_granularity(SelectionGranularity::Object);
        context.selection().set_scope(SelectionScope::Scene);
        context.selection().mark_dirty();
    }

    void ImportMeshCommand::cleanup_selection(CommandContext& context)
    {
        SelectionState& selection = context.selection();

        bool changed = false;

        if (selection.objects().contains(importedNode_)) {
            changed = selection.objects().remove(importedNode_) || changed;
        }

        if (selection.objects().active() == importedNode_) {
            selection.objects().set_active({});
            changed = true;
        }

        if (selection.objects().hovered() == importedNode_) {
            selection.objects().set_hovered({});
            changed = true;
        }

        if (selection.mesh().active_mesh() == importedNode_) {
            selection.mesh().clear();
            changed = true;
        }

        if (changed) {
            selection.mark_dirty();
        }
    }

} // namespace locus::editor
