/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/mesh/vertex/RegisterVertexActions.h"

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/mesh/MeshOperationAction.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/topology/DissolveMeshElementsOp.h"
#include "kernel/modeling/operations/topology/MergeVerticesOp.h"
#include "kernel/modeling/operations/topology/DeleteMeshElementsOp.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        using VertexHandle =
            kernel::geometry::VertexHandle;

        using VertexList =
            std::vector<VertexHandle>;

        /**
         * @brief Strategy used by an immediate merge vertex action.
         */
        enum class VertexMergeActionMode {
            Center,
            First,
            Last
        };

        /**
         * @brief Converts a textual identifier into ActionId.
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
         * @brief Checks that every captured vertex still exists.
         *
         * @param mesh Mesh containing the captured vertices.
         * @param vertices Captured vertex handles.
         * @return True when every handle references an active vertex.
         */
        bool all_vertices_valid(
            const kernel::geometry::LEM& mesh,
            const VertexList& vertices) {
            if (vertices.size() < 2u) {
                return false;
            }

            for (const VertexHandle vertex : vertices) {
                if (!mesh.is_valid(vertex)) {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Computes the average position of captured vertices.
         *
         * This function must be called before the first topology mutation,
         * because source vertices are removed as merges are performed.
         *
         * @param mesh Mesh containing the vertices.
         * @param vertices Vertices whose positions will be averaged.
         * @return Average object-space position.
         */
        glm::vec3 calculate_center(
            const kernel::geometry::LEM& mesh,
            const VertexList& vertices) {
            glm::vec3 center{ 0.0f };

            for (const VertexHandle vertex : vertices) {
                center += mesh.vertex(vertex).position;
            }

            return center
                / static_cast<float>(vertices.size());
        }

        /**
         * @brief Executes one pair merge through the modeling operation.
         *
         * @param editor Editable mesh facade.
         * @param source Vertex removed by the merge.
         * @param target Vertex that survives the merge.
         * @param finalPosition Optional position assigned to the survivor.
         * @param useFinalPosition Whether PairAtPosition must be used.
         * @return True when the operation changed the mesh.
         */
        bool execute_pair_merge(
            kernel::geometry::LEMEditor& editor,
            VertexHandle source,
            VertexHandle target,
            const glm::vec3& finalPosition,
            bool useFinalPosition) {
            kernel::modeling::MergeVerticesOp operation =
                useFinalPosition
                ? kernel::modeling::MergeVerticesOp{
                    source,
                    target,
                    finalPosition
            }
                : kernel::modeling::MergeVerticesOp{
                    source,
                    target
            };

            kernel::modeling::OperationContext
                operationContext{};

            operationContext.mesh = &editor.mesh();
            operationContext.validateAfterExecute = true;
            operationContext.rebuildNormals = true;
            operationContext.allowNonManifold = false;

            const kernel::modeling::OperationResult result =
                operation.execute(operationContext);

            return result.is_success()
                && result.changed();
        }

        /**
         * @brief Merges every selected vertex into one survivor.
         *
         * The complete sequence runs inside one ApplyMeshOperationCommand
         * callback. Returning false from any step causes the command to
         * restore its pre-operation snapshot.
         *
         * @param editor Editable mesh facade.
         * @param vertices Ordered selected vertices.
         * @param mode Merge destination strategy.
         * @return True when every required pair merge succeeded.
         */
        bool execute_vertex_merge(
            kernel::geometry::LEMEditor& editor,
            const VertexList& vertices,
            VertexMergeActionMode mode) {
            kernel::geometry::LEM& mesh =
                editor.mesh();

            if (!all_vertices_valid(mesh, vertices)) {
                return false;
            }

            const std::size_t survivorIndex =
                mode == VertexMergeActionMode::Last
                ? vertices.size() - 1u
                : 0u;

            const VertexHandle survivor =
                vertices[survivorIndex];

            const bool mergeAtCenter =
                mode == VertexMergeActionMode::Center;

            const glm::vec3 finalPosition =
                mergeAtCenter
                ? calculate_center(mesh, vertices)
                : mesh.vertex(survivor).position;

            bool changed = false;

            for (std::size_t index = 0u;
                index < vertices.size();
                ++index) {
                if (index == survivorIndex) {
                    continue;
                }

                const VertexHandle source =
                    vertices[index];

                if (!mesh.is_valid(source)
                    || !mesh.is_valid(survivor)) {
                    return false;
                }

                if (!execute_pair_merge(
                    editor,
                    source,
                    survivor,
                    finalPosition,
                    mergeAtCenter)) {
                    return false;
                }

                changed = true;
            }

            return changed
                && mesh.is_valid(survivor);
        }

        /**
         * @brief Checks whether Dissolve Vertex can run without mutating the document.
         *
         * @param node Active mesh node.
         * @param target Captured vertex target.
         * @return True when the kernel accepts the selected topology.
         */
        bool can_dissolve_vertices(
            const MeshNode& node,
            const MeshToolTarget& target) {
            if (target.vertices.empty()) {
                return false;
            }

            kernel::geometry::LEM candidate =
                node.mesh();

            kernel::modeling::DissolveMeshElementsOp operation =
                kernel::modeling::DissolveMeshElementsOp::vertices(
                    target.vertices);

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
         * @brief Creates one merge vertex action.
         *
         * @param id Stable action identifier.
         * @param name User-facing action name.
         * @param description User-facing action description.
         * @param commandLabel Undo history label.
         * @param mode Merge strategy.
         * @param keywords Search keywords.
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_merge_action(
                std::string_view id,
                std::string name,
                std::string description,
                std::string commandLabel,
                VertexMergeActionMode mode,
                std::vector<std::string> keywords) {
            ActionDescriptor descriptor{
                make_action_id(id),
                std::move(name),
                std::move(description),
                ActionCategory::Mesh,
                std::move(keywords)
            };

            MeshOperationAction::OperationFactory operationFactory =
                [mode](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const VertexList vertices =
                    target.vertices;

                return [vertices, mode](
                    kernel::geometry::LEMEditor& editor) {
                        return execute_vertex_merge(
                            editor,
                            vertices,
                            mode);
                    };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Vertex,
                2u,
                std::move(operationFactory),
                std::move(commandLabel));
        }

        /**
         * @brief Creates the Merge at Center action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_merge_at_center_action() {
            return make_merge_action(
                vertex_actions::MergeAtCenterId,
                "Merge at Center",
                "Merges the selected vertices into one vertex positioned "
                "at the average of their original positions.",
                "Merge Vertices at Center",
                VertexMergeActionMode::Center,
                {
                    "merge",
                    "vertices",
                    "vertex",
                    "center",
                    "centre",
                    "average",
                    "collapse",
                    "weld"
                });
        }

        /**
         * @brief Creates the Merge at First action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_merge_at_first_action() {
            return make_merge_action(
                vertex_actions::MergeAtFirstId,
                "Merge at First",
                "Merges the selected vertices into the first vertex in "
                "the ordered selection.",
                "Merge Vertices at First",
                VertexMergeActionMode::First,
                {
                    "merge",
                    "vertices",
                    "vertex",
                    "first",
                    "initial",
                    "collapse",
                    "weld"
                });
        }

        /**
         * @brief Creates the Merge at Last action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_merge_at_last_action() {
            return make_merge_action(
                vertex_actions::MergeAtLastId,
                "Merge at Last",
                "Merges the selected vertices into the last vertex in "
                "the ordered selection.",
                "Merge Vertices at Last",
                VertexMergeActionMode::Last,
                {
                    "merge",
                    "vertices",
                    "vertex",
                    "last",
                    "final",
                    "collapse",
                    "weld"
                });
        }

        /**
         * @brief Creates the built-in Dissolve Vertex action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_dissolve_vertex_action() {
            ActionDescriptor descriptor{
                make_action_id(
                    vertex_actions::DissolveId),
                "Dissolve Vertices",
                "Dissolves selected loose chain vertices while preserving "
                "their surrounding connectivity.",
                ActionCategory::Mesh,
                {
                    "dissolve",
                    "vertex",
                    "vertices",
                    "remove",
                    "chain",
                    "edge",
                    "topology"
                }
            };

            MeshOperationAction::OperationFactory operationFactory =
                [](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const VertexList vertices =
                    target.vertices;

                return [vertices](
                    kernel::geometry::LEMEditor& editor) {
                    kernel::modeling::DissolveMeshElementsOp operation =
                        kernel::modeling::DissolveMeshElementsOp::vertices(
                            vertices);

                    kernel::modeling::OperationContext
                        operationContext{};

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
                SelectionGranularity::Vertex,
                1u,
                std::move(operationFactory),
                "Dissolve Vertices",
                can_dissolve_vertices);
        }

        /**
         * @brief Creates the built-in Delete Vertex action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_delete_vertex_action() {
            ActionDescriptor descriptor{
                make_action_id(
                    vertex_actions::DeleteId),
                "Delete Vertices",
                "Deletes selected mesh vertices and their incident faces "
                "and edges.",
                ActionCategory::Mesh,
                {
                    "delete",
                    "remove",
                    "vertex",
                    "vertices",
                    "edge",
                    "face",
                    "topology"
                }
            };

            MeshOperationAction::OperationFactory operationFactory =
                [](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const VertexList vertices =
                    target.vertices;

                return [vertices](
                    kernel::geometry::LEMEditor& editor) {
                    kernel::modeling::DeleteMeshElementsOp operation =
                        kernel::modeling::DeleteMeshElementsOp::vertices(
                            vertices);

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
                };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Vertex,
                1u,
                std::move(operationFactory),
                "Delete Vertices");
        }

        /**
         * @brief Removes actions inserted by a failed group registration.
         *
         * @param registry Registry containing the inserted actions.
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

    bool register_vertex_actions(
        ActionRegistry& registry) {
        std::vector<ActionId> insertedIds{};
        insertedIds.reserve(5u);

        const ActionId mergeAtCenterId =
            make_action_id(
                vertex_actions::MergeAtCenterId);

        if (!registry.register_action(
            make_merge_at_center_action())) {
            return false;
        }

        insertedIds.push_back(mergeAtCenterId);

        const ActionId mergeAtFirstId =
            make_action_id(
                vertex_actions::MergeAtFirstId);

        if (!registry.register_action(
            make_merge_at_first_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(mergeAtFirstId);

        const ActionId mergeAtLastId =
            make_action_id(
                vertex_actions::MergeAtLastId);

        if (!registry.register_action(
            make_merge_at_last_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(mergeAtLastId);

        const ActionId dissolveId =
            make_action_id(
                vertex_actions::DissolveId);

        if (!registry.register_action(
            make_dissolve_vertex_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(dissolveId);

        const ActionId deleteId =
            make_action_id(
                vertex_actions::DeleteId);

        if (!registry.register_action(
            make_delete_vertex_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(deleteId);

        return true;
    }

} // namespace locus::editor
