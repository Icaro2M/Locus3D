/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/selection/MeshSelectionSnapshot.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that clears mesh component selection.
     */
    class ClearMeshSelectionCommand final : public ICommand {
    public:
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
         * @brief Restores previous mesh component selection.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

    private:
        MeshSelectionSnapshot previousSelection_{};
        bool executed_ = false;
    };

} // namespace locus::editor