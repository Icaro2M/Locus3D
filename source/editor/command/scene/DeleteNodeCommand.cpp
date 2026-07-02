/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/DeleteNodeCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/EmptyNode.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionState.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace locus::editor {

    DeleteNodeCommand::DeleteNodeCommand(SceneNodeId node)
        : node_(node) {
    }

    std::string_view DeleteNodeCommand::name() const {
        return "Delete Node";
    }

    CommandResult DeleteNodeCommand::execute(CommandContext& context) {
        if (node_.is_invalid()) {
            return CommandResult::fail("Cannot delete an invalid node.");
        }

        if (!context.scene().find_node(node_)) {
            return CommandResult::fail("Cannot delete a missing node.");
        }

        if (!captured_ && !capture_subtree(context)) {
            return CommandResult::fail("Failed to capture node subtree before deletion.");
        }

        cleanup_selection(context);

        if (!context.scene().remove_node(node_)) {
            return CommandResult::fail("Failed to delete node.");
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene
            | EditorDirtyFlags::Selection
            | EditorDirtyFlags::Mesh
            | EditorDirtyFlags::Render
            | EditorDirtyFlags::Picking,
            "Node deleted.");
    }

    CommandResult DeleteNodeCommand::undo(CommandContext& context) {
        if (!captured_ || snapshots_.empty()) {
            return CommandResult::fail("Cannot undo node deletion without a captured subtree.");
        }

        if (context.scene().find_node(node_)) {
            return CommandResult::fail("Cannot restore deleted node because its id already exists.");
        }

        if (!restore_subtree(context)) {
            return CommandResult::fail("Failed to restore deleted node subtree.");
        }

        mark_restored_subtree_dirty(context);

        return CommandResult::ok(
            EditorDirtyFlags::Scene
            | EditorDirtyFlags::Selection
            | EditorDirtyFlags::Mesh
            | EditorDirtyFlags::Render
            | EditorDirtyFlags::Picking,
            "Node deletion undone.");
    }

    CommandResult DeleteNodeCommand::redo(CommandContext& context) {
        if (!captured_) {
            return CommandResult::fail("Cannot redo node deletion before execution.");
        }

        if (!context.scene().find_node(node_)) {
            return CommandResult::fail("Cannot redo node deletion because the node is missing.");
        }

        cleanup_selection(context);

        if (!context.scene().remove_node(node_)) {
            return CommandResult::fail("Failed to delete node.");
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene
            | EditorDirtyFlags::Selection
            | EditorDirtyFlags::Mesh
            | EditorDirtyFlags::Render
            | EditorDirtyFlags::Picking,
            "Node deleted.");
    }

    bool DeleteNodeCommand::capture_subtree(CommandContext& context) {
        snapshots_.clear();
        capture_node_recursive(context, node_);
        captured_ = !snapshots_.empty();
        return captured_;
    }

    void DeleteNodeCommand::capture_node_recursive(CommandContext& context, SceneNodeId id) {
        const SceneNode* node = context.scene().find_node(id);
        if (!node) {
            return;
        }

        NodeSnapshot snapshot{};
        snapshot.id = node->id();
        snapshot.parent = node->parent();
        snapshot.type = node->type();
        snapshot.transform = node->transform();
        snapshot.pivot = node->pivot();
        snapshot.metadata = node->metadata();

        if (const auto* meshNode = dynamic_cast<const MeshNode*>(node)) {
            snapshot.mesh = meshNode->mesh();
        }

        snapshots_.push_back(std::move(snapshot));

        for (SceneNodeId child : node->children()) {
            capture_node_recursive(context, child);
        }
    }

    bool DeleteNodeCommand::contains_snapshot(SceneNodeId id) const {
        if (id.is_invalid()) {
            return false;
        }

        return std::any_of(
            snapshots_.begin(),
            snapshots_.end(),
            [id](const NodeSnapshot& snapshot) {
                return snapshot.id == id;
            });
    }

    bool DeleteNodeCommand::restore_subtree(CommandContext& context) {
        for (const NodeSnapshot& snapshot : snapshots_) {
            if (context.scene().find_node(snapshot.id)) {
                return false;
            }
        }

        for (const NodeSnapshot& snapshot : snapshots_) {
            std::unique_ptr<SceneNode> node;

            switch (snapshot.type) {
            case NodeType::Empty:
                node = std::make_unique<EmptyNode>(snapshot.id, snapshot.metadata.name);
                break;

            case NodeType::Mesh: {
                auto meshNode = std::make_unique<MeshNode>(snapshot.id, snapshot.metadata.name);
                if (snapshot.mesh.has_value()) {
                    meshNode->mesh() = snapshot.mesh.value();
                }
                node = std::move(meshNode);
                break;
            }

            default:
                return false;
            }

            node->transform() = snapshot.transform;
            node->pivot() = snapshot.pivot;
            node->metadata() = snapshot.metadata;

            if (context.scene().tree().insert_node(std::move(node)).is_invalid()) {
                return false;
            }
        }

        for (const NodeSnapshot& snapshot : snapshots_) {
            if (snapshot.parent.is_valid()) {
                if (!context.scene().reparent(snapshot.id, snapshot.parent)) {
                    return false;
                }
            }
        }

        return true;
    }

    void DeleteNodeCommand::cleanup_selection(CommandContext& context) {
        auto& selection = context.selection();

        const SceneNodeId activeObject = selection.objects().active();
        const SceneNodeId hoveredObject = selection.objects().hovered();
        const SceneNodeId activeMesh = selection.mesh().active_mesh();

        const bool deletedActiveObject = contains_snapshot(activeObject);
        const bool deletedHoveredObject = contains_snapshot(hoveredObject);
        const bool deletedActiveMesh = contains_snapshot(activeMesh);

        std::vector<SceneNodeId> selected = selection.objects().selected();
        bool changed = false;

        for (SceneNodeId id : selected) {
            if (contains_snapshot(id)) {
                changed = selection.objects().remove(id) || changed;
            }
        }

        if (deletedActiveObject) {
            selection.objects().set_active({});
            changed = true;
        }

        if (deletedHoveredObject) {
            selection.objects().set_hovered({});
            changed = true;
        }

        if (deletedActiveMesh) {
            selection.mesh().clear();
            changed = true;
        }

        if (changed) {
            selection.mark_dirty();
        }
    }

    void DeleteNodeCommand::mark_restored_subtree_dirty(CommandContext& context) {
        for (const NodeSnapshot& snapshot : snapshots_) {
            if (SceneNode* node = context.scene().find_node(snapshot.id)) {
                node->mark_dirty(
                    EditorDirtyFlags::Scene
                    | EditorDirtyFlags::Mesh
                    | EditorDirtyFlags::Render
                    | EditorDirtyFlags::Picking);
            }
        }
    }

}