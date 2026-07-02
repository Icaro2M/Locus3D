/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/scene/NodeMetadata.h"
#include "editor/scene/NodePivot.h"
#include "editor/scene/NodeTransform.h"
#include "editor/scene/NodeType.h"
#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/mesh/LEM.h"

#include <optional>
#include <string_view>
#include <vector>

namespace locus::editor {

    /**
     * @brief Command that deletes a scene node and all of its descendants.
     *
     * The command stores a snapshot of the removed subtree so undo can restore
     * the same node ids, hierarchy, metadata, transforms, pivots, and mesh data.
     */
    class DeleteNodeCommand final : public ICommand {
    public:
        /**
         * @brief Creates a delete command.
         *
         * @param node Node to delete.
         */
        explicit DeleteNodeCommand(SceneNodeId node);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Deletes the node subtree.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the deleted node subtree.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Deletes the restored node subtree again.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        struct NodeSnapshot {
            SceneNodeId id{};
            SceneNodeId parent{};
            NodeType type = NodeType::Empty;
            NodeTransform transform{};
            NodePivot pivot{};
            NodeMetadata metadata{};
            std::optional<kernel::geometry::LEM> mesh{};
        };

        [[nodiscard]] bool capture_subtree(CommandContext& context);
        void capture_node_recursive(CommandContext& context, SceneNodeId id);
        [[nodiscard]] bool contains_snapshot(SceneNodeId id) const;
        [[nodiscard]] bool restore_subtree(CommandContext& context);
        void cleanup_selection(CommandContext& context);
        void mark_restored_subtree_dirty(CommandContext& context);

        SceneNodeId node_{};
        std::vector<NodeSnapshot> snapshots_{};
        bool captured_ = false;
    };

}