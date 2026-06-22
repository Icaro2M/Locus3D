/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/GeometryEditor.h"

#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

namespace locus::kernel::geometry {

    GeometryEditor::GeometryEditor(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff) {
    }

    LEM& GeometryEditor::mesh() {
        return mesh_;
    }

    const LEM& GeometryEditor::mesh() const {
        return mesh_;
    }

    bool GeometryEditor::set_vertex_position(VertexHandle handle, const glm::vec3& position) {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);

        if (vertex.position == position) {
            return true;
        }

        vertex.position = position;

        diff_.record(LEMChangeType::VertexModified, handle);
        rebuild_normals_around_vertex(handle);

        return true;
    }

    bool GeometryEditor::translate_vertex(VertexHandle handle, const glm::vec3& offset) {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        return set_vertex_position(handle, mesh_.vertex(handle).position + offset);
    }

    std::size_t GeometryEditor::translate_vertices(const std::vector<VertexHandle>& vertices, const glm::vec3& offset) {
        std::size_t count = 0;

        for (VertexHandle handle : vertices) {
            if (translate_vertex(handle, offset)) {
                ++count;
            }
        }

        return count;
    }

    std::size_t GeometryEditor::transform_vertices(const std::vector<VertexHandle>& vertices, const glm::mat4& transform) {
        std::size_t count = 0;

        for (VertexHandle handle : vertices) {
            if (!mesh_.is_valid(handle)) {
                continue;
            }

            const glm::vec4 transformed = transform * glm::vec4(mesh_.vertex(handle).position, 1.0f);

            if (set_vertex_position(handle, glm::vec3(transformed))) {
                ++count;
            }
        }

        return count;
    }

    void GeometryEditor::rebuild_face_normals() {
        NormalBuilder::rebuild_face_normals(mesh_);

        for (FaceHandle faceHandle : TopologyTraversal::faces(mesh_)) {
            diff_.record(LEMChangeType::NormalsChanged, faceHandle);
        }
    }

    void GeometryEditor::rebuild_normals_around_vertex(VertexHandle vertex) {
        if (!mesh_.is_valid(vertex)) {
            return;
        }

        for (FaceHandle faceHandle : TopologyTraversal::vertex_faces(mesh_, vertex)) {
            rebuild_normals_around_face(faceHandle);
        }
    }

    bool GeometryEditor::rebuild_normals_around_face(FaceHandle face) {
        if (!mesh_.is_valid(face)) {
            return false;
        }

        mesh_.face(face).normal = NormalBuilder::face_normal(mesh_, face);
        diff_.record(LEMChangeType::NormalsChanged, face);

        return true;
    }

}