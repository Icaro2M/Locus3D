/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/selection/MeshSelectionSnapshot.h"
#include "editor/command/selection/ObjectSelectionSnapshot.h"
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
     * @brief Command that clears the whole editor scene.
     *
     * The command snapshots all scene nodes and selection state before clearing the
     * document, allowing undo to restore the previous hierarchy with stable ids.
     */
    class ClearSceneCommand final : public ICommand {
    public:
        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Clears all scene nodes and selection state.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previously cleared scene.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Clears the scene again after undo.
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

        void capture_scene(CommandContext& context);
        void capture_node_recursive(CommandContext& context, SceneNodeId id);
        [[nodiscard]] bool restore_scene(CommandContext& context) const;
        void mark_restored_scene_dirty(CommandContext& context) const;

        std::vector<NodeSnapshot> snapshots_{};
        ObjectSelectionSnapshot objectSelection_{};
        MeshSelectionSnapshot meshSelection_{};
        bool captured_ = false;
    };

} // namespace locus::editor