/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/transform/NodeTransformSnapshot.h"
#include "editor/scene/NodeTransform.h"
#include "editor/scene/SceneNodeId.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that replaces a scene node local transform.
     *
     * This is the absolute transform command used as a base for higher-level
     * transform operations such as translate, rotate, scale, gizmo drags, and
     * numeric transform editing.
     */
    class SetNodeTransformCommand final : public ICommand {
    public:
        /**
         * @brief Creates a command.
         *
         * @param id Node to update.
         * @param transform New local transform.
         */
        SetNodeTransformCommand(SceneNodeId id, const NodeTransform& transform);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Applies the new transform.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previous transform.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Reapplies the new transform.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        CommandResult apply_transform(
            CommandContext& context,
            const NodeTransform& transform,
            std::string_view message);

        SceneNodeId node_{};
        NodeTransformSnapshot previousTransform_{};
        NodeTransformSnapshot nextTransform_{};
        bool captured_ = false;
    };

}