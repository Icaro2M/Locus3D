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
#include "editor/scene/MeshNode.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/edge/CreaseOp.h"
#include "kernel/modeling/operations/topology/BridgeEdgeOp.h"
#include "kernel/modeling/operations/topology/DeleteMeshElementsOp.h"
#include "kernel/modeling/operations/topology/DissolveMeshElementsOp.h"

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
         * @brief Checks the cheap Bridge Edge preconditions.
         *
         * Deep topology validation remains in BridgeEdgeOp. Availability only
         * verifies the contextual requirements that can be checked without
         * mutating the mesh.
         *
         * @param node Active mesh node.
         * @param target Captured edge target.
         * @return True when Bridge Edge should be available.
         */
        bool can_bridge_edges(
            const MeshNode& node,
            const MeshToolTarget& target) {
            if (target.edges.size() != 2u) {
                return false;
            }

            const EdgeHandle firstEdge = target.edges[0];
            const EdgeHandle secondEdge = target.edges[1];

            if (firstEdge == secondEdge) {
                return false;
            }

            const kernel::geometry::LEM& mesh =
                node.mesh();

            return mesh.is_valid(firstEdge)
                && mesh.is_valid(secondEdge)
                && kernel::geometry::TopologyTraversal::is_boundary_edge(
                    mesh,
                    firstEdge)
                && kernel::geometry::TopologyTraversal::is_boundary_edge(
                    mesh,
                    secondEdge);
        }

        /**
         * @brief Applies Bridge Edge to the two captured boundary edges.
         *
         * @param editor Editable mesh facade.
         * @param edges Captured selected edge handles.
         * @return True when the kernel operation changed the mesh.
         */
        bool execute_bridge(
            kernel::geometry::LEMEditor& editor,
            const EdgeList& edges) {
            if (edges.size() != 2u) {
                return false;
            }

            const EdgeHandle firstEdge = edges[0];
            const EdgeHandle secondEdge = edges[1];

            if (firstEdge == secondEdge
                || !editor.mesh().is_valid(firstEdge)
                || !editor.mesh().is_valid(secondEdge)) {
                return false;
            }

            kernel::modeling::BridgeEdgeOp operation =
                kernel::modeling::BridgeEdgeOp::edges(
                    EdgeList{ firstEdge },
                    EdgeList{ secondEdge });

            operation.set_closed(false);

            kernel::modeling::OperationContext operationContext{};
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
         * @brief Checks whether Dissolve Edge can run without mutating the document.
         *
         * @param node Active mesh node.
         * @param target Captured edge target.
         * @return True when the kernel accepts the selected topology.
         */
        bool can_dissolve_edges(
            const MeshNode& node,
            const MeshToolTarget& target) {
            if (target.edges.empty()) {
                return false;
            }

            kernel::geometry::LEM candidate =
                node.mesh();

            kernel::modeling::DissolveMeshElementsOp operation =
                kernel::modeling::DissolveMeshElementsOp::edges(
                    target.edges);

            kernel::modeling::OperationContext operationContext{};
            operationContext.mesh = &candidate;
            operationContext.validateAfterExecute = true;
            operationContext.rebuildNormals = false;
            operationContext.allowNonManifold = false;

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
         * @brief Creates the Bridge Edge action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_bridge_action() {
            ActionDescriptor descriptor{
                make_action_id(
                    edge_actions::BridgeId),
                "Bridge Edge",
                "Creates one face bridge between exactly two selected "
                "boundary edges.",
                ActionCategory::Mesh,
                {
                    "bridge",
                    "edge",
                    "edges",
                    "boundary",
                    "connect",
                    "face",
                    "topology"
                }
            };

            MeshOperationAction::OperationFactory operationFactory =
                [](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const EdgeList edges =
                    target.edges;

                return [edges](
                    kernel::geometry::LEMEditor& editor) {
                        return execute_bridge(
                            editor,
                            edges);
                    };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Edge,
                2u,
                std::move(operationFactory),
                "Bridge Edge",
                can_bridge_edges);
        }

        /**
         * @brief Creates the built-in Dissolve Edge action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_dissolve_edge_action() {
            ActionDescriptor descriptor{
                make_action_id(
                    edge_actions::DissolveId),
                "Dissolve Edges",
                "Dissolves selected manifold edges by merging their "
                "adjacent faces into polygonal faces.",
                ActionCategory::Mesh,
                {
                    "dissolve",
                    "edge",
                    "edges",
                    "merge",
                    "face",
                    "faces",
                    "topology"
                }
            };

            MeshOperationAction::OperationFactory operationFactory =
                [](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const EdgeList edges =
                    target.edges;

                return [edges](
                    kernel::geometry::LEMEditor& editor) {
                    kernel::modeling::DissolveMeshElementsOp operation =
                        kernel::modeling::DissolveMeshElementsOp::edges(
                            edges);

                    kernel::modeling::OperationContext operationContext{};
                    operationContext.mesh = &editor.mesh();
                    operationContext.validateAfterExecute = true;
                    operationContext.rebuildNormals = true;
                    operationContext.allowNonManifold = false;

                    const kernel::modeling::OperationResult result =
                        operation.execute(operationContext);

                    return result.is_success()
                        && result.changed();
                };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Edge,
                1u,
                std::move(operationFactory),
                "Dissolve Edges",
                can_dissolve_edges);
        }

        /**
         * @brief Creates the built-in Delete Edge action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_delete_edge_action() {
            ActionDescriptor descriptor{
                make_action_id(
                    edge_actions::DeleteId),
                "Delete Edges",
                "Deletes selected mesh edges after removing faces that "
                "depend on them.",
                ActionCategory::Mesh,
                {
                    "delete",
                    "remove",
                    "edge",
                    "edges",
                    "face",
                    "faces",
                    "topology"
                }
            };

            MeshOperationAction::OperationFactory operationFactory =
                [](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const EdgeList edges =
                    target.edges;

                return [edges](
                    kernel::geometry::LEMEditor& editor) {
                    kernel::modeling::DeleteMeshElementsOp operation =
                        kernel::modeling::DeleteMeshElementsOp::edges(
                            edges);

                    kernel::modeling::OperationContext operationContext{};
                    operationContext.mesh = &editor.mesh();
                    operationContext.validateAfterExecute = true;
                    operationContext.rebuildNormals = true;
                    operationContext.allowNonManifold = true;

                    const kernel::modeling::OperationResult result =
                        operation.execute(operationContext);

                    return result.is_success()
                        && result.changed();
                };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Edge,
                1u,
                std::move(operationFactory),
                "Delete Edges");
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
        insertedIds.reserve(5u);

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

        const ActionId bridgeId =
            make_action_id(
                edge_actions::BridgeId);

        if (!registry.register_action(
            make_bridge_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(bridgeId);

        const ActionId dissolveId =
            make_action_id(
                edge_actions::DissolveId);

        if (!registry.register_action(
            make_dissolve_edge_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(dissolveId);

        const ActionId deleteId =
            make_action_id(
                edge_actions::DeleteId);

        if (!registry.register_action(
            make_delete_edge_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(deleteId);

        return true;
    }

} // namespace locus::editor
