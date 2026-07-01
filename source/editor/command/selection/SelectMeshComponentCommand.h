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
     * @brief Command that selects one component in the active mesh.
     */
    class SelectMeshComponentCommand final : public ICommand {
    public:
        /**
         * @brief Creates a vertex selection command.
         *
         * @param handle Vertex handle.
         */
        explicit SelectMeshComponentCommand(kernel::geometry::VertexHandle handle);

        /**
         * @brief Creates an edge selection command.
         *
         * @param handle Edge handle.
         */
        explicit SelectMeshComponentCommand(kernel::geometry::EdgeHandle handle);

        /**
         * @brief Creates a loop selection command.
         *
         * @param handle Loop handle.
         */
        explicit SelectMeshComponentCommand(kernel::geometry::LoopHandle handle);

        /**
         * @brief Creates a face selection command.
         *
         * @param handle Face handle.
         */
        explicit SelectMeshComponentCommand(kernel::geometry::FaceHandle handle);

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
        [[nodiscard]] CommandResult select_component(CommandContext& context);
        [[nodiscard]] const char* component_name() const;

        SelectionGranularity component_ = SelectionGranularity::Vertex;

        kernel::geometry::VertexHandle vertex_{};
        kernel::geometry::EdgeHandle edge_{};
        kernel::geometry::LoopHandle loop_{};
        kernel::geometry::FaceHandle face_{};

        MeshSelectionSnapshot previousSelection_{};
        bool executed_ = false;
    };

} // namespace locus::editor