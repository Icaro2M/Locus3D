/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/LEMEditor.h"

namespace locus::kernel::geometry {

    LEMEditor::LEMEditor(LEM& mesh)
        : mesh_(mesh)
        , diff_()
        , topology_(mesh_, diff_)
        , geometry_(mesh_, diff_)
        , attributes_(mesh_, diff_)
    {
    }

    LEM& LEMEditor::mesh()
    {
        return mesh_;
    }

    const LEM& LEMEditor::mesh() const
    {
        return mesh_;
    }

    TopologyEditor& LEMEditor::topology()
    {
        return topology_;
    }

    const TopologyEditor& LEMEditor::topology() const
    {
        return topology_;
    }

    GeometryEditor& LEMEditor::geometry()
    {
        return geometry_;
    }

    const GeometryEditor& LEMEditor::geometry() const
    {
        return geometry_;
    }

    AttributeEditor& LEMEditor::attributes()
    {
        return attributes_;
    }

    const AttributeEditor& LEMEditor::attributes() const
    {
        return attributes_;
    }

    const LEMDiff& LEMEditor::diff() const
    {
        return diff_;
    }

    LEMDiff LEMEditor::take_diff()
    {
        LEMDiff result = diff_;
        diff_.clear();
        return result;
    }

    void LEMEditor::clear_diff()
    {
        diff_.clear();
    }

    VertexHandle LEMEditor::add_vertex(const glm::vec3& position)
    {
        return topology_.add_vertex(position);
    }

    EdgeHandle LEMEditor::find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB)
    {
        return topology_.find_or_create_edge(vertexA, vertexB);
    }

    FaceHandle LEMEditor::add_face(const std::vector<VertexHandle>& vertices)
    {
        return topology_.add_face(vertices);
    }

    bool LEMEditor::remove_face(FaceHandle face)
    {
        return topology_.remove_face(face);
    }

    bool LEMEditor::remove_edge_if_loose(EdgeHandle edge)
    {
        return topology_.remove_edge_if_loose(edge);
    }

    bool LEMEditor::remove_vertex_if_loose(VertexHandle vertex)
    {
        return topology_.remove_vertex_if_loose(vertex);
    }

    bool LEMEditor::kill_face_only(FaceHandle face)
    {
        return topology_.kill_face_only(face);
    }

    bool LEMEditor::kill_edge_only(EdgeHandle edge)
    {
        return topology_.kill_edge_only(edge);
    }

    bool LEMEditor::kill_loop(LoopHandle loop)
    {
        return topology_.kill_loop(loop);
    }

    std::optional<VertexHandle> LEMEditor::split_edge(EdgeHandle edge)
    {
        return topology_.split_edge(edge);
    }

    std::optional<VertexHandle> LEMEditor::split_edge_at_param(EdgeHandle edge, float t)
    {
        return topology_.split_edge_at_param(edge, t);
    }

    std::optional<EdgeHandle> LEMEditor::split_face(
        FaceHandle face,
        VertexHandle vertexA,
        VertexHandle vertexB)
    {
        return topology_.split_face(face, vertexA, vertexB);
    }

    bool LEMEditor::collapse_edge(EdgeHandle edge)
    {
        return topology_.collapse_edge(edge);
    }

    bool LEMEditor::merge_vertices(VertexHandle sourceVertex, VertexHandle targetVertex)
    {
        return topology_.merge_vertices(sourceVertex, targetVertex);
    }

    bool LEMEditor::merge_vertices_at_position(
        VertexHandle sourceVertex,
        VertexHandle targetVertex,
        const glm::vec3& position)
    {
        return topology_.merge_vertices_at_position(sourceVertex, targetVertex, position);
    }

    std::size_t LEMEditor::merge_vertices_by_distance(float distance)
    {
        return topology_.merge_vertices_by_distance(distance);
    }

    std::size_t LEMEditor::weld_vertices(
        const std::vector<VertexHandle>& vertices,
        float distance)
    {
        return topology_.weld_vertices(vertices, distance);
    }

    bool LEMEditor::dissolve_edge(EdgeHandle edge)
    {
        return topology_.dissolve_edge(edge);
    }

    bool LEMEditor::dissolve_vertex(VertexHandle vertex)
    {
        return topology_.dissolve_vertex(vertex);
    }

    bool LEMEditor::dissolve_face(FaceHandle face)
    {
        return topology_.dissolve_face(face);
    }

    bool LEMEditor::flip_face(FaceHandle face)
    {
        return topology_.flip_face(face);
    }

    std::size_t LEMEditor::flip_all_faces()
    {
        return topology_.flip_all_faces();
    }

    bool LEMEditor::flip_edge(EdgeHandle edge)
    {
        return topology_.flip_edge(edge);
    }

    bool LEMEditor::set_vertex_position(VertexHandle handle, const glm::vec3& position)
    {
        return geometry_.set_vertex_position(handle, position);
    }

    bool LEMEditor::set_vertex_position_lerp(
        VertexHandle handle,
        const glm::vec3& target,
        float t)
    {
        return geometry_.set_vertex_position_lerp(handle, target, t);
    }

    bool LEMEditor::translate_vertex(VertexHandle handle, const glm::vec3& offset)
    {
        return geometry_.translate_vertex(handle, offset);
    }

    std::size_t LEMEditor::translate_vertices(
        const std::vector<VertexHandle>& vertices,
        const glm::vec3& offset)
    {
        return geometry_.translate_vertices(vertices, offset);
    }

    std::size_t LEMEditor::transform_vertices(
        const std::vector<VertexHandle>& vertices,
        const glm::mat4& transform)
    {
        return geometry_.transform_vertices(vertices, transform);
    }

    bool LEMEditor::offset_vertex_along_normal(VertexHandle handle, float distance)
    {
        return geometry_.offset_vertex_along_normal(handle, distance);
    }

    void LEMEditor::rebuild_face_normals()
    {
        geometry_.rebuild_face_normals();
    }

    void LEMEditor::rebuild_normals_around_vertex(VertexHandle vertex)
    {
        geometry_.rebuild_normals_around_vertex(vertex);
    }

    bool LEMEditor::rebuild_normals_around_face(FaceHandle face)
    {
        return geometry_.rebuild_normals_around_face(face);
    }

    bool LEMEditor::set_selected(VertexHandle handle, bool selected)
    {
        return attributes_.set_selected(handle, selected);
    }

    bool LEMEditor::set_selected(EdgeHandle handle, bool selected)
    {
        return attributes_.set_selected(handle, selected);
    }

    bool LEMEditor::set_selected(FaceHandle handle, bool selected)
    {
        return attributes_.set_selected(handle, selected);
    }

    void LEMEditor::clear_selection()
    {
        attributes_.clear_selection();
    }

    bool LEMEditor::set_hidden(VertexHandle handle, bool hidden)
    {
        return attributes_.set_hidden(handle, hidden);
    }

    bool LEMEditor::set_hidden(EdgeHandle handle, bool hidden)
    {
        return attributes_.set_hidden(handle, hidden);
    }

    bool LEMEditor::set_hidden(FaceHandle handle, bool hidden)
    {
        return attributes_.set_hidden(handle, hidden);
    }

    void LEMEditor::clear_visibility()
    {
        attributes_.clear_visibility();
    }

    bool LEMEditor::set_smooth(EdgeHandle handle, bool smooth)
    {
        return attributes_.set_smooth(handle, smooth);
    }

    bool LEMEditor::set_crease(EdgeHandle handle, float crease)
    {
        return attributes_.set_crease(handle, crease);
    }

    bool LEMEditor::set_tag(VertexHandle handle, std::uint32_t tag)
    {
        return attributes_.set_tag(handle, tag);
    }

    bool LEMEditor::set_tag(EdgeHandle handle, std::uint32_t tag)
    {
        return attributes_.set_tag(handle, tag);
    }

    bool LEMEditor::set_tag(FaceHandle handle, std::uint32_t tag)
    {
        return attributes_.set_tag(handle, tag);
    }

    void LEMEditor::clear_tags()
    {
        attributes_.clear_tags();
    }

    void LEMEditor::clear()
    {
        topology_.clear();
    }

}