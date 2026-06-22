/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/face/FlipFaceOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"

namespace locus::kernel::modeling {

    FlipFaceOp::FlipFaceOp(geometry::FaceHandle face)
        : face_(face)
    {
    }

    std::string_view FlipFaceOp::name() const
    {
        return "FlipFaceOp";
    }

    void FlipFaceOp::set_face(geometry::FaceHandle face)
    {
        face_ = face;
    }

    geometry::FaceHandle FlipFaceOp::face() const
    {
        return face_;
    }

    OperationResult FlipFaceOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();

        if (!mesh.is_valid(face_)) {
            return OperationResult::no_change(
                "Flip face operation has an invalid face.");
        }

        geometry::LEMEditor editor(mesh);

        const bool changed = editor.flip_face(face_);

        if (!changed) {
            return OperationResult::no_change(
                "Flip face operation did not modify the mesh.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_normals_around_face(face_);
        }

        return OperationResult::success(editor.take_diff());
    }

}