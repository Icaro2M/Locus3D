/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/scene/NodePivot.h"
#include "editor/scene/SceneNodeId.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that replaces a scene node pivot settings.
     *
     * The pivot is stored in node-local space and is mainly consumed by transform
     * tools, gizmos, snapping, and future pivot-aware manipulation logic.
     */
    class SetNodePivotCommand final : public ICommand {
    public:
        /**
         * @brief Creates a pivot command.
         *
         * @param id Node to update.
         * @param pivot New pivot settings.
         */
        SetNodePivotCommand(SceneNodeId id, const NodePivot& pivot);

        /**
         * @brief Creates a pivot command from an offset.
         *
         * @param id Node to update.
         * @param offset New local pivot offset.
         * @param custom Whether the node should use a custom pivot.
         */
        SetNodePivotCommand(SceneNodeId id, const glm::vec3& offset, bool custom = true);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Applies the new pivot.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previous pivot.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Reapplies the new pivot.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        CommandResult apply_pivot(
            CommandContext& context,
            const NodePivot& pivot,
            std::string_view message);

        SceneNodeId node_{};
        NodePivot previousPivot_{};
        NodePivot nextPivot_{};
        bool captured_ = false;
    };

}