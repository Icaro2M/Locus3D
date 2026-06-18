/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/face/FlipFaceOp.h"

#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <utility>

namespace locus::kernel::modeling {

    FlipFaceOp::FlipFaceOp(std::vector<geometry::FaceHandle> faces)
        : faces_(std::move(faces))
    {
    }

    std::string_view FlipFaceOp::name() const
    {
        return "FlipFaceOp";
    }

    void FlipFaceOp::set_target(FlipFaceTarget target)
    {
        target_ = target;
    }

    FlipFaceTarget FlipFaceOp::target() const
    {
        return target_;
    }

    void FlipFaceOp::set_faces(std::vector<geometry::FaceHandle> faces)
    {
        faces_ = std::move(faces);
    }

    const std::vector<geometry::FaceHandle>& FlipFaceOp::faces() const
    {
        return faces_;
    }

    void FlipFaceOp::clear_faces()
    {
        faces_.clear();
    }

    OperationResult FlipFaceOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();
        geometry::LEMDiff diff;

        const std::vector<geometry::FaceHandle> targets = collect_faces(mesh);

        if (targets.empty()) {
            return OperationResult::no_change("Flip face operation has no valid target faces.");
        }

        std::size_t changedCount = 0;

        for (geometry::FaceHandle faceHandle : targets) {
            if (flip_face(mesh, faceHandle, diff)) {
                ++changedCount;
            }
        }

        if (changedCount == 0) {
            return OperationResult::no_change("Flip face operation did not modify any face.");
        }

        return OperationResult::success(std::move(diff));
    }

    std::vector<geometry::FaceHandle> FlipFaceOp::collect_faces(const geometry::LEM& mesh) const
    {
        if (!faces_.empty()) {
            std::vector<geometry::FaceHandle> result;
            result.reserve(faces_.size());

            for (geometry::FaceHandle faceHandle : faces_) {
                if (mesh.is_valid(faceHandle)) {
                    result.push_back(faceHandle);
                }
            }

            return result;
        }

        std::vector<geometry::FaceHandle> result;

        for (geometry::FaceHandle faceHandle : geometry::TopologyTraversal::faces(mesh)) {
            if (!mesh.is_valid(faceHandle)) {
                continue;
            }

            if (target_ == FlipFaceTarget::SelectedFaces && !mesh.face(faceHandle).selected) {
                continue;
            }

            result.push_back(faceHandle);
        }

        return result;
    }

    bool FlipFaceOp::flip_face(
        geometry::LEM& mesh,
        geometry::FaceHandle faceHandle,
        geometry::LEMDiff& diff) const
    {
        if (!mesh.is_valid(faceHandle)) {
            return false;
        }

        const std::vector<geometry::LoopHandle> loops = mesh.face_loops(faceHandle);

        if (loops.size() < 3) {
            return false;
        }

        for (geometry::LoopHandle loopHandle : loops) {
            if (!mesh.is_valid(loopHandle)) {
                return false;
            }
        }

        for (geometry::LoopHandle loopHandle : loops) {
            geometry::Loop& loop = mesh.loop(loopHandle);
            const geometry::LoopHandle next = loop.next;
            loop.next = loop.previous;
            loop.previous = next;
            diff.record(geometry::LEMChangeType::LoopModified, loopHandle);
        }

        mesh.face(faceHandle).normal = geometry::NormalBuilder::face_normal(mesh, faceHandle);
        diff.record(geometry::LEMChangeType::FaceModified, faceHandle);
        diff.record(geometry::LEMChangeType::NormalsChanged, faceHandle);

        return true;
    }

}
