/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/scene/PasteNodesCommand.h"

#include "editor/command/CommandContext.h"
#include "editor/io/SceneFragmentSerializer.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/EmptyNode.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"

#include <memory>
#include <string>
#include <utility>

namespace locus::editor {

    namespace {

        [[nodiscard]] EditorDirtyFlags paste_dirty_flags() noexcept
        {
            return EditorDirtyFlags::Scene
                | EditorDirtyFlags::Selection
                | EditorDirtyFlags::Mesh
                | EditorDirtyFlags::Render
                | EditorDirtyFlags::Picking;
        }

    } // namespace

    PasteNodesCommand::PasteNodesCommand(SceneFragment fragment)
        : fragment_(std::move(fragment))
    {
    }

    std::string_view PasteNodesCommand::name() const
    {
        return "Paste Nodes";
    }

    const std::vector<SceneNodeId>& PasteNodesCommand::pasted_nodes() const
    {
        return pastedNodes_;
    }

    CommandResult PasteNodesCommand::execute(CommandContext& context)
    {
        if (executed_) {
            return redo(context);
        }

        return execute_first(context);
    }

    CommandResult PasteNodesCommand::undo(CommandContext& context)
    {
        if (!executed_ || snapshots_.empty()) {
            return CommandResult::fail(
                "Cannot undo paste before successful execution.");
        }

        remove_created_nodes(context);
        SelectionSerializer::restore(
            previousSelection_,
            context.selection());

        return CommandResult::ok(
            paste_dirty_flags(),
            "Paste undone.");
    }

    CommandResult PasteNodesCommand::redo(CommandContext& context)
    {
        if (!executed_ || snapshots_.empty()) {
            return CommandResult::fail(
                "Cannot redo paste before successful execution.");
        }

        if (!restore_from_snapshots(context)) {
            return CommandResult::fail(
                "Failed to restore pasted nodes.");
        }

        select_pasted_nodes(context);
        mark_pasted_dirty(context);

        return CommandResult::ok(
            paste_dirty_flags(),
            "Nodes pasted.");
    }

    CommandResult PasteNodesCommand::execute_first(CommandContext& context)
    {
        std::string validationMessage{};
        if (!validate_scene_fragment(fragment_, &validationMessage)) {
            return CommandResult::fail(
                validationMessage.empty()
                ? "Cannot paste an invalid scene fragment."
                : validationMessage);
        }

        previousSelection_ =
            SelectionSerializer::capture(context.selection());

        remap_.clear();
        snapshots_.clear();
        pastedNodes_.clear();
        remap_.reserve(fragment_.nodes.size());
        snapshots_.reserve(fragment_.nodes.size());
        pastedNodes_.reserve(fragment_.nodes.size());

        for (const SerializedNode& serialized : fragment_.nodes) {
            SceneNodeId created{};

            switch (serialized.type) {
            case NodeType::Empty:
                created =
                    context.scene().create_empty(serialized.metadata.name);
                break;
            case NodeType::Mesh:
                created =
                    context.scene().create_mesh(serialized.metadata.name);
                break;
            default:
                remove_created_nodes(context);
                return CommandResult::fail(
                    "Cannot paste an unsupported node type.");
            }

            if (created.is_invalid()) {
                remove_created_nodes(context);
                return CommandResult::fail(
                    "Failed to create pasted node.");
            }

            SceneNode* node = context.scene().find_node(created);
            if (!node) {
                remove_created_nodes(context);
                return CommandResult::fail(
                    "Failed to initialize pasted node.");
            }

            node->transform() = serialized.transform;
            node->pivot() = serialized.pivot;
            node->metadata() = serialized.metadata;

            if (serialized.type == NodeType::Mesh) {
                auto* meshNode = dynamic_cast<MeshNode*>(node);
                if (!meshNode || !serialized.mesh.has_value()) {
                    remove_created_nodes(context);
                    return CommandResult::fail(
                        "Failed to initialize pasted mesh node.");
                }

                meshNode->mesh() = serialized.mesh.value();
                meshNode->bump_mesh_revision();
            }

            remap_.emplace(serialized.id, created);
            pastedNodes_.push_back(created);

            NodeSnapshot snapshot{};
            snapshot.id = created;
            snapshot.node = serialized;
            snapshots_.push_back(std::move(snapshot));
        }

        if (!reparent_created_nodes(context)) {
            remove_created_nodes(context);
            snapshots_.clear();
            pastedNodes_.clear();
            remap_.clear();
            return CommandResult::fail(
                "Failed to restore pasted node hierarchy.");
        }

        for (NodeSnapshot& snapshot : snapshots_) {
            const SceneNode* node = context.scene().find_node(snapshot.id);
            snapshot.parent = node ? node->parent() : SceneNodeId{};
        }

        select_pasted_nodes(context);
        mark_pasted_dirty(context);
        executed_ = true;

        return CommandResult::ok(
            paste_dirty_flags(),
            "Nodes pasted.");
    }

    bool PasteNodesCommand::restore_from_snapshots(CommandContext& context)
    {
        for (const NodeSnapshot& snapshot : snapshots_) {
            if (context.scene().find_node(snapshot.id)) {
                return false;
            }
        }

        for (const NodeSnapshot& snapshot : snapshots_) {
            std::unique_ptr<SceneNode> node{};

            switch (snapshot.node.type) {
            case NodeType::Empty:
                node = std::make_unique<EmptyNode>(
                    snapshot.id,
                    snapshot.node.metadata.name);
                break;
            case NodeType::Mesh: {
                auto meshNode = std::make_unique<MeshNode>(
                    snapshot.id,
                    snapshot.node.metadata.name);
                if (snapshot.node.mesh.has_value()) {
                    meshNode->mesh() = snapshot.node.mesh.value();
                    meshNode->bump_mesh_revision();
                }
                node = std::move(meshNode);
                break;
            }
            default:
                return false;
            }

            node->transform() = snapshot.node.transform;
            node->pivot() = snapshot.node.pivot;
            node->metadata() = snapshot.node.metadata;

            if (context.scene().tree().insert_node(std::move(node))
                .is_invalid()) {
                remove_created_nodes(context);
                return false;
            }
        }

        for (const NodeSnapshot& snapshot : snapshots_) {
            if (snapshot.parent.is_valid()
                && !context.scene().reparent(
                    snapshot.id,
                    snapshot.parent)) {
                remove_created_nodes(context);
                return false;
            }
        }

        return true;
    }

    bool PasteNodesCommand::reparent_created_nodes(CommandContext& context)
    {
        for (NodeSnapshot& snapshot : snapshots_) {
            if (!snapshot.node.parentId.has_value()) {
                continue;
            }

            const auto parent = remap_.find(
                snapshot.node.parentId.value());
            if (parent == remap_.end()) {
                return false;
            }

            if (!context.scene().reparent(snapshot.id, parent->second)) {
                return false;
            }
        }

        return true;
    }

    void PasteNodesCommand::remove_created_nodes(CommandContext& context)
    {
        for (auto iterator = pastedNodes_.rbegin();
            iterator != pastedNodes_.rend();
            ++iterator) {
            if (context.scene().find_node(*iterator)) {
                (void)context.scene().remove_node(*iterator);
            }
        }
    }

    void PasteNodesCommand::select_pasted_nodes(CommandContext& context) const
    {
        context.selection().clear();
        context.selection().objects().set(pastedNodes_);
        context.selection().mark_dirty();
    }

    void PasteNodesCommand::mark_pasted_dirty(CommandContext& context) const
    {
        for (const SceneNodeId nodeId : pastedNodes_) {
            if (SceneNode* node = context.scene().find_node(nodeId)) {
                node->mark_dirty(paste_dirty_flags());
            }
        }
    }

} // namespace locus::editor
