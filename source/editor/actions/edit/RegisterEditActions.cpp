/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/edit/RegisterEditActions.h"

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/core/IEditorAction.h"
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/actions/mesh/face/RegisterFaceActions.h"
#include "editor/actions/mesh/vertex/RegisterVertexActions.h"
#include "editor/actions/scene/RegisterSceneActions.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace locus::editor {

    namespace {

        ActionId make_action_id(std::string_view value)
        {
            return ActionId{ std::string{ value } };
        }

        [[nodiscard]] std::array<ActionId, 4> delete_candidates()
        {
            return {
                make_action_id(scene_actions::DeleteObjectsId),
                make_action_id(vertex_actions::DeleteId),
                make_action_id(edge_actions::DeleteId),
                make_action_id(face_actions::DeleteId)
            };
        }

        class ContextualDeleteAction final : public IEditorAction {
        public:
            explicit ContextualDeleteAction(ActionRegistry& registry)
                : registry_(&registry)
                , descriptor_(
                    make_action_id(edit_actions::DeleteId),
                    "Delete",
                    "Deletes the current object or mesh component selection.",
                    ActionCategory::Selection,
                    {
                        "delete",
                        "remove",
                        "object",
                        "vertex",
                        "edge",
                        "face"
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
                return find_available_action(context) != nullptr;
            }

            ActionResult execute(ActionContext& context) override
            {
                IEditorAction* action = find_available_action(context);

                if (!action) {
                    return ActionResult::unavailable(
                        "Delete is not available for the current selection.");
                }

                return action->execute(context);
            }

        private:
            [[nodiscard]] IEditorAction* find_available_action(
                const ActionContext& context) const
            {
                if (!registry_) {
                    return nullptr;
                }

                for (const ActionId& id : delete_candidates()) {
                    IEditorAction* action = registry_->find(id);

                    if (action && action->can_execute(context)) {
                        return action;
                    }
                }

                return nullptr;
            }

            ActionRegistry* registry_ = nullptr;
            ActionDescriptor descriptor_{};
        };

    } // namespace

    bool register_edit_actions(ActionRegistry& registry)
    {
        return registry.register_action(
            std::make_unique<ContextualDeleteAction>(registry));
    }

} // namespace locus::editor
