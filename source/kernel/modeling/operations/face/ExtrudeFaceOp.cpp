/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/face/ExtrudeFaceOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/geometric.hpp>
#include <utility>

namespace locus::kernel::modeling {

    ExtrudeFaceOp::ExtrudeFaceOp(geometry::FaceHandle face, float distance)
        : faces_({ face })
        , distance_(distance)
    {
    }

    ExtrudeFaceOp::ExtrudeFaceOp(
        std::vector<geometry::FaceHandle> faces,
        float distance)
        : faces_(std::move(faces))
        , distance_(distance)
    {
    }

    ExtrudeFaceOp::ExtrudeFaceOp(
        geometry::FaceHandle face,
        const glm::vec3& offset)
        : directionMode_(ExtrudeFaceDirectionMode::ExplicitOffset)
        , faces_({ face })
        , offset_(offset)
    {
    }

    ExtrudeFaceOp::ExtrudeFaceOp(
        std::vector<geometry::FaceHandle> faces,
        const glm::vec3& offset)
        : directionMode_(ExtrudeFaceDirectionMode::ExplicitOffset)
        , faces_(std::move(faces))
        , offset_(offset)
    {
    }

    ExtrudeFaceOp ExtrudeFaceOp::selected(float distance)
    {
        ExtrudeFaceOp op;
        op.set_target(ExtrudeFaceTarget::SelectedFaces);
        op.set_distance(distance);
        return op;
    }

    ExtrudeFaceOp ExtrudeFaceOp::selected(const glm::vec3& offset)
    {
        ExtrudeFaceOp op;
        op.set_target(ExtrudeFaceTarget::SelectedFaces);
        op.set_offset(offset);
        return op;
    }

    std::string_view ExtrudeFaceOp::name() const
    {
        return "ExtrudeFaceOp";
    }

    void ExtrudeFaceOp::set_target(ExtrudeFaceTarget target)
    {
        target_ = target;
    }

    ExtrudeFaceTarget ExtrudeFaceOp::target() const
    {
        return target_;
    }

    void ExtrudeFaceOp::set_direction_mode(ExtrudeFaceDirectionMode mode)
    {
        directionMode_ = mode;
    }

    ExtrudeFaceDirectionMode ExtrudeFaceOp::direction_mode() const
    {
        return directionMode_;
    }

    void ExtrudeFaceOp::set_distance(float distance)
    {
        directionMode_ = ExtrudeFaceDirectionMode::FaceNormal;
        distance_ = distance;
    }

    float ExtrudeFaceOp::distance() const
    {
        return distance_;
    }

    void ExtrudeFaceOp::set_offset(const glm::vec3& offset)
    {
        directionMode_ = ExtrudeFaceDirectionMode::ExplicitOffset;
        offset_ = offset;
    }

    const glm::vec3& ExtrudeFaceOp::offset() const
    {
        return offset_;
    }

    void ExtrudeFaceOp::set_faces(std::vector<geometry::FaceHandle> faces)
    {
        faces_ = std::move(faces);
    }

    const std::vector<geometry::FaceHandle>& ExtrudeFaceOp::faces() const
    {
        return faces_;
    }

    void ExtrudeFaceOp::clear_faces()
    {
        faces_.clear();
    }

    void ExtrudeFaceOp::set_keep_source_face(bool keepSourceFace)
    {
        keepSourceFace_ = keepSourceFace;
    }

    bool ExtrudeFaceOp::keep_source_face() const
    {
        return keepSourceFace_;
    }

    OperationResult ExtrudeFaceOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::FaceHandle> targets = collect_faces(mesh);

        if (targets.empty()) {
            return OperationResult::no_change(
                "Extrude face operation found no valid faces.");
        }

        geometry::LEMEditor editor(mesh);
        std::size_t extrudedCount = 0;

        for (geometry::FaceHandle face : targets) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            if (extrude_face(mesh, editor, face)) {
                ++extrudedCount;
            }
        }

        if (extrudedCount == 0) {
            return OperationResult::no_change(
                "Extrude face operation did not modify the mesh.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::FaceHandle> ExtrudeFaceOp::collect_faces(
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

            if (target_ == ExtrudeFaceTarget::SelectedFaces &&
                !mesh.face(face).selected) {
                continue;
            }

            result.push_back(face);
        }

        return result;
    }

    glm::vec3 ExtrudeFaceOp::offset_for_face(
        const geometry::LEM& mesh,
        geometry::FaceHandle face) const
    {
        if (directionMode_ == ExtrudeFaceDirectionMode::ExplicitOffset) {
            return offset_;
        }

        const glm::vec3 normal = geometry::NormalBuilder::face_normal(mesh, face);
        return normal * distance_;
    }

    bool ExtrudeFaceOp::extrude_face(
        geometry::LEM& mesh,
        geometry::LEMEditor& editor,
        geometry::FaceHandle face) const
    {
        if (!mesh.is_valid(face)) {
            return false;
        }

        const std::vector<geometry::VertexHandle> sourceVertices =
            geometry::TopologyTraversal::face_vertices(mesh, face);

        if (sourceVertices.size() < 3) {
            return false;
        }

        std::vector<glm::vec3> sourcePositions;
        sourcePositions.reserve(sourceVertices.size());

        for (geometry::VertexHandle vertex : sourceVertices) {
            if (!mesh.is_valid(vertex)) {
                return false;
            }

            sourcePositions.push_back(mesh.vertex(vertex).position);
        }

        const glm::vec3 extrusionOffset = offset_for_face(mesh, face);

        if (glm::length(extrusionOffset) <= 0.0f) {
            return false;
        }

        std::vector<geometry::VertexHandle> extrudedVertices;
        extrudedVertices.reserve(sourceVertices.size());

        for (const glm::vec3& position : sourcePositions) {
            geometry::VertexHandle vertex =
                editor.add_vertex(position + extrusionOffset);

            if (!mesh.is_valid(vertex)) {
                return false;
            }

            extrudedVertices.push_back(vertex);
        }

        if (!keepSourceFace_) {
            if (!editor.remove_face(face)) {
                return false;
            }
        }

        std::size_t createdFaceCount = 0;

        const geometry::FaceHandle capFace = editor.add_face(extrudedVertices);
        if (mesh.is_valid(capFace)) {
            ++createdFaceCount;
        }

        for (std::size_t i = 0; i < sourceVertices.size(); ++i) {
            const std::size_t next = (i + 1) % sourceVertices.size();

            const geometry::VertexHandle sourceA = sourceVertices[i];
            const geometry::VertexHandle sourceB = sourceVertices[next];
            const geometry::VertexHandle extrudedA = extrudedVertices[i];
            const geometry::VertexHandle extrudedB = extrudedVertices[next];

            if (!mesh.is_valid(sourceA) ||
                !mesh.is_valid(sourceB) ||
                !mesh.is_valid(extrudedA) ||
                !mesh.is_valid(extrudedB)) {
                continue;
            }

            const geometry::FaceHandle sideFace =
                editor.add_face({ sourceA, sourceB, extrudedB, extrudedA });

            if (mesh.is_valid(sideFace)) {
                ++createdFaceCount;
            }
        }

        return createdFaceCount > 0;
    }

}