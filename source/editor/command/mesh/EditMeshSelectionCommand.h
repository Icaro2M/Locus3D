/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/mesh/MeshSnapshot.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionState.h"
#include "kernel/geometry/mesh/LEM.h"

#include <functional>
#include <string>
#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that edits mesh component selection state.
     *
     * This command is intended for mesh-level selection edits that may need to stay
     * synchronized with the active mesh after topology operations. It snapshots the
     * mesh too because some selection edits may update selected flags stored inside
     * LEM elements.
     */
    class EditMeshSelectionCommand final : public ICommand {
    public:
        /**
         * @brief Selection edit callback.
         *
         * @param mesh Active mesh.
         * @param selection Editor selection state.
         * @return True when the edit succeeded.
         */
        using MeshSelectionEdit = std::function<bool(
            kernel::geometry::LEM& mesh,
            SelectionState& selection)>;

        /**
         * @brief Creates a mesh selection edit command.
         *
         * @param meshNode Mesh node whose selection is edited.
         * @param edit Selection edit callback.
         * @param label Human-readable command label.
         */
        EditMeshSelectionCommand(
            SceneNodeId meshNode,
            MeshSelectionEdit edit,
            std::string label = "Edit Mesh Selection");

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Applies the selection edit.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previous selection state.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Restores the edited selection state.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        [[nodiscard]] MeshNode* find_target(CommandContext& context) const;

        SceneNodeId meshNode_{};
        MeshSelectionEdit edit_{};
        std::string label_{};
        MeshSnapshot previousSnapshot_{};
        MeshSnapshot nextSnapshot_{};
        bool executed_ = false;
    };

} // namespace locus::editor