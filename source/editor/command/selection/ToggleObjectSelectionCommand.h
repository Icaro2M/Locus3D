/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/selection/ObjectSelectionSnapshot.h"
#include "editor/scene/SceneNodeId.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that toggles one scene object in the current object selection.
     */
    class ToggleObjectSelectionCommand final : public ICommand {
    public:
        /**
         * @brief Creates a command.
         *
         * @param id Object to toggle.
         */
        explicit ToggleObjectSelectionCommand(SceneNodeId id);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Executes the command.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores previous object selection.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

    private:
        SceneNodeId object_{};
        ObjectSelectionSnapshot previousSelection_{};
        bool selectedAfterExecute_ = false;
        bool executed_ = false;
    };

} // namespace locus::editor