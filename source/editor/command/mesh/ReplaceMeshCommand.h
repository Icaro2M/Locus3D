/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/mesh/MeshSnapshot.h"
#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/mesh/LEM.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that replaces the full editable mesh stored in a mesh node.
     *
     * This command is useful for importers, procedural mesh creation, destructive
     * modeling operations, and any editor action that already has a complete
     * replacement LEM.
     */
    class ReplaceMeshCommand final : public ICommand {
    public:
        /**
         * @brief Creates a mesh replacement command.
         *
         * @param meshNode Mesh node to replace.
         * @param mesh Replacement mesh.
         * @param clearComponentSelection Whether component selection should be cleared.
         */
        ReplaceMeshCommand(
            SceneNodeId meshNode,
            const kernel::geometry::LEM& mesh,
            bool clearComponentSelection = true);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Replaces the mesh.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previous mesh.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Restores the replacement mesh after undo.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        [[nodiscard]] MeshNode* find_target(CommandContext& context) const;

        SceneNodeId meshNode_{};
        kernel::geometry::LEM replacementMesh_{};
        MeshSnapshot previousSnapshot_{};
        MeshSnapshot nextSnapshot_{};
        bool clearComponentSelection_ = true;
        bool executed_ = false;
    };

} // namespace locus::editor