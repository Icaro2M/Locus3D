/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/transform/NodeTransformSnapshot.h"
#include "editor/scene/SceneNodeId.h"

#include <glm/gtc/quaternion.hpp>

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that rotates a scene node by multiplying its local rotation.
     */
    class RotateNodeCommand final : public ICommand {
    public:
        /**
         * @brief Creates a rotate command.
         *
         * @param id Node to rotate.
         * @param delta Rotation delta quaternion.
         */
        RotateNodeCommand(SceneNodeId id, const glm::quat& delta);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Applies the rotation.
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
         * @brief Reapplies the rotated transform.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        CommandResult apply_transform(
            CommandContext& context,
            const NodeTransformSnapshot& snapshot,
            std::string_view message);

        SceneNodeId node_{};
        glm::quat delta_{ 1.0f, 0.0f, 0.0f, 0.0f };
        NodeTransformSnapshot previousTransform_{};
        NodeTransformSnapshot nextTransform_{};
        bool captured_ = false;
    };

}