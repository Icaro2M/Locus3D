/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/ICommand.h"
#include "editor/command/selection/ObjectSelectionSnapshot.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/tools/selection/shapes/SelectionShapeTypes.h"

#include <string_view>
#include <vector>

namespace locus::editor {

    class SetObjectSelectionCommand final : public ICommand {
    public:
        SetObjectSelectionCommand(
            std::vector<SceneNodeId> objects,
            SelectionOperation operation);

        [[nodiscard]] std::string_view name() const override;

        CommandResult execute(CommandContext& context) override;
        CommandResult undo(CommandContext& context) override;

    private:
        std::vector<SceneNodeId> objects_{};
        SelectionOperation operation_ = SelectionOperation::Replace;
        ObjectSelectionSnapshot previousSelection_{};
        bool executed_ = false;
    };

} // namespace locus::editor
