/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/GeometryEditor.h"

namespace locus::kernel::geometry {

    GeometryEditor::GeometryEditor(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
        , position_(mesh, diff)
        , transform_(mesh, diff)
        , normals_(mesh, diff)
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
        return position_.set_vertex_position(handle, position);
    }

    bool GeometryEditor::set_vertex_position_lerp(
        VertexHandle handle,
        const glm::vec3& target,
        float t)
    {
        return position_.set_vertex_position_lerp(handle, target, t);
    }

    bool GeometryEditor::translate_vertex(VertexHandle handle, const glm::vec3& offset)
    {
        return position_.translate_vertex(handle, offset);
    }

    std::size_t GeometryEditor::translate_vertices(
        const std::vector<VertexHandle>& vertices,
        const glm::vec3& offset)
    {
        return transform_.translate_vertices(vertices, offset);
    }

    std::size_t GeometryEditor::transform_vertices(
        const std::vector<VertexHandle>& vertices,
        const glm::mat4& transform)
    {
        return transform_.transform_vertices(vertices, transform);
    }

    bool GeometryEditor::offset_vertex_along_normal(VertexHandle handle, float distance)
    {
        return position_.offset_vertex_along_normal(handle, distance);
    }

    void GeometryEditor::rebuild_face_normals()
    {
        normals_.rebuild_face_normals();
    }

    void GeometryEditor::rebuild_normals_around_vertex(VertexHandle vertex)
    {
        normals_.rebuild_normals_around_vertex(vertex);
    }

    bool GeometryEditor::rebuild_normals_around_face(FaceHandle face)
    {
        return normals_.rebuild_normals_around_face(face);
    }

}