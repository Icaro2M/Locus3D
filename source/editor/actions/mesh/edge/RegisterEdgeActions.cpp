/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/mesh/edge/RegisterEdgeActions.h"

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/mesh/MeshOperationAction.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/edge/CreaseOp.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        using EdgeHandle =
            kernel::geometry::EdgeHandle;

        using EdgeList =
            std::vector<EdgeHandle>;

        /**
         * @brief Converts a stable textual identifier into ActionId.
         *
         * @param value Stable textual identifier.
         * @return Action identifier.
         */
        ActionId make_action_id(std::string_view value) {
            return ActionId{
                std::string{ value }
            };
        }

        /**
         * @brief Applies one crease value to captured mesh edges.
         *
         * @param editor Editable mesh facade.
         * @param edges Captured selected edges.
         * @param crease Crease value in the range [0, 1].
         * @return True when the operation changed the mesh.
         */
        bool execute_crease(
            kernel::geometry::LEMEditor& editor,
            const EdgeList& edges,
            float crease) {
            if (edges.empty()) {
                return false;
            }

            for (const EdgeHandle edge : edges) {
                if (!editor.mesh().is_valid(edge)) {
                    return false;
                }
            }

            kernel::modeling::CreaseOp operation{
                edges,
                crease
            };

            kernel::modeling::OperationContext
                operationContext{};

            operationContext.mesh = &editor.mesh();
            operationContext.validateAfterExecute = true;
            operationContext.rebuildNormals = true;
            operationContext.allowNonManifold = true;

            const kernel::modeling::OperationResult result =
                operation.execute(operationContext);

            return result.is_success()
                && result.changed();
        }

        /**
         * @brief Creates an action that assigns one crease value.
         *
         * @param id Stable action identifier.
         * @param name User-facing action name.
         * @param description User-facing description.
         * @param commandLabel Undo history label.
         * @param crease Crease value assigned to selected edges.
         * @param keywords Search terms.
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_crease_action(
                std::string_view id,
                std::string name,
                std::string description,
                std::string commandLabel,
                float crease,
                std::vector<std::string> keywords) {
            ActionDescriptor descriptor{
                make_action_id(id),
                std::move(name),
                std::move(description),
                ActionCategory::Mesh,
                std::move(keywords)
            };

            MeshOperationAction::OperationFactory operationFactory =
                [crease](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const EdgeList edges =
                    target.edges;

                return [edges, crease](
                    kernel::geometry::LEMEditor& editor) {
                        return execute_crease(
                            editor,
                            edges,
                            crease);
                    };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Edge,
                1u,
                std::move(operationFactory),
                std::move(commandLabel));
        }

        /**
         * @brief Creates the Mark Sharp action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_mark_sharp_action() {
            return make_crease_action(
                edge_actions::MarkSharpId,
                "Mark Sharp",
                "Marks the selected mesh edges as fully sharp.",
                "Mark Edges Sharp",
                1.0f,
                {
                    "mark",
                    "sharp",
                    "crease",
                    "edge",
                    "edges",
                    "hard",
                    "normal",
                    "shading"
                });
        }

        /**
         * @brief Creates the Clear Sharp action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_clear_sharp_action() {
            return make_crease_action(
                edge_actions::ClearSharpId,
                "Clear Sharp",
                "Removes the sharp crease from the selected mesh edges.",
                "Clear Edge Sharpness",
                0.0f,
                {
                    "clear",
                    "sharp",
                    "crease",
                    "edge",
                    "edges",
                    "smooth",
                    "normal",
                    "shading"
                });
        }

        /**
         * @brief Removes actions inserted by a failed registration.
         *
         * @param registry Registry containing inserted actions.
         * @param insertedIds Identifiers inserted by this invocation.
         */
        void rollback_registration(
            ActionRegistry& registry,
            const std::vector<ActionId>& insertedIds) {
            for (auto iterator = insertedIds.rbegin();
                iterator != insertedIds.rend();
                ++iterator) {
                registry.unregister_action(*iterator);
            }
        }

    } // namespace

    bool register_edge_actions(
        ActionRegistry& registry) {
        std::vector<ActionId> insertedIds{};
        insertedIds.reserve(2u);

        const ActionId markSharpId =
            make_action_id(
                edge_actions::MarkSharpId);

        if (!registry.register_action(
            make_mark_sharp_action())) {
            return false;
        }

        insertedIds.push_back(markSharpId);

        const ActionId clearSharpId =
            make_action_id(
                edge_actions::ClearSharpId);

        if (!registry.register_action(
            make_clear_sharp_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(clearSharpId);

        return true;
    }

} // namespace locus::editor