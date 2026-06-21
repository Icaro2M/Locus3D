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
        , diff_(diff)
    {
    }

    LEM& GeometryEditor::mesh()
    {
        return mesh_;
    }

    const LEM& GeometryEditor::mesh() const
    {
        return mesh_;
    }

    bool GeometryEditor::set_vertex_position(VertexHandle handle, const glm::vec3& position)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);

        if (vertex.position == position) {
            return true;
        }

        vertex.position = position;
        diff_.record(LEMChangeType::VertexModified, handle);

        rebuild_adjacent_face_normals(handle);

        return true;
    }

    bool GeometryEditor::translate_vertex(VertexHandle handle, const glm::vec3& offset)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        return set_vertex_position(handle, mesh_.vertex(handle).position + offset);
    }

    std::size_t GeometryEditor::translate_vertices(const std::vector<VertexHandle>& vertices, const glm::vec3& offset)
    {
        std::size_t count = 0;

        for (VertexHandle handle : vertices) {
            if (translate_vertex(handle, offset)) {
                ++count;
            }
        }

        return count;
    }

    void GeometryEditor::rebuild_adjacent_face_normals(VertexHandle vertex)
    {
        for (FaceHandle faceHandle : TopologyTraversal::vertex_faces(mesh_, vertex)) {
            if (!mesh_.is_valid(faceHandle)) {
                continue;
            }

            mesh_.face(faceHandle).normal = NormalBuilder::face_normal(mesh_, faceHandle);
            diff_.record(LEMChangeType::NormalsChanged, faceHandle);
        }
    }

}