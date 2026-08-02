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
#include <vector>

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

        [[nodiscard]] std::array<ActionId, 3> dissolve_candidates()
        {
            return {
                make_action_id(vertex_actions::DissolveId),
                make_action_id(edge_actions::DissolveId),
                make_action_id(face_actions::DissolveId)
            };
        }

        class ContextualRegistryAction final : public IEditorAction {
        public:
            ContextualRegistryAction(
                ActionRegistry& registry,
                ActionDescriptor descriptor,
                std::vector<ActionId> candidates,
                std::string unavailableMessage)
                : registry_(&registry)
                , descriptor_(std::move(descriptor))
                , candidates_(std::move(candidates))
                , unavailableMessage_(std::move(unavailableMessage))
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
                        unavailableMessage_);
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

                for (const ActionId& id : candidates_) {
                    IEditorAction* action = registry_->find(id);

                    if (action && action->can_execute(context)) {
                        return action;
                    }
                }

                return nullptr;
            }

            ActionRegistry* registry_ = nullptr;
            ActionDescriptor descriptor_{};
            std::vector<ActionId> candidates_{};
            std::string unavailableMessage_{};
        };

        [[nodiscard]] std::unique_ptr<IEditorAction> make_contextual_delete(
            ActionRegistry& registry)
        {
            const std::array<ActionId, 4> candidates =
                delete_candidates();

            return std::make_unique<ContextualRegistryAction>(
                registry,
                ActionDescriptor{
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
                    }
                },
                std::vector<ActionId>{
                    candidates.begin(),
                    candidates.end()
                },
                "Delete is not available for the current selection.");
        }

        [[nodiscard]] std::unique_ptr<IEditorAction> make_contextual_dissolve(
            ActionRegistry& registry)
        {
            const std::array<ActionId, 3> meshCandidates =
                dissolve_candidates();

            return std::make_unique<ContextualRegistryAction>(
                registry,
                ActionDescriptor{
                    make_action_id(edit_actions::DissolveId),
                    "Dissolve",
                    "Dissolves the current mesh component selection when "
                    "the surface can be preserved.",
                    ActionCategory::Mesh,
                    {
                        "dissolve",
                        "vertex",
                        "edge",
                        "face",
                        "topology"
                    }
                },
                std::vector<ActionId>{
                    meshCandidates[0],
                    meshCandidates[1],
                    meshCandidates[2]
                },
                "Dissolve is not available for the current selection.");
        }

    } // namespace

    bool register_edit_actions(ActionRegistry& registry)
    {
        if (!registry.register_action(make_contextual_delete(registry))) {
            return false;
        }

        if (!registry.register_action(make_contextual_dissolve(registry))) {
            registry.unregister_action(make_action_id(edit_actions::DeleteId));
            return false;
        }

        return true;
    }

} // namespace locus::editor
