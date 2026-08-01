/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/scene/RegisterSceneActions.h"

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/core/IEditorAction.h"
#include "editor/command/scene/DeleteNodesCommand.h"
#include "editor/scene/SceneNode.h"
#include "editor/selection/SelectionScope.h"
#include "editor/selection/SelectionState.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        ActionId make_action_id(std::string_view value)
        {
            return ActionId{ std::string{ value } };
        }

        [[nodiscard]] bool can_delete_selected_objects(
            const ActionContext& context)
        {
            if (context.selection().scope() != SelectionScope::Scene
                || context.selection().granularity()
                != SelectionGranularity::Object
                || context.selection().objects().empty()) {
                return false;
            }

            for (const SceneNodeId nodeId
                : context.selection().objects().selected()) {
                const SceneNode* node =
                    context.scene().find_node(nodeId);

                if (node
                    && !node->metadata().locked) {
                    return true;
                }
            }

            return false;
        }

        class DeleteObjectsAction final : public IEditorAction {
        public:
            DeleteObjectsAction()
                : descriptor_(
                    make_action_id(scene_actions::DeleteObjectsId),
                    "Delete Objects",
                    "Deletes the selected scene objects.",
                    ActionCategory::Scene,
                    {
                        "delete",
                        "remove",
                        "object",
                        "objects",
                        "scene",
                        "node",
                        "nodes"
                    })
            {
            }

            [[nodiscard]] const ActionDescriptor& descriptor()
                const override
            {
                return descriptor_;
            }

            [[nodiscard]] bool can_execute(
                const ActionContext& context) const override
            {
                return can_delete_selected_objects(context);
            }

            ActionResult execute(ActionContext& context) override
            {
                if (!can_execute(context)) {
                    return ActionResult::unavailable(
                        "No deletable scene object is selected.");
                }

                std::vector<SceneNodeId> nodes{};

                for (const SceneNodeId nodeId
                    : context.selection().objects().selected()) {
                    const SceneNode* node =
                        context.scene().find_node(nodeId);

                    if (node && !node->metadata().locked) {
                        nodes.push_back(nodeId);
                    }
                }

                return ActionResult::from_command(
                    context.execute_command(
                        std::make_unique<DeleteNodesCommand>(
                            std::move(nodes))));
            }

        private:
            ActionDescriptor descriptor_{};
        };

    } // namespace

    bool register_scene_actions(ActionRegistry& registry)
    {
        return registry.register_action(
            std::make_unique<DeleteObjectsAction>());
    }

} // namespace locus::editor
