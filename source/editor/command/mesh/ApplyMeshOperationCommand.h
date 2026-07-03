/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/mesh/MeshSnapshot.h"
#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <functional>
#include <string>
#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that applies a generic LEMEditor operation to a mesh node.
     *
     * The operation is executed only on the first execute call. Redo restores the
     * already captured post-operation mesh snapshot, making redo deterministic even
     * for operations that allocate handles or depend on current topology.
     */
    class ApplyMeshOperationCommand final : public ICommand {
    public:
        /**
         * @brief Operation callback executed against a LEMEditor facade.
         *
         * @return True when the operation succeeded.
         */
        using MeshOperation = std::function<bool(kernel::geometry::LEMEditor&)>;

        /**
         * @brief Creates a mesh operation command.
         *
         * @param meshNode Mesh node to edit.
         * @param operation Operation callback.
         * @param label Human-readable operation label.
         */
        ApplyMeshOperationCommand(
            SceneNodeId meshNode,
            MeshOperation operation,
            std::string label = "Apply Mesh Operation");

        /**
         * @brief Returns the command name.
         *
         * @return Command name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Applies the mesh operation.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult execute(CommandContext& context) override;

        /**
         * @brief Restores the previous mesh state.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult undo(CommandContext& context) override;

        /**
         * @brief Restores the post-operation mesh state.
         *
         * @param context Command context.
         * @return Command result.
         */
        CommandResult redo(CommandContext& context) override;

    private:
        [[nodiscard]] MeshNode* find_target(CommandContext& context) const;

        SceneNodeId meshNode_{};
        MeshOperation operation_{};
        std::string label_{};
        MeshSnapshot previousSnapshot_{};
        MeshSnapshot nextSnapshot_{};
        bool executed_ = false;
    };

} // namespace locus::editor