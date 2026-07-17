/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/mesh/topology/RegisterTopologyActions.h"

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/mesh/MeshOperationAction.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/topology/SubdivideOp.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        /**
         * @brief Creates the built-in Subdivide Edges action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_subdivide_edges_action() {
            ActionDescriptor descriptor{
                ActionId{
                    std::string{
                        topology_actions::SubdivideEdgesId
                    }
                },
                "Subdivide Edges",
                "Splits each selected mesh edge at its midpoint.",
                ActionCategory::Mesh,
                {
                    "subdivide",
                    "edge",
                    "edges",
                    "split",
                    "midpoint",
                    "topology"
                }
            };

            MeshOperationAction::OperationFactory operationFactory =
                [](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const std::vector<
                    kernel::geometry::EdgeHandle>
                    edges = target.edges;

                return [edges](
                    kernel::geometry::LEMEditor& editor) {
                        if (edges.empty()) {
                            return false;
                        }

                        kernel::modeling::SubdivideOp operation{
                            edges
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
                    };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Edge,
                1u,
                std::move(operationFactory),
                "Subdivide Edges");
        }

        /**
         * @brief Creates the built-in Subdivide Faces action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_subdivide_faces_action() {
            ActionDescriptor descriptor{
                ActionId{
                    std::string{
                        topology_actions::SubdivideFacesId
                    }
                },
                "Subdivide Faces",
                "Subdivides each selected mesh face around a new center "
                "vertex.",
                ActionCategory::Mesh,
                {
                    "subdivide",
                    "face",
                    "faces",
                    "center",
                    "triangulate",
                    "topology"
                }
            };

            MeshOperationAction::OperationFactory operationFactory =
                [](
                    const MeshToolTarget& target)
                -> ApplyMeshOperationCommand::MeshOperation {
                const std::vector<
                    kernel::geometry::FaceHandle>
                    faces = target.faces;

                return [faces](
                    kernel::geometry::LEMEditor& editor) {
                        if (faces.empty()) {
                            return false;
                        }

                        kernel::modeling::SubdivideOp operation =
                            kernel::modeling::SubdivideOp::faces(
                                faces);

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
                SelectionGranularity::Face,
                1u,
                std::move(operationFactory),
                "Subdivide Faces");
        }

    } // namespace

    bool register_topology_actions(
        ActionRegistry& registry) {
        const ActionId subdivideEdgesId{
            std::string{
                topology_actions::SubdivideEdgesId
            }
        };

        if (!registry.register_action(
            make_subdivide_edges_action())) {
            return false;
        }

        if (!registry.register_action(
            make_subdivide_faces_action())) {
            registry.unregister_action(
                subdivideEdgesId);

            return false;
        }

        return true;
    }

} // namespace locus::editor