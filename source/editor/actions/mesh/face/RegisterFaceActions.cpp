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
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        /**
         * @brief Creates the built-in Flip Face action.
         *
         * @return Owned action instance.
         */
        std::unique_ptr<IEditorAction>
            make_flip_face_action() {
            ActionDescriptor descriptor{
                ActionId{
                    std::string{
                        face_actions::FlipFaceId
                    }
                },
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

    } // namespace

    bool register_face_actions(
        ActionRegistry& registry) {
        return registry.register_action(
            make_flip_face_action());
    }

} // namespace locus::editor