/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/scene/NodeMetadata.h"
#include "editor/scene/NodePivot.h"
#include "editor/scene/NodeTransform.h"
#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/mesh/LEM.h"

#include <string>
#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that imports an editable mesh into the current scene.
     *
     * The command creates a new MeshNode containing the provided LEM. Undo removes
     * the imported node, and redo restores the same node id after the first
     * successful execution.
     */
    class ImportMeshCommand final : public ICommand {
    public:
        /**
         * @brief Creates an import command.
         *
         * @param mesh Imported editable mesh.
         * @param name Name assigned to the created mesh node.
         * @param parent Optional parent node. Invalid means root.
         * @param selectImported Whether the imported node should become selected.
         */
        ImportMeshCommand(
            const kernel::geometry::LEM& mesh,
            std::string name = "Imported Mesh",
            SceneNodeId parent = {},
            bool selectImported = true);

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Returns the imported node id.
         *
         * @return Imported node id, or invalid before execution.
         */
        [[nodiscard]] SceneNodeId imported_node() const;

        /**
         * @brief Imports the mesh into the editor scene.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Removes the imported mesh node.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Restores the imported mesh node.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        [[nodiscard]] bool create_imported_node(CommandContext& context);
        [[nodiscard]] bool restore_imported_node(CommandContext& context);
        void capture_imported_node(CommandContext& context);
        void select_imported_node(CommandContext& context);
        void cleanup_selection(CommandContext& context);

        kernel::geometry::LEM mesh_{};
        std::string nodeName_{};
        SceneNodeId parent_{};
        SceneNodeId importedNode_{};
        NodeTransform transform_{};
        NodePivot pivot_{};
        NodeMetadata metadata_{};
        bool selectImported_ = true;
        bool hasExecuted_ = false;
    };

} // namespace locus::editor