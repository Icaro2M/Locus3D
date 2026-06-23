/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/face/InsetFaceOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/vec3.hpp>

#include <utility>

namespace locus::kernel::modeling {

    InsetFaceOp::InsetFaceOp(geometry::FaceHandle face, float factor)
        : faces_({ face })
        , factor_(factor)
    {
    }

    InsetFaceOp::InsetFaceOp(
        std::vector<geometry::FaceHandle> faces,
        float factor)
        : faces_(std::move(faces))
        , factor_(factor)
    {
    }

    InsetFaceOp InsetFaceOp::selected(float factor)
    {
        InsetFaceOp op;
        op.set_target(InsetFaceTarget::SelectedFaces);
        op.set_factor(factor);
        return op;
    }

    std::string_view InsetFaceOp::name() const
    {
        return "InsetFaceOp";
    }

    void InsetFaceOp::set_target(InsetFaceTarget target)
    {
        target_ = target;
    }

    InsetFaceTarget InsetFaceOp::target() const
    {
        return target_;
    }

    void InsetFaceOp::set_factor(float factor)
    {
        factor_ = factor;
    }

    float InsetFaceOp::factor() const
    {
        return factor_;
    }

    void InsetFaceOp::set_faces(std::vector<geometry::FaceHandle> faces)
    {
        faces_ = std::move(faces);
    }

    const std::vector<geometry::FaceHandle>& InsetFaceOp::faces() const
    {
        return faces_;
    }

    void InsetFaceOp::clear_faces()
    {
        faces_.clear();
    }

    OperationResult InsetFaceOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::FaceHandle> targets = collect_faces(mesh);

        if (targets.empty()) {
            return OperationResult::no_change(
                "Inset face operation found no valid faces.");
        }

        geometry::LEMEditor editor(mesh);
        std::size_t insetCount = 0;

        for (geometry::FaceHandle face : targets) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            if (inset_face(mesh, editor, face)) {
                ++insetCount;
            }
        }

        if (insetCount == 0) {
            return OperationResult::no_change(
                "Inset face operation did not modify the mesh.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::FaceHandle> InsetFaceOp::collect_faces(
        const geometry::LEM& mesh) const
    {
        std::vector<geometry::FaceHandle> result;

        if (!faces_.empty()) {
            result.reserve(faces_.size());

            for (geometry::FaceHandle face : faces_) {
                if (!mesh.is_valid(face) || contains(result, face)) {
                    continue;
                }

                result.push_back(face);
            }

            return result;
        }

        const std::vector<geometry::FaceHandle> activeFaces =
            geometry::TopologyTraversal::faces(mesh);

        result.reserve(activeFaces.size());

        for (geometry::FaceHandle face : activeFaces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            if (target_ == InsetFaceTarget::SelectedFaces &&
                !mesh.face(face).selected) {
                continue;
            }

            result.push_back(face);
        }

        return result;
    }

    bool InsetFaceOp::inset_face(
        geometry::LEM& mesh,
        geometry::LEMEditor& editor,
        geometry::FaceHandle face) const
    {
        if (!mesh.is_valid(face)) {
            return false;
        }

        const float clampedFactor = std::clamp(factor_, 0.0f, 1.0f);

        if (clampedFactor <= 0.0f || clampedFactor >= 1.0f) {
            return false;
        }

        const std::vector<geometry::VertexHandle> sourceVertices =
            geometry::TopologyTraversal::face_vertices(mesh, face);

        if (sourceVertices.size() < 3) {
            return false;
        }

        std::vector<glm::vec3> sourcePositions;
        sourcePositions.reserve(sourceVertices.size());

        glm::vec3 center{ 0.0f };

        for (geometry::VertexHandle vertex : sourceVertices) {
            if (!mesh.is_valid(vertex)) {
                return false;
            }

            const glm::vec3& position = mesh.vertex(vertex).position;
            sourcePositions.push_back(position);
            center += position;
        }

        center /= static_cast<float>(sourcePositions.size());

        std::vector<geometry::VertexHandle> insetVertices;
        insetVertices.reserve(sourceVertices.size());

        for (const glm::vec3& position : sourcePositions) {
            const glm::vec3 insetPosition =
                position + (center - position) * clampedFactor;

            const geometry::VertexHandle insetVertex =
                editor.add_vertex(insetPosition);

            if (!mesh.is_valid(insetVertex)) {
                return false;
            }

            insetVertices.push_back(insetVertex);
        }

        if (!editor.remove_face(face)) {
            return false;
        }

        std::size_t createdFaceCount = 0;

        const geometry::FaceHandle innerFace = editor.add_face(insetVertices);
        if (mesh.is_valid(innerFace)) {
            ++createdFaceCount;
        }

        for (std::size_t i = 0; i < sourceVertices.size(); ++i) {
            const std::size_t next = (i + 1) % sourceVertices.size();

            const geometry::VertexHandle sourceA = sourceVertices[i];
            const geometry::VertexHandle sourceB = sourceVertices[next];
            const geometry::VertexHandle insetA = insetVertices[i];
            const geometry::VertexHandle insetB = insetVertices[next];

            if (!mesh.is_valid(sourceA) ||
                !mesh.is_valid(sourceB) ||
                !mesh.is_valid(insetA) ||
                !mesh.is_valid(insetB)) {
                continue;
            }

            const geometry::FaceHandle sideFace =
                editor.add_face({ sourceA, sourceB, insetB, insetA });

            if (mesh.is_valid(sideFace)) {
                ++createdFaceCount;
            }
        }

        return createdFaceCount > 0;
    }

}