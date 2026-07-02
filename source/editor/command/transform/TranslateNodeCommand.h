/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/transform/NodeTransformSnapshot.h"
#include "editor/scene/SceneNodeId.h"

#include <glm/vec3.hpp>

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that translates a scene node in local transform space.
     */
    class TranslateNodeCommand final : public ICommand {
    public:
        /**
         * @brief Creates a translate command.
         *
         * @param id Node to translate.
         * @param delta Translation delta.
         */
        TranslateNodeCommand(SceneNodeId id, const glm::vec3& delta);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Applies the translation.
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
         * @brief Reapplies the translated transform.
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
        glm::vec3 delta_{ 0.0f };
        NodeTransformSnapshot previousTransform_{};
        NodeTransformSnapshot nextTransform_{};
        bool captured_ = false;
    };

}