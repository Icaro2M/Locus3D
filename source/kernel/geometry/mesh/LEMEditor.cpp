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
        , attributes_(mesh_, diff_) {
    }

    LEM& LEMEditor::mesh() {
        return mesh_;
    }

    const LEM& LEMEditor::mesh() const {
        return mesh_;
    }

    TopologyEditor& LEMEditor::topology() {
        return topology_;
    }

    const TopologyEditor& LEMEditor::topology() const {
        return topology_;
    }

    GeometryEditor& LEMEditor::geometry() {
        return geometry_;
    }

    const GeometryEditor& LEMEditor::geometry() const {
        return geometry_;
    }

    AttributeEditor& LEMEditor::attributes() {
        return attributes_;
    }

    const AttributeEditor& LEMEditor::attributes() const {
        return attributes_;
    }

    const LEMDiff& LEMEditor::diff() const {
        return diff_;
    }

    LEMDiff LEMEditor::take_diff() {
        LEMDiff result = diff_;
        diff_.clear();
        return result;
    }

    void LEMEditor::clear_diff() {
        diff_.clear();
    }

    VertexHandle LEMEditor::add_vertex(const glm::vec3& position) {
        return topology_.add_vertex(position);
    }

    EdgeHandle LEMEditor::find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB) {
        return topology_.find_or_create_edge(vertexA, vertexB);
    }

    FaceHandle LEMEditor::add_face(const std::vector<VertexHandle>& vertices) {
        return topology_.add_face(vertices);
    }

    bool LEMEditor::remove_face(FaceHandle face) {
        return topology_.remove_face(face);
    }

    bool LEMEditor::remove_edge_if_loose(EdgeHandle edge) {
        return topology_.remove_edge_if_loose(edge);
    }

    bool LEMEditor::remove_vertex_if_loose(VertexHandle vertex) {
        return topology_.remove_vertex_if_loose(vertex);
    }

    bool LEMEditor::flip_face(FaceHandle face) {
        return topology_.flip_face(face);
    }

    std::size_t LEMEditor::flip_all_faces() {
        return topology_.flip_all_faces();
    }

    bool LEMEditor::set_vertex_position(VertexHandle handle, const glm::vec3& position) {
        return geometry_.set_vertex_position(handle, position);
    }

    bool LEMEditor::translate_vertex(VertexHandle handle, const glm::vec3& offset) {
        return geometry_.translate_vertex(handle, offset);
    }

    std::size_t LEMEditor::translate_vertices(const std::vector<VertexHandle>& vertices, const glm::vec3& offset) {
        return geometry_.translate_vertices(vertices, offset);
    }

    std::size_t LEMEditor::transform_vertices(const std::vector<VertexHandle>& vertices, const glm::mat4& transform) {
        return geometry_.transform_vertices(vertices, transform);
    }

    bool LEMEditor::set_selected(VertexHandle handle, bool selected) {
        return attributes_.set_selected(handle, selected);
    }

    bool LEMEditor::set_selected(EdgeHandle handle, bool selected) {
        return attributes_.set_selected(handle, selected);
    }

    bool LEMEditor::set_selected(FaceHandle handle, bool selected) {
        return attributes_.set_selected(handle, selected);
    }

    void LEMEditor::clear_selection() {
        attributes_.clear_selection();
    }

    bool LEMEditor::set_hidden(VertexHandle handle, bool hidden) {
        return attributes_.set_hidden(handle, hidden);
    }

    bool LEMEditor::set_hidden(EdgeHandle handle, bool hidden) {
        return attributes_.set_hidden(handle, hidden);
    }

    bool LEMEditor::set_hidden(FaceHandle handle, bool hidden) {
        return attributes_.set_hidden(handle, hidden);
    }

    void LEMEditor::clear_visibility() {
        attributes_.clear_visibility();
    }

    void LEMEditor::rebuild_face_normals() {
        geometry_.rebuild_face_normals();
    }

    void LEMEditor::clear() {
        topology_.clear();
    }

}