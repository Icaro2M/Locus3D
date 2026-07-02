/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/DuplicateNodeCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/EmptyNode.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"

#include <memory>
#include <string>
#include <utility>

namespace locus::editor {

    namespace {

        [[nodiscard]] std::string duplicate_name(const std::string& name, bool isRoot) {
            if (!isRoot) {
                return name;
            }

            if (name.empty()) {
                return "Copy";
            }

            return name + " Copy";
        }

    }

    DuplicateNodeCommand::DuplicateNodeCommand(SceneNodeId node)
        : sourceNode_(node) {
    }

    std::string_view DuplicateNodeCommand::name() const {
        return "Duplicate Node";
    }

    SceneNodeId DuplicateNodeCommand::duplicated_node() const {
        return duplicatedNode_;
    }

    CommandResult DuplicateNodeCommand::execute(CommandContext& context) {
        if (sourceNode_.is_invalid()) {
            return CommandResult::fail("Cannot duplicate an invalid node.");
        }

        const SceneNode* source = context.scene().find_node(sourceNode_);
        if (!source) {
            return CommandResult::fail("Cannot duplicate a missing node.");
        }

        if (hasExecuted_) {
            return redo(context);
        }

        snapshots_.clear();

        duplicatedNode_ = duplicate_node_recursive(
            context,
            sourceNode_,
            source->parent(),
            true);

        if (duplicatedNode_.is_invalid()) {
            snapshots_.clear();
            return CommandResult::fail("Failed to duplicate node subtree.");
        }

        hasExecuted_ = true;
        mark_duplicate_dirty(context);

        return CommandResult::ok(
            EditorDirtyFlags::Scene
            | EditorDirtyFlags::Mesh
            | EditorDirtyFlags::Render
            | EditorDirtyFlags::Picking,
            "Node duplicated.");
    }

    CommandResult DuplicateNodeCommand::undo(CommandContext& context) {
        if (!hasExecuted_ || duplicatedNode_.is_invalid()) {
            return CommandResult::fail("Cannot undo node duplication before execution.");
        }

        if (!context.scene().find_node(duplicatedNode_)) {
            return CommandResult::fail("Cannot undo node duplication because the duplicate is missing.");
        }

        if (!context.scene().remove_node(duplicatedNode_)) {
            return CommandResult::fail("Failed to remove duplicated node.");
        }

        return CommandResult::ok(
            EditorDirtyFlags::Scene
            | EditorDirtyFlags::Selection
            | EditorDirtyFlags::Mesh
            | EditorDirtyFlags::Render
            | EditorDirtyFlags::Picking,
            "Node duplication undone.");
    }

    CommandResult DuplicateNodeCommand::redo(CommandContext& context) {
        if (!hasExecuted_ || duplicatedNode_.is_invalid() || snapshots_.empty()) {
            return CommandResult::fail("Cannot redo node duplication before execution.");
        }

        if (context.scene().find_node(duplicatedNode_)) {
            return CommandResult::fail("Cannot redo node duplication because the duplicate already exists.");
        }

        if (!restore_duplicate_subtree(context)) {
            return CommandResult::fail("Failed to restore duplicated node subtree.");
        }

        mark_duplicate_dirty(context);

        return CommandResult::ok(
            EditorDirtyFlags::Scene
            | EditorDirtyFlags::Mesh
            | EditorDirtyFlags::Render
            | EditorDirtyFlags::Picking,
            "Node duplicated.");
    }

    SceneNodeId DuplicateNodeCommand::duplicate_node_recursive(
        CommandContext& context,
        SceneNodeId source,
        SceneNodeId duplicatedParent,
        bool isRoot) {
        const SceneNode* sourceNode = context.scene().find_node(source);
        if (!sourceNode) {
            return {};
        }

        const std::string name = duplicate_name(sourceNode->metadata().name, isRoot);

        SceneNodeId duplicated{};
        switch (sourceNode->type()) {
        case NodeType::Empty:
            duplicated = context.scene().create_empty(name);
            break;

        case NodeType::Mesh:
            duplicated = context.scene().create_mesh(name);
            break;

        default:
            return {};
        }

        if (duplicated.is_invalid()) {
            return {};
        }

        SceneNode* duplicatedNode = context.scene().find_node(duplicated);
        if (!duplicatedNode) {
            return {};
        }

        duplicatedNode->transform() = sourceNode->transform();
        duplicatedNode->pivot() = sourceNode->pivot();
        duplicatedNode->metadata() = sourceNode->metadata();
        duplicatedNode->metadata().name = name;

        if (sourceNode->type() == NodeType::Mesh) {
            const auto* sourceMesh = dynamic_cast<const MeshNode*>(sourceNode);
            auto* duplicatedMesh = dynamic_cast<MeshNode*>(duplicatedNode);

            if (!sourceMesh || !duplicatedMesh) {
                return {};
            }

            duplicatedMesh->mesh() = sourceMesh->mesh();
        }

        if (duplicatedParent.is_valid()) {
            if (!context.scene().reparent(duplicated, duplicatedParent)) {
                return {};
            }
        }

        capture_duplicated_node(context, duplicated);

        for (SceneNodeId child : sourceNode->children()) {
            const SceneNodeId duplicatedChild = duplicate_node_recursive(
                context,
                child,
                duplicated,
                false);

            if (duplicatedChild.is_invalid()) {
                return {};
            }
        }

        return duplicated;
    }

    void DuplicateNodeCommand::capture_duplicated_node(CommandContext& context, SceneNodeId duplicated) {
        const SceneNode* node = context.scene().find_node(duplicated);
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
    }

    bool DuplicateNodeCommand::restore_duplicate_subtree(CommandContext& context) {
        for (const NodeSnapshot& snapshot : snapshots_) {
            if (context.scene().find_node(snapshot.id)) {
                return false;
            }

            if (snapshot.parent.is_valid()
                && !context.scene().find_node(snapshot.parent)) {
                bool parentWillBeRestored = false;

                for (const NodeSnapshot& candidate : snapshots_) {
                    if (candidate.id == snapshot.parent) {
                        parentWillBeRestored = true;
                        break;
                    }
                }

                if (!parentWillBeRestored) {
                    return false;
                }
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

    void DuplicateNodeCommand::mark_duplicate_dirty(CommandContext& context) {
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