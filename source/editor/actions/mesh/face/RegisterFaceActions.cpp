/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/actions/mesh/face/RegisterFaceActions.h"

#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/mesh/MeshOperationAction.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        /**
         * @brief Converts a stable textual identifier to ActionId.
         *
         * @param value Stable textual action identifier.
         * @return Action identifier.
         */
        ActionId make_action_id(std::string_view value) {
            return ActionId{
                std::string{ value }
            };
        }

        /**
         * @brief Creates the built-in Flip Face action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_flip_face_action() {
            ActionDescriptor descriptor{
                make_action_id(
                    face_actions::FlipFaceId),
                "Flip Face",
                "Reverses the winding and normal orientation of the "
                "selected mesh faces.",
                ActionCategory::Mesh,
                {
                    "flip",
                    "face",
                    "faces",
                    "normal",
                    "normals",
                    "winding",
                    "orientation",
                    "reverse"
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

                        bool changed = false;

                        for (const kernel::geometry::FaceHandle face
                            : faces) {
                            kernel::modeling::FlipFaceOp operation{
                                face
                            };

                            kernel::modeling::OperationContext
                                operationContext{};

                            operationContext.mesh = &editor.mesh();
                            operationContext.validateAfterExecute = true;
                            operationContext.rebuildNormals = true;
                            operationContext.allowNonManifold = true;

                            const kernel::modeling::OperationResult
                                operationResult =
                                operation.execute(
                                    operationContext);

                            if (!operationResult.is_success()
                                || !operationResult.changed()) {
                                return false;
                            }

                            changed = true;
                        }

                        return changed;
                    };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Face,
                1u,
                std::move(operationFactory),
                "Flip Faces");
        }

        /**
         * @brief Creates the built-in Recalculate Normals action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_recalculate_normals_action() {
            ActionDescriptor descriptor{
                make_action_id(
                    face_actions::RecalculateNormalsId),
                "Recalculate Normals",
                "Recalculates the geometric normals of the selected mesh "
                "faces from their current vertex positions and winding.",
                ActionCategory::Mesh,
                {
                    "recalculate",
                    "rebuild",
                    "normal",
                    "normals",
                    "face",
                    "faces",
                    "geometry",
                    "shading"
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

                        for (const kernel::geometry::FaceHandle face
                            : faces) {
                            if (!editor.rebuild_normals_around_face(
                                face)) {
                                return false;
                            }
                        }

                        return true;
                    };
                };

            return std::make_unique<MeshOperationAction>(
                std::move(descriptor),
                SelectionGranularity::Face,
                1u,
                std::move(operationFactory),
                "Recalculate Normals");
        }

        /**
         * @brief Removes actions inserted during a failed registration.
         *
         * @param registry Registry containing inserted actions.
         * @param insertedIds Identifiers inserted by this invocation.
         */
        void rollback_registration(
            ActionRegistry& registry,
            const std::vector<ActionId>& insertedIds) {
            for (const ActionId& id : insertedIds) {
                registry.unregister_action(id);
            }
        }

    } // namespace

    bool register_face_actions(
        ActionRegistry& registry) {
        std::vector<ActionId> insertedIds{};
        insertedIds.reserve(2u);

        const ActionId flipFaceId =
            make_action_id(
                face_actions::FlipFaceId);

        if (!registry.register_action(
            make_flip_face_action())) {
            return false;
        }

        insertedIds.push_back(flipFaceId);

        const ActionId recalculateNormalsId =
            make_action_id(
                face_actions::RecalculateNormalsId);

        if (!registry.register_action(
            make_recalculate_normals_action())) {
            rollback_registration(
                registry,
                insertedIds);

            return false;
        }

        insertedIds.push_back(
            recalculateNormalsId);

        return true;
    }

} // namespace locus::editor