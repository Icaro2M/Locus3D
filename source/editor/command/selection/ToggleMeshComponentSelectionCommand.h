/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/selection/MeshSelectionSnapshot.h"
#include "editor/selection/SelectionGranularity.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <string_view>

namespace locus::editor {

    /**
     * @brief Command that toggles one component in the active mesh selection.
     */
    class ToggleMeshComponentSelectionCommand final : public ICommand {
    public:
        /**
         * @brief Creates a vertex toggle command.
         *
         * @param handle Vertex handle.
         */
        explicit ToggleMeshComponentSelectionCommand(kernel::geometry::VertexHandle handle);

        /**
         * @brief Creates an edge toggle command.
         *
         * @param handle Edge handle.
         */
        explicit ToggleMeshComponentSelectionCommand(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Creates a loop toggle command.
         *
         * @param handle Loop handle.
         */
        explicit ToggleMeshComponentSelectionCommand(kernel::geometry::LoopHandle handle);

        /**
         * @brief Creates a face toggle command.
         *
         * @param handle Face handle.
         */
        explicit ToggleMeshComponentSelectionCommand(kernel::geometry::FaceHandle handle);

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
        [[nodiscard]] bool has_valid_handle() const;
        [[nodiscard]] CommandResult toggle_component(CommandContext& context);
        [[nodiscard]] const char* component_name() const;

        SelectionGranularity component_ = SelectionGranularity::Vertex;

        kernel::geometry::VertexHandle vertex_{};
        kernel::geometry::EdgeHandle edge_{};
        kernel::geometry::LoopHandle loop_{};
        kernel::geometry::FaceHandle face_{};

        MeshSelectionSnapshot previousSelection_{};
        bool selectedAfterExecute_ = false;
        bool executed_ = false;
    };

} // namespace locus::editor